/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "data/data_types.h"

enum NewMessageType : char;
enum class ImageRoundRadius;
class MainWindow;
class MainWidget;
class HistoryItem;
class History;
namespace HistoryView {
class Element;
} // namespace HistoryView

using HistoryItemsMap = base::flat_set<not_null<HistoryItem*>>;
using GifItems = QHash<Media::Clip::Reader*, HistoryItem*>;

enum RoundCorners {
	SmallMaskCorners = 0x00, // for images
	LargeMaskCorners,

	BoxCorners,
	MenuCorners,
	BotKbOverCorners,
	StickerCorners,
	StickerSelectedCorners,
	SelectedOverlaySmallCorners,
	SelectedOverlayLargeCorners,
	DateCorners,
	DateSelectedCorners,
	OverviewVideoCorners,
	OverviewVideoSelectedCorners,
	ForwardCorners,
	MediaviewSaveCorners,
	EmojiHoverCorners,
	StickerHoverCorners,
	BotKeyboardCorners,
	PhotoSelectOverlayCorners,

	Doc1Corners,
	Doc2Corners,
	Doc3Corners,
	Doc4Corners,

	InShadowCorners, // for photos without bg
	InSelectedShadowCorners,

	MessageInCorners, // with shadow
	MessageInSelectedCorners,
	MessageOutCorners,
	MessageOutSelectedCorners,

	RoundCornersCount
};

namespace App {
	MainWindow *wnd();
	MainWidget *main();

	QString formatPhone(QString phone);

	bool checkEntitiesAndViewsUpdate(const MTPDmessage &m); // returns true if item found and it is not detached
	void updateEditedMessage(const MTPMessage &m);
	void addSavedGif(DocumentData *doc);
	void checkSavedGif(HistoryItem *item);
	void feedMsgs(const QVector<MTPMessage> &msgs, NewMessageType type);
	void feedMsgs(const MTPVector<MTPMessage> &msgs, NewMessageType type);
	void feedInboxRead(const PeerId &peer, MsgId upTo);
	void feedOutboxRead(const PeerId &peer, MsgId upTo, TimeId when);
	void feedWereDeleted(ChannelId channelId, const QVector<MTPint> &msgsIds);
	void feedUserLink(MTPint userId, const MTPContactLink &myLink, const MTPContactLink &foreignLink);

	[[nodiscard]] QString peerName(const PeerData *peer, bool forDialogs = false);

	[[nodiscard]] HistoryItem *histItemById(ChannelId channelId, MsgId itemId);
	[[nodiscard]] HistoryItem *histItemById(
		const ChannelData *channel,
		MsgId itemId);
	[[nodiscard]] inline HistoryItem *histItemById(const FullMsgId &msgId) {
		return histItemById(msgId.channel, msgId.msg);
	}
	void historyRegItem(not_null<HistoryItem*> item);
	void historyUnregItem(not_null<HistoryItem*> item);
	void historyUpdateDependent(not_null<HistoryItem*> item);
	void historyClearMsgs();
	void historyClearItems();
	void historyRegDependency(HistoryItem *dependent, HistoryItem *dependency);
	void historyUnregDependency(HistoryItem *dependent, HistoryItem *dependency);

	void historyRegRandom(uint64 randomId, const FullMsgId &itemId);
	void historyUnregRandom(uint64 randomId);
	FullMsgId histItemByRandom(uint64 randomId);
	void historyRegSentData(uint64 randomId, const PeerId &peerId, const QString &text);
	void historyUnregSentData(uint64 randomId);
	void histSentDataByItem(uint64 randomId, PeerId &peerId, QString &text);

	void hoveredItem(HistoryView::Element *item);
	HistoryView::Element *hoveredItem();
	void pressedItem(HistoryView::Element *item);
	HistoryView::Element *pressedItem();
	void hoveredLinkItem(HistoryView::Element *item);
	HistoryView::Element *hoveredLinkItem();
	void pressedLinkItem(HistoryView::Element *item);
	HistoryView::Element *pressedLinkItem();
	void mousedItem(HistoryView::Element *item);
	HistoryView::Element *mousedItem();
	void clearMousedItems();

	const style::font &monofont();

	void initMedia();
	void deinitMedia();

	enum LaunchState {
		Launched = 0,
		QuitRequested = 1,
		QuitProcessed = 2,
	};
	void quit();
	bool quitting();
	LaunchState launchState();
	void setLaunchState(LaunchState state);
	void restart();

	constexpr auto kFileSizeLimit = 100 * 1024 * 1024; // Load files up to 100mb
	constexpr auto kImageSizeLimit = 8 * 1024 * 1024; // Open images up to 8mb jpg/png/gif
	QImage readImage(QByteArray data, QByteArray *format = nullptr, bool opaque = true, bool *animated = nullptr);
	QImage readImage(const QString &file, QByteArray *format = nullptr, bool opaque = true, bool *animated = nullptr, QByteArray *content = 0);
	QPixmap pixmapFromImageInPlace(QImage &&image);

	void complexOverlayRect(Painter &p, QRect rect, ImageRoundRadius radius, RectParts corners);
	void complexLocationRect(Painter &p, QRect rect, ImageRoundRadius radius, RectParts corners);

	QImage *cornersMask(ImageRoundRadius radius);
	void roundRect(Painter &p, int32 x, int32 y, int32 w, int32 h, style::color bg, RoundCorners index, const style::color *shadow = nullptr, RectParts parts = RectPart::Full);
	inline void roundRect(Painter &p, const QRect &rect, style::color bg, RoundCorners index, const style::color *shadow = nullptr, RectParts parts = RectPart::Full) {
		return roundRect(p, rect.x(), rect.y(), rect.width(), rect.height(), bg, index, shadow, parts);
	}
	void roundShadow(Painter &p, int32 x, int32 y, int32 w, int32 h, style::color shadow, RoundCorners index, RectParts parts = RectPart::Full);
	inline void roundShadow(Painter &p, const QRect &rect, style::color shadow, RoundCorners index, RectParts parts = RectPart::Full) {
		return roundShadow(p, rect.x(), rect.y(), rect.width(), rect.height(), shadow, index, parts);
	}
	void roundRect(Painter &p, int32 x, int32 y, int32 w, int32 h, style::color bg, ImageRoundRadius radius, RectParts parts = RectPart::Full);
	inline void roundRect(Painter &p, const QRect &rect, style::color bg, ImageRoundRadius radius, RectParts parts = RectPart::Full) {
		return roundRect(p, rect.x(), rect.y(), rect.width(), rect.height(), bg, radius, parts);
	}

};
