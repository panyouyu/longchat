/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "window/section_widget.h"
#include "ui/effects/animations.h"
#include "ui/widgets/scroll_area.h"
#include "dialogs/dialogs_key.h"
#include "ui/special_buttons.h"
#include "contact/datadefine.h"

class DialogsInner;

namespace Dialogs {
struct RowDescriptor;
class Row;
class FakeRow;
class IndexedList;
class Key;
} // namespace Dialogs

namespace Ui {
class IconButton;
class PopupMenu;
class DropdownMenu;
class FlatButton;
class FlatInput;
class CrossButton;
template <typename Widget>
class FadeWrapScaled;
} // namespace Ui

namespace Window {
class Controller;
class ConnectionState;
} // namespace Window

enum DialogsSearchRequestType {
	DialogsSearchFromStart,
	DialogsSearchFromOffset,
	DialogsSearchPeerFromStart,
	DialogsSearchPeerFromOffset,
	DialogsSearchMigratedFromStart,
	DialogsSearchMigratedFromOffset,
};

class DialogsWidget : public Window::AbstractSectionWidget, public RPCSender {
	Q_OBJECT

public:
	DialogsWidget(QWidget *parent, not_null<Window::Controller*> controller);

	void updateDragInScroll(bool inScroll);

	void searchInChat(Dialogs::Key chat);
	QMutex& getUserGroupMutex();
	QMap<uint64, QSet<uint64>>& getUserGroupInfo();
	QVector<Contact::ContactInfo*>& getGroupInfo();
	QString getUserGroupInfo(uint64 userId);
	bool userInSeeking(uint64 userId);
	QString getGroupName(uint64 groupId);
	QVector<Contact::ContactInfo*>& getGroupInfo4Search();

	void loadDialogs();
	void loadGroupDialogs();
	void setDialogGetFull(bool full);
	base::Observable<int>& signalGroupChanged();
	void loadPinnedDialogs();
	void createDialog(Dialogs::Key key);
	void removeDialog(Dialogs::Key key);
	void repaintDialogRow(Dialogs::Mode list, not_null<Dialogs::Row*> row);
	void repaintDialogRow(Dialogs::RowDescriptor row);

	void jumpToTop();

	void startWidthAnimation();
	void stopWidthAnimation();

	bool hasTopBarShadow() const {
		return true;
	}
	void showAnimated(Window::SlideDirection direction, const Window::SectionSlideParams &params);
	void showFast();

	void destroyData();

	void scrollToEntry(const Dialogs::RowDescriptor &entry);

	Dialogs::IndexedList *contactsList();
	Dialogs::IndexedList *dialogsList();
	Dialogs::IndexedList *contactsNoDialogsList();

	void searchMessages(const QString &query, Dialogs::Key inChat = {});
	void onSearchMore();

	// Float player interface.
	bool wheelEventFromFloatPlayer(QEvent *e) override;
	QRect rectForFloatPlayer() const override;

	void notify_historyMuteUpdated(History *history);

	~DialogsWidget();

signals:
	void cancelled();
	void kfSessionTimeOut(int64 peerId);
public slots:
	void onDraggingScrollDelta(int delta);

	void onCancel();
	void onListScroll();
	void activate();
	bool onCancelSearch();
	void onCancelSearchInChat();

	void onFilterCursorMoved(int from = -1, int to = -1);
	void onCompleteHashtag(QString tag);

	void onDialogMoved(int movedFrom, int movedTo);
	bool onSearchMessages(bool searchCache = false);
	void onNeedSearchMessages();

	void onChooseByDrag();
	void onQueueCountChanged(int count);
	void onContactStatus();
	void onKfSessionTimeOut(int64 peerId);

private slots:
	void onDraggingScrollTimer();

protected:
	void dragEnterEvent(QDragEnterEvent *e) override;
	void dragMoveEvent(QDragMoveEvent *e) override;
	void dragLeaveEvent(QDragLeaveEvent *e) override;
	void dropEvent(QDropEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;
	void keyPressEvent(QKeyEvent *e) override;
	void paintEvent(QPaintEvent *e) override;

private:
	void userGroupDone(const MTPUserGroupData& result);
	bool userGroupFail(const RPCError& error);
	void animationCallback();
	void dialogsReceived(
		const MTPmessages_Dialogs &result,
		mtpRequestId requestId);
	void pinnedDialogsReceived(
		const MTPmessages_PeerDialogs &result,
		mtpRequestId requestId);
	void searchReceived(
		DialogsSearchRequestType type,
		const MTPmessages_Messages &result,
		mtpRequestId requestId);
	void peerSearchReceived(
		const MTPcontacts_Found &result,
		mtpRequestId requestId);
	void updateDialogsOffset(
		const QVector<MTPDialog> &dialogs,
		const QVector<MTPMessage> &messages);
	void applyReceivedDialogs(
		const QVector<MTPDialog> &dialogs,
		const QVector<MTPMessage> &messages);

	void setupSupportMode();
	void setupConnectingWidget();
	bool searchForPeersRequired(const QString &query) const;
	void setSearchInChat(Dialogs::Key chat, UserData *from = nullptr);
	void showJumpToDate();
	void showSearchFrom();
	void showMainMenu();
	void clearSearchCache();
	void updateLockUnlockVisibility();
	void updateJumpToDateVisibility(bool fast = false);
	void updateSearchFromVisibility(bool fast = false);
	void updateControlsGeometry();
	void updateForwardBar();
	void checkUpdateStatus();

	void applyFilterUpdate(bool force = false);
	bool loadingBlockedByDate() const;
	void refreshLoadMoreButton();
	void loadMoreBlockedByDateChats();

	bool dialogsFailed(const RPCError &error, mtpRequestId req);
	bool searchFailed(DialogsSearchRequestType type, const RPCError &error, mtpRequestId req);
	bool peopleFailed(const RPCError &error, mtpRequestId req);

	bool _dragInScroll = false;
	bool _dragForward = false;
	QTimer _chooseByDragTimer;

	bool _dialogsFull = false;
	TimeId _dialogsLoadTill = 0;
	TimeId _dialogsOffsetDate = 0;
	MsgId _dialogsOffsetId = 0;
	PeerData *_dialogsOffsetPeer = nullptr;
	mtpRequestId _dialogsRequestId = 0;
	mtpRequestId _allUserTagRequest = 0;
	mtpRequestId _pinnedDialogsRequestId = 0;
	bool _pinnedDialogsReceived = false;

	object_ptr<Ui::IconButton> _forwardCancel = { nullptr };
	object_ptr<Ui::IconButton> _mainMenuToggle;
	object_ptr<Ui::FlatInput> _filter;
	object_ptr<Ui::FadeWrapScaled<Ui::IconButton>> _chooseFromUser;
	object_ptr<Ui::FadeWrapScaled<Ui::IconButton>> _jumpToDate;
	object_ptr<Ui::CrossButton> _cancelSearch;
	object_ptr<Ui::IconButton> _lockUnlock;
	object_ptr<Ui::ScrollArea> _scroll;
	QPointer<DialogsInner> _inner;
	class BottomButton;
	object_ptr<BottomButton> _updateTelegram = { nullptr };
	object_ptr<BottomButton> _loadMoreChats = { nullptr };
	object_ptr<BottomButton> _custQueueCount = { nullptr };
	std::unique_ptr<Window::ConnectionState> _connecting;

	Ui::Animations::Simple _scrollToAnimation;
	Ui::Animations::Simple _a_show;
	Window::SlideDirection _showDirection;
	QPixmap _cacheUnder, _cacheOver;

	Ui::Animations::Simple _scrollToTopShown;
	bool _scrollToTopIsShown = false;
	object_ptr<Ui::HistoryDownButton> _scrollToTop;

	void scrollToTop();
	void setupScrollUpButton();
	void updateScrollUpVisibility();
	void startScrollUpButtonAnimation(bool shown);
	void updateScrollUpPosition();

	Dialogs::Key _searchInChat;
	History *_searchInMigrated = nullptr;
	UserData *_searchFromUser = nullptr;
	QString _lastFilterText;

	QTimer _searchTimer;

	QString _peerSearchQuery;
	bool _peerSearchFull = false;
	mtpRequestId _peerSearchRequest = 0;

	QString _searchQuery;
	UserData *_searchQueryFrom = nullptr;
	bool _searchFull = false;
	bool _searchFullMigrated = false;
	mtpRequestId _searchRequest = 0;

	using SearchCache = QMap<QString, MTPmessages_Messages>;
	SearchCache _searchCache;

	using SearchQueries = QMap<mtpRequestId, QString>;
	SearchQueries _searchQueries;

	using PeerSearchCache = QMap<QString, MTPcontacts_Found>;
	PeerSearchCache _peerSearchCache;

	using PeerSearchQueries = QMap<mtpRequestId, QString>;
	PeerSearchQueries _peerSearchQueries;

	QPixmap _widthAnimationCache;

	object_ptr<QTimer> _draggingScrollTimer = { nullptr };
	int _draggingScrollDelta = 0;

};
