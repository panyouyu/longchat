/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "boxes/peer_list_controllers.h"

#include "boxes/confirm_box.h"
#include "boxes/handle_friend_request_box.h"
#include "observer_peer.h"
#include "ui/widgets/checkbox.h"
#include "ui/image/image.h"
#include "ui/text_options.h"
#include "ui/effects/ripple_animation.h"
#include "auth_session.h"
#include "data/data_session.h"
#include "data/data_channel.h"
#include "data/data_chat.h"
#include "data/data_user.h"
#include "apiwrap.h"
#include "mainwidget.h"
#include "lang/lang_keys.h"
#include "history/history.h"
#include "dialogs/dialogs_indexed_list.h"
#include "styles/style_boxes.h"
#include "styles/style_profile.h"
#include "styles/style_window.h"
#include "styles/style_widgets.h"

namespace {
constexpr auto kUserpicSize = 160;

void ShareBotGame(not_null<UserData*> bot, not_null<PeerData*> chat) {
	const auto history = chat->owner().historyLoaded(chat);
	const auto randomId = rand_value<uint64>();
	const auto requestId = MTP::send(
		MTPmessages_SendMedia(
			MTP_flags(0),
			chat->input,
			MTP_int(0),
			MTP_inputMediaGame(
				MTP_inputGameShortName(
					bot->inputUser,
					MTP_string(bot->botInfo->shareGameShortName))),
			MTP_string(""),
			MTP_long(randomId),
			MTPReplyMarkup(),
			MTPnullEntities),
		App::main()->rpcDone(&MainWidget::sentUpdatesReceived),
		App::main()->rpcFail(&MainWidget::sendMessageFail),
		0,
		0,
		history ? history->sendRequestId : 0);
	if (history) {
		history->sendRequestId = requestId;
	}
	Ui::hideLayer();
	Ui::showPeerHistory(chat, ShowAtUnreadMsgId);
}

void AddBotToGroup(not_null<UserData*> bot, not_null<PeerData*> chat) {
	if (bot->botInfo && !bot->botInfo->startGroupToken.isEmpty()) {
		Auth().api().sendBotStart(bot, chat);
	} else {
		Auth().api().addChatParticipants(chat, { 1, bot });
	}
	Ui::hideLayer();
	Ui::showPeerHistory(chat, ShowAtUnreadMsgId);
}

} // namespace

// Not used for now.
//
//MembersAddButton::MembersAddButton(QWidget *parent, const style::TwoIconButton &st) : RippleButton(parent, st.ripple)
//, _st(st) {
//	resize(_st.width, _st.height);
//	setCursor(style::cur_pointer);
//}
//
//void MembersAddButton::paintEvent(QPaintEvent *e) {
//	Painter p(this);
//
//	auto ms = crl::now();
//	auto over = isOver();
//	auto down = isDown();
//
//	((over || down) ? _st.iconBelowOver : _st.iconBelow).paint(p, _st.iconPosition, width());
//	paintRipple(p, _st.rippleAreaPosition.x(), _st.rippleAreaPosition.y(), ms);
//	((over || down) ? _st.iconAboveOver : _st.iconAbove).paint(p, _st.iconPosition, width());
//}
//
//QImage MembersAddButton::prepareRippleMask() const {
//	return Ui::RippleAnimation::ellipseMask(QSize(_st.rippleAreaSize, _st.rippleAreaSize));
//}
//
//QPoint MembersAddButton::prepareRippleStartPosition() const {
//	return mapFromGlobal(QCursor::pos()) - _st.rippleAreaPosition;
//}

void PeerListRowWithLink::setActionLink(const QString &action) {
	_action = action;
	refreshActionLink();
}

void PeerListRowWithLink::refreshActionLink() {
	if (!isInitialized()) return;
	_actionWidth = _action.isEmpty() ? 0 : st::normalFont->width(_action);
}

void PeerListRowWithLink::lazyInitialize(const style::PeerListItem &st) {
	PeerListRow::lazyInitialize(st);
	refreshActionLink();
}

QSize PeerListRowWithLink::actionSize() const {
	return QSize(_actionWidth, st::normalFont->height);
}

QMargins PeerListRowWithLink::actionMargins() const {
	return QMargins(
		st::contactsCheckPosition.x(),
		(st::contactsPadding.top() + st::contactsPhotoSize + st::contactsPadding.bottom() - st::normalFont->height) / 2,
		st::defaultPeerListItem.photoPosition.x() + st::contactsCheckPosition.x(),
		0);
}

void PeerListRowWithLink::paintAction(
		Painter &p,
		int x,
		int y,
		int outerWidth,
		bool selected,
		bool actionSelected) {
	p.setFont(actionSelected ? st::linkOverFont : st::linkFont);
	p.setPen(actionSelected ? st::defaultLinkButton.overColor : st::defaultLinkButton.color);
	p.drawTextLeft(x, y, outerWidth, _action, _actionWidth);
}

PeerListGlobalSearchController::PeerListGlobalSearchController() {
	_timer.setCallback([this] { searchOnServer(); });
}

void PeerListGlobalSearchController::searchQuery(const QString &query) {
	if (_query != query) {
		_query = query;
		_requestId = 0;
		if (!_query.isEmpty() && !searchInCache()) {
			_timer.callOnce(AutoSearchTimeout);
		} else {
			_timer.cancel();
		}
	}
}

bool PeerListGlobalSearchController::searchInCache() {
	auto it = _cache.find(_query);
	if (it != _cache.cend()) {
		_requestId = 0;
		searchDone(it->second, _requestId);
		return true;
	}
	return false;
}

void PeerListGlobalSearchController::searchOnServer() {
	_requestId = request(MTPcontacts_Search(
		MTP_string(_query),
		MTP_int(SearchPeopleLimit)
	)).done([=](const MTPcontacts_Found &result, mtpRequestId requestId) {
		searchDone(result, requestId);
	}).fail([=](const RPCError &error, mtpRequestId requestId) {
		if (_requestId == requestId) {
			_requestId = 0;
			delegate()->peerListSearchRefreshRows();
		}
	}).send();
	_queries.emplace(_requestId, _query);
}

void PeerListGlobalSearchController::searchDone(
		const MTPcontacts_Found &result,
		mtpRequestId requestId) {
	Expects(result.type() == mtpc_contacts_found);

	auto &contacts = result.c_contacts_found();
	auto query = _query;
	if (requestId) {
		Auth().data().processUsers(contacts.vusers);
		Auth().data().processChats(contacts.vchats);
		auto it = _queries.find(requestId);
		if (it != _queries.cend()) {
			query = it->second;
			_cache[query] = result;
			_queries.erase(it);
		}
	}
	const auto feedList = [&](const MTPVector<MTPPeer> &list) {
		for (const auto &mtpPeer : list.v) {
			if (const auto peer = Auth().data().peerLoaded(peerFromMTP(mtpPeer))) {
				delegate()->peerListSearchAddRow(peer);
			}
		}
	};
	if (_requestId == requestId) {
		_requestId = 0;
		feedList(contacts.vmy_results);
		feedList(contacts.vresults);
		delegate()->peerListSearchRefreshRows();
	}
}

bool PeerListGlobalSearchController::isLoading() {
	return _timer.isActive() || _requestId;
}

ChatsListBoxController::Row::Row(not_null<History*> history)
: PeerListRow(history->peer)
, _history(history) {
}

ChatsListBoxController::ChatsListBoxController(
	std::unique_ptr<PeerListSearchController> searchController)
: PeerListController(std::move(searchController)) {
}

void ChatsListBoxController::prepare() {
	setSearchNoResultsText(lang(lng_blocked_list_not_found));
	delegate()->peerListSetSearchMode(PeerListSearchMode::Enabled);

	prepareViewHook();

	rebuildRows();

	auto &sessionData = Auth().data();
	subscribe(sessionData.contactsLoaded(), [this](bool loaded) {
		rebuildRows();
	});
	subscribe(sessionData.moreChatsLoaded(), [this] {
		rebuildRows();
	});
	subscribe(sessionData.allChatsLoaded(), [this](bool loaded) {
		checkForEmptyRows();
	});
}

void ChatsListBoxController::rebuildRows() {
	auto wasEmpty = !delegate()->peerListFullRowsCount();
	auto appendList = [this](auto chats) {
		auto count = 0;
		for (const auto row : chats->all()) {
			if (const auto history = row->history()) {
				if (appendRow(history)) {
					++count;
				}
			}
		}
		return count;
	};
	auto added = 0;
	if (respectSavedMessagesChat()) {
		if (appendRow(Auth().data().history(Auth().user()))) {
			++added;
		}
	}
	added += appendList(App::main()->dialogsList());
	added += appendList(App::main()->contactsNoDialogsList());
	if (!wasEmpty && added > 0) {
		// Place dialogs list before contactsNoDialogs list.
		delegate()->peerListPartitionRows([](const PeerListRow &a) {
			auto history = static_cast<const Row&>(a).history();
			return history->inChatList(Dialogs::Mode::All);
		});
		if (respectSavedMessagesChat()) {
			delegate()->peerListPartitionRows([](const PeerListRow &a) {
				return a.peer()->isSelf();
			});
		}
	}
	checkForEmptyRows();
	delegate()->peerListRefreshRows();
}

void ChatsListBoxController::checkForEmptyRows() {
	if (delegate()->peerListFullRowsCount()) {
		setDescriptionText(QString());
	} else {
		auto &sessionData = Auth().data();
		auto loaded = sessionData.contactsLoaded().value() && sessionData.allChatsLoaded().value();
		setDescriptionText(loaded ? emptyBoxText() : lang(lng_contacts_loading));
	}
}

QString ChatsListBoxController::emptyBoxText() const {
	return lang(lng_contacts_not_found);
}

std::unique_ptr<PeerListRow> ChatsListBoxController::createSearchRow(not_null<PeerData*> peer) {
	return createRow(peer->owner().history(peer));
}

bool ChatsListBoxController::appendRow(not_null<History*> history) {
	if (auto row = delegate()->peerListFindRow(history->peer->id)) {
		updateRowHook(static_cast<Row*>(row));
		return false;
	}
	if (auto row = createRow(history)) {
		delegate()->peerListAppendRow(std::move(row));
		return true;
	}
	return false;
}

class ContactsBoxController::NewFriendRow : public PeerListRow {
public:
	NewFriendRow(not_null<ContactsBoxController*> controller) 
		: PeerListRow(Auth().user())
	, _controller(controller) {
		setCustomStatus(QString());
		auto image = QImage(qsl(":/gui/art/new_friend.png")).scaledToWidth(
			kUserpicSize,
			Qt::SmoothTransformation);
		_pic = Images::Create(std::move(image), "PNG");
		_name.setText(st::newFriendTextStyle, lang(lng_friend_request_new), Ui::NameTextOptions());
		for(const auto &item : Auth().data().friendRequests()) {
			_unreadCount += (item->verifyStatus() == UserData::VerifyStatus::UnDeal);
		}
		Auth().data().friendRequestChanged(
		) | rpl::start_with_next([=](auto count) {
			_unreadCount = count;
			_controller->delegate()->peerListUpdateRow(this);
		}, _lifetime);
	}
	void paintUserpic(
		Painter& p,
		const style::PeerListItem& st,
		int x,
		int y,
		int outerWidth) {
		auto size = st.photoSize;
		p.drawPixmap(x, y, _pic->pixRounded(Data::FileOriginPeerPhoto(Auth().userPeerId()), size, size, ImageRoundRadius::Small));
	}
	bool isSpecialRow() const {
		return true;
	}
	void paintSpecialRow(
		Painter& p,
		const style::PeerListItem& st,
		int x,
		int y,
		int width,
		int outerWidth) {
		auto namew = width;
		if (_unreadCount > 0) {
			QString cnt = (_unreadCount < 1000) ? QString("%1").arg(_unreadCount) : QString("..%1").arg(_unreadCount % 100, 2, 10, QChar('0'));
			QImage result(size, size, QImage::Format_ARGB32);
			int32 cntSize = cnt.size();
			result.fill(Qt::transparent);
			{
				QPainter p(&result);
				p.setBrush(bg);
				p.setPen(Qt::NoPen);
				p.setRenderHint(QPainter::Antialiasing);
				int32 fontSize;
				if (size == 16) {
					fontSize = (cntSize < 2) ? 11 : ((cntSize < 3) ? 11 : 8);
				} else if (size == 20) {
					fontSize = (cntSize < 2) ? 14 : ((cntSize < 3) ? 13 : 10);
				} else if (size == 24) {
					fontSize = (cntSize < 2) ? 17 : ((cntSize < 3) ? 16 : 12);
				} else {
					fontSize = (cntSize < 2) ? 22 : ((cntSize < 3) ? 20 : 16);
				}
				style::font f = { fontSize, 0, 0 };
				int32 w = f->width(cnt), d, r;
				if (size == 16) {
					d = (cntSize < 2) ? 5 : ((cntSize < 3) ? 2 : 1);
					r = (cntSize < 2) ? 8 : ((cntSize < 3) ? 7 : 3);
				} else if (size == 20) {
					d = (cntSize < 2) ? 6 : ((cntSize < 3) ? 2 : 1);
					r = (cntSize < 2) ? 10 : ((cntSize < 3) ? 9 : 5);
				} else if (size == 24) {
					d = (cntSize < 2) ? 7 : ((cntSize < 3) ? 3 : 1);
					r = (cntSize < 2) ? 12 : ((cntSize < 3) ? 11 : 6);
				} else {
					d = (cntSize < 2) ? 9 : ((cntSize < 3) ? 4 : 2);
					r = (cntSize < 2) ? 16 : ((cntSize < 3) ? 14 : 8);
				}
				p.drawRoundedRect(QRect(size - w - d * 2, size - f->height, w + d * 2, f->height), r, r);
				p.setFont(f);

				p.setPen(fg);

				p.drawText(size - w - d, size - f->height + f->ascent, cnt);
			}
			if (_unreadCount) {
				int unread_x = x + width - result.width() - st::mainMenuUnReadCountPadding;
				int unread_y = y + 10;
				p.drawImage(unread_x, unread_y, result);
				namew = unread_x - x;
			}			
		}


		
		p.setPen(st::contactsNameFg);
		_name.drawLeftElided(p, x, y, namew, outerWidth);
	}
private:
	not_null<ContactsBoxController*> _controller;
	ImagePtr _pic;
	Text _name;
	int _unreadCount = 0;
	const int size = 20;
	const style::color& bg = st::trayCounterBg;
	const style::color& fg = st::trayCounterFg;
	rpl::lifetime _lifetime;
};

ContactsBoxController::ContactsBoxController(
	std::unique_ptr<PeerListSearchController> searchController)
: PeerListController(std::move(searchController)) {
}

void ContactsBoxController::prepare() {
	setSearchNoResultsText(lang(lng_blocked_list_not_found));
	delegate()->peerListSetSearchMode(PeerListSearchMode::Enabled);
	delegate()->peerListSetTitle(langFactory(lng_contacts_header));

	prepareViewHook();

	rebuildRows();

	auto &sessionData = Auth().data();
	subscribe(sessionData.contactsLoaded(), [this](bool loaded) {
		rebuildRows();
	});
}

void ContactsBoxController::rebuildRows() {
	auto appendList = [this](auto chats) {
		auto count = 0;
		for (const auto row : chats->all()) {
			if (const auto history = row->history()) {
				if (const auto user = history->peer->asUser()) {
					if (appendRow(user)) {
						++count;
					}
				}
			}
		}
		return count;
	};
	appendRow(Auth().user());
	appendList(App::main()->contactsList());
	checkForEmptyRows();
	delegate()->peerListRefreshRows();
}

void ContactsBoxController::checkForEmptyRows() {
	if (delegate()->peerListFullRowsCount()) {
		setDescriptionText(QString());
	} else {
		auto &sessionData = Auth().data();
		auto loaded = sessionData.contactsLoaded().value();
		setDescriptionText(lang(loaded ? lng_contacts_not_found : lng_contacts_loading));
	}
}

std::unique_ptr<PeerListRow> ContactsBoxController::createSearchRow(
		not_null<PeerData*> peer) {
	if (const auto user = peer->asUser()) {
		return createRow(user);
	}
	return nullptr;
}

void ContactsBoxController::rowClicked(not_null<PeerListRow*> row) {
	if (row->peer()->isSelf()) {
		Ui::show(Box<PeerListBox>(std::make_unique<FriendRequestBoxController>(), [](not_null<PeerListBox*> box) {
			box->addButton(langFactory(lng_close), [box] { box->closeBox(); });
		}), LayerOption::KeepOther);
	} else {
		Ui::showPeerHistory(row->peer(), ShowAtUnreadMsgId);
	}
	
}

bool ContactsBoxController::appendRow(not_null<UserData*> user) {
	if (auto row = delegate()->peerListFindRow(user->id)) {
		updateRowHook(row);
		return false;
	}
	if (auto row = createRow(user)) {
		delegate()->peerListAppendRow(std::move(row));
		return true;
	}
	return false;
}

std::unique_ptr<PeerListRow> ContactsBoxController::createRow(not_null<UserData*> user) {
	if (user->isSelf()) {
		return std::make_unique<NewFriendRow>(this);
	}
	return std::make_unique<PeerListRow>(user);
}

void AddBotToGroupBoxController::Start(not_null<UserData*> bot) {
	auto initBox = [=](not_null<PeerListBox*> box) {
		box->addButton(langFactory(lng_cancel), [box] { box->closeBox(); });
	};
	Ui::show(Box<PeerListBox>(std::make_unique<AddBotToGroupBoxController>(bot), std::move(initBox)));
}

AddBotToGroupBoxController::AddBotToGroupBoxController(not_null<UserData*> bot)
: ChatsListBoxController(SharingBotGame(bot)
	? std::make_unique<PeerListGlobalSearchController>()
	: nullptr)
, _bot(bot) {
}

void AddBotToGroupBoxController::rowClicked(not_null<PeerListRow*> row) {
	if (sharingBotGame()) {
		shareBotGame(row->peer());
	} else {
		addBotToGroup(row->peer());
	}
}

void AddBotToGroupBoxController::shareBotGame(not_null<PeerData*> chat) {
	auto send = crl::guard(this, [bot = _bot, chat] {
		ShareBotGame(bot, chat);
	});
	auto confirmText = [chat] {
		if (chat->isUser()) {
			return lng_bot_sure_share_game(lt_user, App::peerName(chat));
		}
		return lng_bot_sure_share_game_group(lt_group, chat->name);
	}();
	Ui::show(
		Box<ConfirmBox>(confirmText, std::move(send)),
		LayerOption::KeepOther);
}

void AddBotToGroupBoxController::addBotToGroup(not_null<PeerData*> chat) {
	if (const auto megagroup = chat->asMegagroup()) {
		if (!megagroup->canAddMembers()) {
			Ui::show(
				Box<InformBox>(lang(lng_error_cant_add_member)),
				LayerOption::KeepOther);
			return;
		}
	}
	auto send = crl::guard(this, [bot = _bot, chat] {
		AddBotToGroup(bot, chat);
	});
	auto confirmText = lng_bot_sure_invite(lt_group, chat->name);
	Ui::show(
		Box<ConfirmBox>(confirmText, send),
		LayerOption::KeepOther);
}

auto AddBotToGroupBoxController::createRow(not_null<History*> history)
-> std::unique_ptr<ChatsListBoxController::Row> {
	if (!needToCreateRow(history->peer)) {
		return nullptr;
	}
	return std::make_unique<Row>(history);
}

bool AddBotToGroupBoxController::needToCreateRow(
		not_null<PeerData*> peer) const {
	if (sharingBotGame()) {
		if (!peer->canWrite()
			|| peer->amRestricted(ChatRestriction::f_send_games)) {
			return false;
		}
		return true;
	}
	if (const auto chat = peer->asChat()) {
		return chat->canAddMembers();
	} else if (const auto group = peer->asMegagroup()) {
		return group->canAddMembers();
	}
	return false;
}

bool AddBotToGroupBoxController::SharingBotGame(not_null<UserData*> bot) {
	auto &info = bot->botInfo;
	return (info && !info->shareGameShortName.isEmpty());
}

bool AddBotToGroupBoxController::sharingBotGame() const {
	return SharingBotGame(_bot);
}

QString AddBotToGroupBoxController::emptyBoxText() const {
	return lang(Auth().data().allChatsLoaded().value()
		? (sharingBotGame() ? lng_bot_no_chats : lng_bot_no_groups)
		: lng_contacts_loading);
}

QString AddBotToGroupBoxController::noResultsText() const {
	return lang(Auth().data().allChatsLoaded().value()
		? (sharingBotGame() ? lng_bot_chats_not_found : lng_bot_groups_not_found)
		: lng_contacts_loading);
}

void AddBotToGroupBoxController::updateLabels() {
	setSearchNoResultsText(noResultsText());
}

void AddBotToGroupBoxController::prepareViewHook() {
	delegate()->peerListSetTitle(langFactory(sharingBotGame()
		? lng_bot_choose_chat
		: lng_bot_choose_group));
	updateLabels();
	subscribe(Auth().data().allChatsLoaded(), [this](bool) { updateLabels(); });
}

ChooseRecipientBoxController::ChooseRecipientBoxController(
	FnMut<void(not_null<PeerData*>)> callback)
: _callback(std::move(callback)) {
}

void ChooseRecipientBoxController::prepareViewHook() {
	delegate()->peerListSetTitle(langFactory(lng_forward_choose));
}

void ChooseRecipientBoxController::rowClicked(not_null<PeerListRow*> row) {
	auto weak = base::make_weak(this);
	auto callback = std::move(_callback);
	callback(row->peer());
	if (weak) {
		_callback = std::move(callback);
	}
}

auto ChooseRecipientBoxController::createRow(
		not_null<History*> history) -> std::unique_ptr<Row> {
	return std::make_unique<Row>(history);
}

class FriendRequestBoxController::Row : public PeerListRow {
public:
	Row(not_null<UserData*> user, const style::TextAction &st = st::defaultTextAction)
		: PeerListRow(user)
	, _st(st)
	, _user(user) {
		reFreshStatus();
		_text = lang(lng_friend_request_accept);
		_textWidth = _st.font->width(_text);
	}
	void reFreshStatus() {
		auto customStatus = [=] {
			_status = _user->verifyStatus();
			switch (_status) {
			case UserData::VerifyStatus::UnDeal:
				return _user->verifyInfo().isEmpty()
					? QString()
					: lng_friend_request_info(lt_info, _user->verifyInfo());
			case UserData::VerifyStatus::Accepted:
				return lang(lng_friend_request_accepted);
			case UserData::VerifyStatus::Refused:
				return lang(lng_friend_request_refused);
			case UserData::VerifyStatus::Invalid:
				return lang(lng_friend_request_invalid);
			}
			return QString();
		}();
		setCustomStatus(customStatus);
	}
	QSize actionSize() const override {
		return _status == UserData::VerifyStatus::UnDeal
		? QSize(_textWidth + _st.padding.left() + _st.padding.right(), 
			_st.height + _st.padding.top() + _st.padding.bottom())
		: QSize();
	}
	QMargins actionMargins() const override {
		auto top = (st::defaultPeerListItem.height - actionSize().height()) >> 1;
		auto bottom = top;
		return QMargins(
			0,
			top,
			st::defaultPeerListItem.photoPosition.x(),
			bottom);
	}
	void addActionRipple(QPoint point, Fn<void()> updateCallback) override;
	void stopLastActionRipple() override;
	void paintAction(
		Painter& p,
		int x,
		int y,
		int outerWidth,
		bool selected,
		bool actionSelected) override;
private:
	const style::TextAction& _st;
	not_null<UserData*> _user;
	QString _text;
	int _textWidth;
	UserData::VerifyStatus _status;
	std::unique_ptr<Ui::RippleAnimation> _actionRipple;
};

void FriendRequestBoxController::Row::addActionRipple(QPoint point, Fn<void()> updateCallback) {
	if (!_actionRipple) {
		auto mask = Ui::RippleAnimation::rectMask(actionSize());
		_actionRipple = std::make_unique<Ui::RippleAnimation>(_st.ripple, std::move(mask), std::move(updateCallback));
	}
	_actionRipple->add(point);
}

void FriendRequestBoxController::Row::paintAction(
	Painter& p,
	int x,
	int y,
	int outerWidth,
	bool selected,
	bool actionSelected) {
	auto size = actionSize();
	auto fill = QRect(x, y, size.width(), size.height());
	App::roundRect(p, fill, _st.textBg, ImageRoundRadius::Small);
	if (_actionRipple) {
		_actionRipple->paint(p, x, y, outerWidth);
		if (_actionRipple->empty()) {
			_actionRipple.reset();
		}
	}	
	p.setFont(_st.font);
	p.setPen(_st.textFg);
	p.drawTextLeft(x + _st.padding.left(), y + _st.padding.top(), outerWidth, _text, _textWidth);

}

void FriendRequestBoxController::Row::stopLastActionRipple() {
	if (_actionRipple) {
		_actionRipple->lastStop();
	}
}

FriendRequestBoxController::FriendRequestBoxController(
	std::unique_ptr<PeerListSearchController> searchController)
: PeerListController(std::move(searchController)) {
}

void FriendRequestBoxController::prepare() {
	prepareViewHook();

	rebuildRows();

	auto &sessionData = Auth().data();
	sessionData.friendRequestChanged(
	) | rpl::start_with_next([=](auto count) {
		if (count) {
			rebuildRows();
		}
	}, lifetime());
}

std::unique_ptr<PeerListRow> FriendRequestBoxController::createSearchRow(
	not_null<PeerData*> peer) {
	if (const auto user = peer->asUser()) {
		return createRow(user);
	}
	return nullptr;
}

void FriendRequestBoxController::rowClicked(not_null<PeerListRow*> row) {
	if (auto user = row->peer()->asUser()) {
		auto status = user->verifyStatus();
		if (status == UserData::VerifyStatus::Accepted) {
			Ui::showPeerHistory(user, ShowAtUnreadMsgId);
		} else if (status == UserData::VerifyStatus::UnDeal) {
			Ui::show(Box<HandleFriendRequestBox>(user), LayerOption::KeepOther);
		}		
	}
}

void FriendRequestBoxController::rowActionClicked(not_null<PeerListRow*> row) {
	rowClicked(row);
}

void FriendRequestBoxController::prepareViewHook() {
	setSearchNoResultsText(lang(lng_blocked_list_not_found));
	delegate()->peerListSetSearchMode(PeerListSearchMode::Enabled);
	delegate()->peerListSetTitle(langFactory(lng_friend_request));
	if (!_lifetime) {
		Auth().data().friendRequestChanged(
		) | rpl::start_with_next([=](auto) {
			rebuildRows();
		}, _lifetime);
	}	
}

void FriendRequestBoxController::updateRowHook(not_null<PeerListRow*> row) {
	static_cast<Row*>(row.get())->reFreshStatus();
}

std::unique_ptr<PeerListRow> FriendRequestBoxController::createRow(not_null<UserData*> user) {
	return std::make_unique<Row>(user);
}

void FriendRequestBoxController::rebuildRows() {
	auto appendList = [this](auto friendRequests) {
		auto count = 0;
		for (auto i = friendRequests.cbegin(), e = friendRequests.cend(); i != e; ++i) {
			if (appendRow(*i)) {
				++count;
			}
		}
		return count;
	};
	auto &friendRequests = Auth().data().friendRequests();
	auto filter = [&friendRequests](UserData::VerifyStatus verifyStatus) {
		std::list<not_null<UserData*>> result;
		for (const auto& user : friendRequests) {
			if (user->verifyStatus() == verifyStatus) {
				result.push_back(user);
			}
		}
		return result;
	};
	appendList(filter(UserData::VerifyStatus::UnDeal));
	appendList(filter(UserData::VerifyStatus::Accepted));
	appendList(filter(UserData::VerifyStatus::Refused));
	appendList(filter(UserData::VerifyStatus::Invalid));
	checkForEmptyRows();
	delegate()->peerListRefreshRows();
}

void FriendRequestBoxController::checkForEmptyRows() {
	if (delegate()->peerListFullRowsCount()) {
		setDescriptionText(QString());
	} else {
		setDescriptionText(lang(lng_friend_request_empty));
	}
}

bool FriendRequestBoxController::appendRow(not_null<UserData*> user) {
	if (auto row = delegate()->peerListFindRow(user->id)) {
		updateRowHook(row);
		return false;
	}
	if (auto row = createRow(user)) {
		delegate()->peerListAppendRow(std::move(row));
		return true;
	}
	return false;
}
