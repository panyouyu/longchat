/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "app.h"

#include "lang/lang_keys.h"
#include "boxes/confirm_box.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_user.h"
#include "data/data_abstract_structure.h"
#include "data/data_media_types.h"
#include "data/data_session.h"
#include "data/data_document.h"
#include "history/history.h"
#include "history/history_location_manager.h"
#include "history/history_item_components.h"
#include "history/view/history_view_service_message.h"
#include "media/audio/media_audio.h"
#include "ui/image/image.h"
#include "inline_bots/inline_bot_layout_item.h"
#include "core/crash_reports.h"
#include "core/update_checker.h"
#include "core/sandbox.h"
#include "core/application.h"
#include "window/themes/window_theme.h"
#include "window/notifications_manager.h"
#include "platform/platform_notifications_manager.h"
#include "storage/file_upload.h"
#include "storage/localstorage.h"
#include "storage/storage_facade.h"
#include "storage/storage_shared_media.h"
#include "mainwindow.h"
#include "mainwidget.h"
#include "apiwrap.h"
#include "numbers.h"
#include "observer_peer.h"
#include "auth_session.h"
#include "styles/style_overview.h"
#include "styles/style_mediaview.h"
#include "styles/style_chat_helpers.h"
#include "styles/style_history.h"
#include "styles/style_boxes.h"

#ifdef OS_MAC_OLD
#include <libexif/exif-data.h>
#endif // OS_MAC_OLD

namespace {
	App::LaunchState _launchState = App::Launched;

	using DependentItemsSet = OrderedSet<HistoryItem*>;
	using DependentItems = QMap<HistoryItem*, DependentItemsSet>;
	DependentItems dependentItems;

	using MsgsData = QHash<MsgId, HistoryItem*>;
	MsgsData msgsData;
	using ChannelMsgsData = QMap<ChannelId, MsgsData>;
	ChannelMsgsData channelMsgsData;

	using RandomData = QMap<uint64, FullMsgId>;
	RandomData randomData;

	using SentData = QMap<uint64, QPair<PeerId, QString>>;
	SentData sentData;

	HistoryView::Element *hoveredItem = nullptr,
		*pressedItem = nullptr,
		*hoveredLinkItem = nullptr,
		*pressedLinkItem = nullptr,
		*mousedItem = nullptr;

	style::font monofont;

	struct CornersPixmaps {
		QPixmap p[4];
	};
	QVector<CornersPixmaps> corners;
	using CornersMap = QMap<uint32, CornersPixmaps>;
	CornersMap cornersMap;
	QImage cornersMaskLarge[4], cornersMaskSmall[4];

	int32 serviceImageCacheSize = 0;

} // namespace

namespace App {

	QString formatPhone(QString phone) {
		if (phone.isEmpty()) return QString();
		if (phone.at(0) == '0') return phone;

		QString number = phone;
		for (const QChar *ch = phone.constData(), *e = ch + phone.size(); ch != e; ++ch) {
			if (ch->unicode() < '0' || ch->unicode() > '9') {
				number = phone.replace(QRegularExpression(qsl("[^\\d]")), QString());
			}
		}

		// hide number ro ***xxxx****
		if (number.size() > 4) {
			auto left = number.size() > 8 
				? number.size() - 8
				: 0;
			auto right = number.size() - 4;
			for (int i = 0; i < left; ++i) {
				number[i] = '*';
			}
			for (int i = right; i < number.size(); ++i) {
				number[i] = '*';
			}
			return number;
		}

		QVector<int> groups = phoneNumberParse(number);
		if (groups.isEmpty()) return '+' + number;

		QString result;
		result.reserve(number.size() + groups.size() + 1);
		result.append('+');
		int32 sum = 0;
		for (int32 i = 0, l = groups.size(); i < l; ++i) {
			result.append(number.midRef(sum, groups.at(i)));
			sum += groups.at(i);
			if (sum < number.size()) result.append(' ');
		}
		if (sum < number.size()) result.append(number.midRef(sum));
		return result;
	}

	MainWindow *wnd() {
		return Core::IsAppLaunched()
			? Core::App().getActiveWindow()
			: nullptr;
	}

	MainWidget *main() {
		if (auto window = wnd()) {
			return window->mainWidget();
		}
		return nullptr;
	}

	bool checkEntitiesAndViewsUpdate(const MTPDmessage &m) {
		auto peerId = peerFromMTP(m.vto_id);
		if (m.has_from_id() && peerId == Auth().userPeerId()) {
			peerId = peerFromUser(m.vfrom_id);
		}
		if (const auto existing = App::histItemById(peerToChannel(peerId), m.vid.v)) {
			auto text = qs(m.vmessage);
			auto entities = m.has_entities()
				? TextUtilities::EntitiesFromMTP(m.ventities.v)
				: EntitiesInText();
			existing->setText({ text, entities });
			existing->updateSentMedia(m.has_media() ? &m.vmedia : nullptr);
			existing->updateReplyMarkup(m.has_reply_markup()
				? (&m.vreply_markup)
				: nullptr);
			existing->updateForwardedInfo(m.has_fwd_from()
				? &m.vfwd_from
				: nullptr);
			existing->setViewsCount(m.has_views() ? m.vviews.v : -1);
			existing->indexAsNewItem();
			Auth().data().requestItemTextRefresh(existing);
			if (existing->mainView()) {
				App::checkSavedGif(existing);
				return true;
			}
			return false;
		}
		return false;
	}

	void updateEditedMessage(const MTPMessage &m) {
		m.match([](const MTPDmessageEmpty &) {
		}, [&m](const auto &message) {
			auto peerId = peerFromMTP(message.vto_id);
			if (message.has_from_id() && peerId == Auth().userPeerId()) {
				peerId = peerFromUser(message.vfrom_id);
			}
			const auto existing = App::histItemById(
				peerToChannel(peerId),
				message.vid.v);
			if (existing) {
				if (existing->isLocalUpdateMedia()) {
					checkEntitiesAndViewsUpdate(m.c_message());
				}
				existing->applyEdition(message);
			}
		});
	}

	void addSavedGif(DocumentData *doc) {
		auto &saved = Auth().data().savedGifsRef();
		int32 index = saved.indexOf(doc);
		if (index) {
			if (index > 0) saved.remove(index);
			saved.push_front(doc);
			if (saved.size() > Global::SavedGifsLimit()) saved.pop_back();
			Local::writeSavedGifs();

			Auth().data().notifySavedGifsUpdated();
			Auth().data().setLastSavedGifsUpdate(0);
			Auth().api().updateStickers();
		}
	}

	void checkSavedGif(HistoryItem *item) {
		if (!item->Has<HistoryMessageForwarded>() && (item->out() || item->history()->peer == Auth().user())) {
			if (const auto media = item->media()) {
				if (const auto document = media->document()) {
					if (document->isGifv()) {
						addSavedGif(document);
					}
				}
			}
		}
	}

	void feedMsgs(const QVector<MTPMessage> &msgs, NewMessageType type) {
		auto indices = base::flat_map<uint64, int>();
		for (int i = 0, l = msgs.size(); i != l; ++i) {
			const auto &msg = msgs[i];
			if (msg.type() == mtpc_message) {
				const auto &data = msg.c_message();
				if (type == NewMessageUnread) { // new message, index my forwarded messages to links overview
					if (checkEntitiesAndViewsUpdate(data)) { // already in blocks
						LOG(("Skipping message, because it is already in blocks!"));
						continue;
					}
				}
			}
			const auto msgId = IdFromMessage(msg);
			indices.emplace((uint64(uint32(msgId)) << 32) | uint64(i), i);
		}
		for (const auto [position, index] : indices) {
			Auth().data().addNewMessage(msgs[index], type);
		}
	}

	void feedMsgs(const MTPVector<MTPMessage> &msgs, NewMessageType type) {
		return feedMsgs(msgs.v, type);
	}

	void feedInboxRead(const PeerId &peer, MsgId upTo) {
		if (const auto history = Auth().data().historyLoaded(peer)) {
			history->inboxRead(upTo);
		}
	}

	void feedOutboxRead(const PeerId &peer, MsgId upTo, TimeId when) {
		if (auto history = Auth().data().historyLoaded(peer)) {
			history->outboxRead(upTo);
			if (const auto user = history->peer->asUser()) {
				user->madeAction(when);
			}
		}
	}

	inline MsgsData *fetchMsgsData(ChannelId channelId, bool insert = true) {
		if (channelId == NoChannel) return &msgsData;
		ChannelMsgsData::iterator i = channelMsgsData.find(channelId);
		if (i == channelMsgsData.cend()) {
			if (insert) {
				i = channelMsgsData.insert(channelId, MsgsData());
			} else {
				return 0;
			}
		}
		return &(*i);
	}

	void feedWereDeleted(
			ChannelId channelId,
			const QVector<MTPint> &msgsIds) {
		const auto data = fetchMsgsData(channelId, false);
		if (!data) return;

		const auto affectedHistory = (channelId != NoChannel)
			? Auth().data().history(peerFromChannel(channelId)).get()
			: nullptr;

		auto historiesToCheck = base::flat_set<not_null<History*>>();
		for (const auto msgId : msgsIds) {
			auto j = data->constFind(msgId.v);
			if (j != data->cend()) {
				const auto history = (*j)->history();
				(*j)->destroy();
				if (!history->chatListMessageKnown()) {
					historiesToCheck.emplace(history);
				}
			} else if (affectedHistory) {
				affectedHistory->unknownMessageDeleted(msgId.v);
			}
		}
		for (const auto history : historiesToCheck) {
			history->requestChatListMessage();
		}
	}

	void feedUserLink(MTPint userId, const MTPContactLink &myLink, const MTPContactLink &foreignLink) {
		if (const auto user = Auth().data().userLoaded(userId.v)) {
			const auto wasShowPhone = (user->contactStatus() == UserData::ContactStatus::CanAdd);
			switch (myLink.type()) {
			case mtpc_contactLinkContact:
				user->setContactStatus(UserData::ContactStatus::Contact);
			break;
			case mtpc_contactLinkHasPhone:
				user->setContactStatus(UserData::ContactStatus::CanAdd);
			break;
			case mtpc_contactLinkNone:
			case mtpc_contactLinkUnknown:
				user->setContactStatus(UserData::ContactStatus::PhoneUnknown);
			break;
			}
			if (user->contactStatus() == UserData::ContactStatus::PhoneUnknown
				&& !user->phone().isEmpty()
				&& user->id != Auth().userPeerId()) {
				user->setContactStatus(UserData::ContactStatus::CanAdd);
			}

			const auto showPhone = !user->isServiceUser()
				&& !user->isSelf()
				&& user->contactStatus() == UserData::ContactStatus::CanAdd;
			const auto showPhoneChanged = !user->isServiceUser()
				&& !user->isSelf()
				&& (showPhone != wasShowPhone);
			if (showPhoneChanged) {
				user->setName(
					TextUtilities::SingleLine(user->firstName),
					TextUtilities::SingleLine(user->lastName),
					showPhone
						? App::formatPhone(user->phone())
						: QString(),
					TextUtilities::SingleLine(user->username));
			}
		}
	}

	QString peerName(const PeerData *peer, bool forDialogs) {
		return peer ? ((forDialogs && peer->isUser() && !peer->asUser()->nameOrPhone.isEmpty()) ? peer->asUser()->nameOrPhone : peer->name) : lang(lng_deleted);
	}

	HistoryItem *histItemById(ChannelId channelId, MsgId itemId) {
		if (!itemId) return nullptr;

		const auto data = fetchMsgsData(channelId, false);
		if (!data) return nullptr;

		const auto i = data->constFind(itemId);
		return (i != data->cend()) ? i.value() : nullptr;
	}

	HistoryItem *histItemById(const ChannelData *channel, MsgId itemId) {
		return histItemById(channel ? peerToChannel(channel->id) : 0, itemId);
	}

	void historyRegItem(not_null<HistoryItem*> item) {
		const auto data = fetchMsgsData(item->channelId());
		const auto i = data->constFind(item->id);
		if (i == data->cend()) {
			data->insert(item->id, item);
		} else if (i.value() != item) {
			LOG(("App Error: trying to historyRegItem() an already registered item"));
			i.value()->destroy();
			data->insert(item->id, item);
		}
	}

	void historyUnregItem(not_null<HistoryItem*> item) {
		const auto data = fetchMsgsData(item->channelId(), false);
		if (!data) return;

		const auto i = data->find(item->id);
		if (i != data->cend()) {
			if (i.value() == item) {
				data->erase(i);
			}
		}
		const auto j = ::dependentItems.find(item);
		if (j != ::dependentItems.cend()) {
			DependentItemsSet items;
			std::swap(items, j.value());
			::dependentItems.erase(j);

			for_const (auto dependent, items) {
				dependent->dependencyItemRemoved(item);
			}
		}
		item->history()->session().notifications().clearFromItem(item);
	}

	void historyUpdateDependent(not_null<HistoryItem*> item) {
		const auto j = ::dependentItems.find(item);
		if (j != ::dependentItems.cend()) {
			for_const (const auto dependent, j.value()) {
				dependent->updateDependencyItem();
			}
		}
		if (App::main()) {
			App::main()->itemEdited(item);
		}
	}

	void historyClearMsgs() {
		::dependentItems.clear();
		const auto oldData = base::take(msgsData);
		const auto oldChannelData = base::take(channelMsgsData);
		for (const auto item : oldData) {
			delete item;
		}
		for (const auto &data : oldChannelData) {
			for (const auto item : data) {
				delete item;
			}
		}

		clearMousedItems();
	}

	void historyClearItems() {
		randomData.clear();
		sentData.clear();
		cSetSavedPeers(SavedPeers());
		cSetSavedPeersByTime(SavedPeersByTime());
		cSetRecentInlineBots(RecentInlineBots());
		cSetRecentStickers(RecentStickerPack());
		cSetReportSpamStatuses(ReportSpamStatuses());
	}

	void historyRegDependency(HistoryItem *dependent, HistoryItem *dependency) {
		::dependentItems[dependency].insert(dependent);
	}

	void historyUnregDependency(HistoryItem *dependent, HistoryItem *dependency) {
		auto i = ::dependentItems.find(dependency);
		if (i != ::dependentItems.cend()) {
			i.value().remove(dependent);
			if (i.value().isEmpty()) {
				::dependentItems.erase(i);
			}
		}
	}

	void historyRegRandom(uint64 randomId, const FullMsgId &itemId) {
		randomData.insert(randomId, itemId);
	}

	void historyUnregRandom(uint64 randomId) {
		randomData.remove(randomId);
	}

	FullMsgId histItemByRandom(uint64 randomId) {
		RandomData::const_iterator i = randomData.constFind(randomId);
		if (i != randomData.cend()) {
			return i.value();
		}
		return FullMsgId();
	}

	void historyRegSentData(uint64 randomId, const PeerId &peerId, const QString &text) {
		sentData.insert(randomId, qMakePair(peerId, text));
	}

	void historyUnregSentData(uint64 randomId) {
		sentData.remove(randomId);
	}

	void histSentDataByItem(uint64 randomId, PeerId &peerId, QString &text) {
		QPair<PeerId, QString> d = sentData.value(randomId);
		peerId = d.first;
		text = d.second;
	}

	void prepareCorners(RoundCorners index, int32 radius, const QBrush &brush, const style::color *shadow = nullptr, QImage *cors = nullptr) {
		Expects(::corners.size() > index);
		int32 r = radius * cIntRetinaFactor(), s = st::msgShadow * cIntRetinaFactor();
		QImage rect(r * 3, r * 3 + (shadow ? s : 0), QImage::Format_ARGB32_Premultiplied), localCors[4];
		{
			Painter p(&rect);
			PainterHighQualityEnabler hq(p);

			p.setCompositionMode(QPainter::CompositionMode_Source);
			p.fillRect(QRect(0, 0, rect.width(), rect.height()), Qt::transparent);
			p.setCompositionMode(QPainter::CompositionMode_SourceOver);
			p.setPen(Qt::NoPen);
			if (shadow) {
				p.setBrush((*shadow)->b);
				p.drawRoundedRect(0, s, r * 3, r * 3, r, r);
			}
			p.setBrush(brush);
			p.drawRoundedRect(0, 0, r * 3, r * 3, r, r);
		}
		if (!cors) cors = localCors;
		cors[0] = rect.copy(0, 0, r, r);
		cors[1] = rect.copy(r * 2, 0, r, r);
		cors[2] = rect.copy(0, r * 2, r, r + (shadow ? s : 0));
		cors[3] = rect.copy(r * 2, r * 2, r, r + (shadow ? s : 0));
		if (index != SmallMaskCorners && index != LargeMaskCorners) {
			for (int i = 0; i < 4; ++i) {
				::corners[index].p[i] = pixmapFromImageInPlace(std::move(cors[i]));
				::corners[index].p[i].setDevicePixelRatio(cRetinaFactor());
			}
		}
	}

	void tryFontFamily(QString &family, const QString &tryFamily) {
		if (family.isEmpty()) {
			if (!QFontInfo(QFont(tryFamily)).family().trimmed().compare(tryFamily, Qt::CaseInsensitive)) {
				family = tryFamily;
			}
		}
	}

	void createMaskCorners() {
		QImage mask[4];
		prepareCorners(SmallMaskCorners, st::buttonRadius, QColor(255, 255, 255), nullptr, mask);
		for (int i = 0; i < 4; ++i) {
			::cornersMaskSmall[i] = mask[i].convertToFormat(QImage::Format_ARGB32_Premultiplied);
			::cornersMaskSmall[i].setDevicePixelRatio(cRetinaFactor());
		}
		prepareCorners(LargeMaskCorners, st::historyMessageRadius, QColor(255, 255, 255), nullptr, mask);
		for (int i = 0; i < 4; ++i) {
			::cornersMaskLarge[i] = mask[i].convertToFormat(QImage::Format_ARGB32_Premultiplied);
			::cornersMaskLarge[i].setDevicePixelRatio(cRetinaFactor());
		}
	}

	void createPaletteCorners() {
		prepareCorners(MenuCorners, st::buttonRadius, st::menuBg);
		prepareCorners(BoxCorners, st::boxRadius, st::boxBg);
		prepareCorners(BotKbOverCorners, st::dateRadius, st::msgBotKbOverBgAdd);
		prepareCorners(StickerCorners, st::dateRadius, st::msgServiceBg);
		prepareCorners(StickerSelectedCorners, st::dateRadius, st::msgServiceBgSelected);
		prepareCorners(SelectedOverlaySmallCorners, st::buttonRadius, st::msgSelectOverlay);
		prepareCorners(SelectedOverlayLargeCorners, st::historyMessageRadius, st::msgSelectOverlay);
		prepareCorners(DateCorners, st::dateRadius, st::msgDateImgBg);
		prepareCorners(DateSelectedCorners, st::dateRadius, st::msgDateImgBgSelected);
		prepareCorners(OverviewVideoCorners, st::overviewVideoStatusRadius, st::msgDateImgBg);
		prepareCorners(OverviewVideoSelectedCorners, st::overviewVideoStatusRadius, st::msgDateImgBgSelected);
		prepareCorners(InShadowCorners, st::historyMessageRadius, st::msgInShadow);
		prepareCorners(InSelectedShadowCorners, st::historyMessageRadius, st::msgInShadowSelected);
		prepareCorners(ForwardCorners, st::historyMessageRadius, st::historyForwardChooseBg);
		prepareCorners(MediaviewSaveCorners, st::mediaviewControllerRadius, st::mediaviewSaveMsgBg);
		prepareCorners(EmojiHoverCorners, st::buttonRadius, st::emojiPanHover);
		prepareCorners(StickerHoverCorners, st::buttonRadius, st::emojiPanHover);
		prepareCorners(BotKeyboardCorners, st::buttonRadius, st::botKbBg);
		prepareCorners(PhotoSelectOverlayCorners, st::buttonRadius, st::overviewPhotoSelectOverlay);

		prepareCorners(Doc1Corners, st::buttonRadius, st::msgFile1Bg);
		prepareCorners(Doc2Corners, st::buttonRadius, st::msgFile2Bg);
		prepareCorners(Doc3Corners, st::buttonRadius, st::msgFile3Bg);
		prepareCorners(Doc4Corners, st::buttonRadius, st::msgFile4Bg);

		prepareCorners(MessageInCorners, st::historyMessageRadius, st::msgInBg, &st::msgInShadow);
		prepareCorners(MessageInSelectedCorners, st::historyMessageRadius, st::msgInBgSelected, &st::msgInShadowSelected);
		prepareCorners(MessageOutCorners, st::historyMessageRadius, st::msgOutBg, &st::msgOutShadow);
		prepareCorners(MessageOutSelectedCorners, st::historyMessageRadius, st::msgOutBgSelected, &st::msgOutShadowSelected);
	}

	void createCorners() {
		::corners.resize(RoundCornersCount);
		createMaskCorners();
		createPaletteCorners();
	}

	void clearCorners() {
		::corners.clear();
		::cornersMap.clear();
	}

	void initMedia() {
		if (!::monofont) {
			QString family;
			tryFontFamily(family, qsl("Consolas"));
			tryFontFamily(family, qsl("Liberation Mono"));
			tryFontFamily(family, qsl("Menlo"));
			tryFontFamily(family, qsl("Courier"));
			if (family.isEmpty()) family = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
			::monofont = style::font(st::normalFont->f.pixelSize(), 0, family);
		}

		createCorners();

		using Update = Window::Theme::BackgroundUpdate;
		static auto subscription = Window::Theme::Background()->add_subscription([](const Update &update) {
			if (update.paletteChanged()) {
				createPaletteCorners();

				if (App::main()) {
					App::main()->updateScrollColors();
				}
				HistoryView::serviceColorsUpdated();
			} else if (update.type == Update::Type::New) {
				prepareCorners(StickerCorners, st::dateRadius, st::msgServiceBg);
				prepareCorners(StickerSelectedCorners, st::dateRadius, st::msgServiceBgSelected);

				if (App::main()) {
					App::main()->updateScrollColors();
				}
				HistoryView::serviceColorsUpdated();
			}
		});
	}

	void deinitMedia() {
		clearCorners();

		Data::clearGlobalStructures();

		Images::ClearAll();
	}

	void hoveredItem(HistoryView::Element *item) {
		::hoveredItem = item;
	}

	HistoryView::Element *hoveredItem() {
		return ::hoveredItem;
	}

	void pressedItem(HistoryView::Element *item) {
		::pressedItem = item;
	}

	HistoryView::Element *pressedItem() {
		return ::pressedItem;
	}

	void hoveredLinkItem(HistoryView::Element *item) {
		::hoveredLinkItem = item;
	}

	HistoryView::Element *hoveredLinkItem() {
		return ::hoveredLinkItem;
	}

	void pressedLinkItem(HistoryView::Element *item) {
		::pressedLinkItem = item;
	}

	HistoryView::Element *pressedLinkItem() {
		return ::pressedLinkItem;
	}

	void mousedItem(HistoryView::Element *item) {
		::mousedItem = item;
	}

	HistoryView::Element *mousedItem() {
		return ::mousedItem;
	}

	void clearMousedItems() {
		hoveredItem(nullptr);
		pressedItem(nullptr);
		hoveredLinkItem(nullptr);
		pressedLinkItem(nullptr);
		mousedItem(nullptr);
	}

	const style::font &monofont() {
		return ::monofont;
	}

	void quit() {
		if (quitting()) {
			return;
		} else if (AuthSession::Exists()
			&& Auth().data().exportInProgress()) {
			Auth().data().stopExportWithConfirmation([] { App::quit(); });
			return;
		}
		setLaunchState(QuitRequested);

		if (auto window = App::wnd()) {
			if (!Core::Sandbox::Instance().isSavingSession()) {
				window->hide();
			}
		}
		Core::Application::QuitAttempt();
	}

	bool quitting() {
		return _launchState != Launched;
	}

	LaunchState launchState() {
		return _launchState;
	}

	void setLaunchState(LaunchState state) {
		_launchState = state;
	}

	void restart() {
		using namespace Core;
		const auto updateReady = !UpdaterDisabled()
			&& (UpdateChecker().state() == UpdateChecker::State::Ready);
		if (updateReady) {
			cSetRestartingUpdate(true);
		} else {
			cSetRestarting(true);
			cSetRestartingToSettings(true);
		}
		App::quit();
	}

	QImage readImage(QByteArray data, QByteArray *format, bool opaque, bool *animated) {
        QByteArray tmpFormat;
		QImage result;
		QBuffer buffer(&data);
        if (!format) {
            format = &tmpFormat;
        }
		{
			QImageReader reader(&buffer, *format);
#ifndef OS_MAC_OLD
			reader.setAutoTransform(true);
#endif // OS_MAC_OLD
			if (animated) *animated = reader.supportsAnimation() && reader.imageCount() > 1;
			QByteArray fmt = reader.format();
			if (!fmt.isEmpty()) *format = fmt;
			if (!reader.read(&result)) {
				return QImage();
			}
			fmt = reader.format();
			if (!fmt.isEmpty()) *format = fmt;
		}
		buffer.seek(0);
		auto fmt = QString::fromUtf8(*format).toLower();
		if (fmt == "jpg" || fmt == "jpeg") {
#ifdef OS_MAC_OLD
			if (auto exifData = exif_data_new_from_data((const uchar*)(data.constData()), data.size())) {
				auto byteOrder = exif_data_get_byte_order(exifData);
				if (auto exifEntry = exif_data_get_entry(exifData, EXIF_TAG_ORIENTATION)) {
					auto orientationFix = [exifEntry, byteOrder] {
						auto orientation = exif_get_short(exifEntry->data, byteOrder);
						switch (orientation) {
						case 2: return QTransform(-1, 0, 0, 1, 0, 0);
						case 3: return QTransform(-1, 0, 0, -1, 0, 0);
						case 4: return QTransform(1, 0, 0, -1, 0, 0);
						case 5: return QTransform(0, -1, -1, 0, 0, 0);
						case 6: return QTransform(0, 1, -1, 0, 0, 0);
						case 7: return QTransform(0, 1, 1, 0, 0, 0);
						case 8: return QTransform(0, -1, 1, 0, 0, 0);
						}
						return QTransform();
					};
					result = result.transformed(orientationFix());
				}
				exif_data_free(exifData);
			}
#endif // OS_MAC_OLD
		} else if (opaque) {
			result = Images::prepareOpaque(std::move(result));
		}
		return result;
	}

	QImage readImage(const QString &file, QByteArray *format, bool opaque, bool *animated, QByteArray *content) {
		QFile f(file);
		if (f.size() > kImageSizeLimit || !f.open(QIODevice::ReadOnly)) {
			if (animated) *animated = false;
			return QImage();
		}
		auto imageBytes = f.readAll();
		auto result = readImage(imageBytes, format, opaque, animated);
		if (content && !result.isNull()) {
			*content = imageBytes;
		}
		return result;
	}

	QPixmap pixmapFromImageInPlace(QImage &&image) {
		return QPixmap::fromImage(std::move(image), Qt::ColorOnly);
	}

	void rectWithCorners(Painter &p, QRect rect, const style::color &bg, RoundCorners index, RectParts corners) {
		auto parts = RectPart::Top
			| RectPart::NoTopBottom
			| RectPart::Bottom
			| corners;
		roundRect(p, rect, bg, index, nullptr, parts);
		if ((corners & RectPart::AllCorners) != RectPart::AllCorners) {
			const auto size = ::corners[index].p[0].width() / cIntRetinaFactor();
			if (!(corners & RectPart::TopLeft)) {
				p.fillRect(rect.x(), rect.y(), size, size, bg);
			}
			if (!(corners & RectPart::TopRight)) {
				p.fillRect(rect.x() + rect.width() - size, rect.y(), size, size, bg);
			}
			if (!(corners & RectPart::BottomLeft)) {
				p.fillRect(rect.x(), rect.y() + rect.height() - size, size, size, bg);
			}
			if (!(corners & RectPart::BottomRight)) {
				p.fillRect(rect.x() + rect.width() - size, rect.y() + rect.height() - size, size, size, bg);
			}
		}
	}

	void complexOverlayRect(Painter &p, QRect rect, ImageRoundRadius radius, RectParts corners) {
		if (radius == ImageRoundRadius::Ellipse) {
			PainterHighQualityEnabler hq(p);
			p.setPen(Qt::NoPen);
			p.setBrush(p.textPalette().selectOverlay);
			p.drawEllipse(rect);
		} else {
			auto overlayCorners = (radius == ImageRoundRadius::Small)
				? SelectedOverlaySmallCorners
				: SelectedOverlayLargeCorners;
			const auto bg = p.textPalette().selectOverlay;
			rectWithCorners(p, rect, bg, overlayCorners, corners);
		}
	}

	void complexLocationRect(Painter &p, QRect rect, ImageRoundRadius radius, RectParts corners) {
		rectWithCorners(p, rect, st::msgInBg, MessageInCorners, corners);
	}

	QImage *cornersMask(ImageRoundRadius radius) {
		switch (radius) {
		case ImageRoundRadius::Large: return ::cornersMaskLarge;
		case ImageRoundRadius::Small:
		default: break;
		}
		return ::cornersMaskSmall;
	}

	void roundRect(Painter &p, int32 x, int32 y, int32 w, int32 h, style::color bg, const CornersPixmaps &corner, const style::color *shadow, RectParts parts) {
		auto cornerWidth = corner.p[0].width() / cIntRetinaFactor();
		auto cornerHeight = corner.p[0].height() / cIntRetinaFactor();
		if (w < 2 * cornerWidth || h < 2 * cornerHeight) return;
		if (w > 2 * cornerWidth) {
			if (parts & RectPart::Top) {
				p.fillRect(x + cornerWidth, y, w - 2 * cornerWidth, cornerHeight, bg);
			}
			if (parts & RectPart::Bottom) {
				p.fillRect(x + cornerWidth, y + h - cornerHeight, w - 2 * cornerWidth, cornerHeight, bg);
				if (shadow) {
					p.fillRect(x + cornerWidth, y + h, w - 2 * cornerWidth, st::msgShadow, *shadow);
				}
			}
		}
		if (h > 2 * cornerHeight) {
			if ((parts & RectPart::NoTopBottom) == RectPart::NoTopBottom) {
				p.fillRect(x, y + cornerHeight, w, h - 2 * cornerHeight, bg);
			} else {
				if (parts & RectPart::Left) {
					p.fillRect(x, y + cornerHeight, cornerWidth, h - 2 * cornerHeight, bg);
				}
				if ((parts & RectPart::Center) && w > 2 * cornerWidth) {
					p.fillRect(x + cornerWidth, y + cornerHeight, w - 2 * cornerWidth, h - 2 * cornerHeight, bg);
				}
				if (parts & RectPart::Right) {
					p.fillRect(x + w - cornerWidth, y + cornerHeight, cornerWidth, h - 2 * cornerHeight, bg);
				}
			}
		}
		if (parts & RectPart::TopLeft) {
			p.drawPixmap(x, y, corner.p[0]);
		}
		if (parts & RectPart::TopRight) {
			p.drawPixmap(x + w - cornerWidth, y, corner.p[1]);
		}
		if (parts & RectPart::BottomLeft) {
			p.drawPixmap(x, y + h - cornerHeight, corner.p[2]);
		}
		if (parts & RectPart::BottomRight) {
			p.drawPixmap(x + w - cornerWidth, y + h - cornerHeight, corner.p[3]);
		}
	}

	void roundRect(Painter &p, int32 x, int32 y, int32 w, int32 h, style::color bg, RoundCorners index, const style::color *shadow, RectParts parts) {
		roundRect(p, x, y, w, h, bg, ::corners[index], shadow, parts);
	}

	void roundShadow(Painter &p, int32 x, int32 y, int32 w, int32 h, style::color shadow, RoundCorners index, RectParts parts) {
		auto &corner = ::corners[index];
		auto cornerWidth = corner.p[0].width() / cIntRetinaFactor();
		auto cornerHeight = corner.p[0].height() / cIntRetinaFactor();
		if (parts & RectPart::Bottom) {
			p.fillRect(x + cornerWidth, y + h, w - 2 * cornerWidth, st::msgShadow, shadow);
		}
		if (parts & RectPart::BottomLeft) {
			p.fillRect(x, y + h - cornerHeight, cornerWidth, st::msgShadow, shadow);
			p.drawPixmap(x, y + h - cornerHeight + st::msgShadow, corner.p[2]);
		}
		if (parts & RectPart::BottomRight) {
			p.fillRect(x + w - cornerWidth, y + h - cornerHeight, cornerWidth, st::msgShadow, shadow);
			p.drawPixmap(x + w - cornerWidth, y + h - cornerHeight + st::msgShadow, corner.p[3]);
		}
	}

	void roundRect(Painter &p, int32 x, int32 y, int32 w, int32 h, style::color bg, ImageRoundRadius radius, RectParts parts) {
		auto colorKey = ((uint32(bg->c.alpha()) & 0xFF) << 24) | ((uint32(bg->c.red()) & 0xFF) << 16) | ((uint32(bg->c.green()) & 0xFF) << 8) | ((uint32(bg->c.blue()) & 0xFF) << 24);
		auto i = cornersMap.find(colorKey);
		if (i == cornersMap.cend()) {
			QImage images[4];
			switch (radius) {
			case ImageRoundRadius::Small: prepareCorners(SmallMaskCorners, st::buttonRadius, bg, nullptr, images); break;
			case ImageRoundRadius::Large: prepareCorners(LargeMaskCorners, st::historyMessageRadius, bg, nullptr, images); break;
			default: p.fillRect(x, y, w, h, bg); return;
			}

			CornersPixmaps pixmaps;
			for (int j = 0; j < 4; ++j) {
				pixmaps.p[j] = pixmapFromImageInPlace(std::move(images[j]));
				pixmaps.p[j].setDevicePixelRatio(cRetinaFactor());
			}
			i = cornersMap.insert(colorKey, pixmaps);
		}
		roundRect(p, x, y, w, h, bg, i.value(), nullptr, parts);
	}

}
