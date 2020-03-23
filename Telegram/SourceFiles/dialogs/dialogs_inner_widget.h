/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "dialogs/dialogs_widget.h"
#include "dialogs/dialogs_key.h"
#include "data/data_messages.h"
#include "ui/effects/animations.h"
#include "base/flags.h"
#include "contact/datadefine.h"

namespace Dialogs {
class Row;
class FakeRow;
class IndexedList;
} // namespace Dialogs

namespace Ui {
class IconButton;
class PopupMenu;
class LinkButton;
} // namespace Ui

namespace Window {
class Controller;
} // namespace Window

class DialogsInner : public Ui::SplittedWidget, public RPCSender, private base::Subscriber {
	Q_OBJECT

public:
	DialogsInner(QWidget *parent, not_null<Window::Controller*> controller, QWidget *main);

	void dialogsReceived(const QVector<MTPDialog> &dialogs);
	void addSavedPeersAfter(const QDateTime &date);
	void addAllSavedPeers();
	bool searchReceived(
		const QVector<MTPMessage> &result,
		DialogsSearchRequestType type,
		int fullCount);
	void peerSearchReceived(
		const QString &query,
		const QVector<MTPPeer> &my,
		const QVector<MTPPeer> &result);
	void showMore(int32 pixels);

	void activate();

	void selectSkip(int32 direction);
	void selectSkipPage(int32 pixels, int32 direction);

	void createDialog(Dialogs::Key key);
	void createGroupDialog(const MTPUserGroupList& result);
	void removeGroupDialog();
	void removeDialog(Dialogs::Key key);
	void repaintDialogRow(Dialogs::Mode list, not_null<Dialogs::Row*> row);
	void repaintDialogRow(Dialogs::RowDescriptor row);

	void dragLeft();

	void clearFilter();
	void refresh(bool toTop = false);

	bool chooseRow();

	void destroyData();

	void scrollToEntry(const Dialogs::RowDescriptor &entry);
	QMap<uint64, QSet<uint64>>& getUserGroupInfo();
	QVector<Contact::ContactInfo*>& getGroupInfo();
	QVector<Contact::ContactInfo*>& getGroupInfo4Search();
	bool existUser(uint64 userId);
	Dialogs::IndexedList *contactsList();
	Dialogs::IndexedList *dialogsList();
	Dialogs::IndexedList *contactsNoDialogsList();
	int32 lastSearchDate() const;
	PeerData *lastSearchPeer() const;
	MsgId lastSearchId() const;
	MsgId lastSearchMigratedId() const;

	enum class State {
		Default,
		Filtered,
	};
	State state() const;
	bool waitingForSearch() const {
		return _waitingForSearch;
	}
	bool hasFilteredResults() const;

	void searchInChat(Dialogs::Key key, UserData *from);

	void applyFilterUpdate(QString newFilter, bool force = false);
	void onHashtagFilterUpdate(QStringRef newFilter);

	PeerData *updateFromParentDrag(QPoint globalPosition);

	void setLoadMoreCallback(Fn<void()> callback);
	[[nodiscard]] rpl::producer<> listBottomReached() const;

	base::Observable<UserData*> searchFromUserChanged;
	base::Observable<int> _signalGroupChanged;

	void notify_historyMuteUpdated(History *history);

	~DialogsInner();

public slots:
	void onParentGeometryChanged();
	void onDialogRowReplaced(Dialogs::Row *oldRow, Dialogs::Row *newRow);

signals:
	void draggingScrollDelta(int delta);
	void mustScrollTo(int scrollToTop, int scrollToBottom);
	void dialogMoved(int movedFrom, int movedTo);
	void searchMessages();
	void clearSearchQuery();
	void cancelSearchInChat();
	void completeHashtag(QString tag);
	void refreshHashtags();

protected:
	void visibleTopBottomUpdated(
		int visibleTop,
		int visibleBottom) override;

	void paintRegion(Painter &p, const QRegion &region, bool paintingOther) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;
	void enterEventHook(QEvent *e) override;
	void leaveEventHook(QEvent *e) override;
	void contextMenuEvent(QContextMenuEvent *e) override;

private:
	struct ImportantSwitch;
	struct HashtagResult;
	struct PeerSearchResult;

	enum class JumpSkip {
		PreviousOrBegin,
		NextOrEnd,
		PreviousOrOriginal,
		NextOrOriginal,
	};

	struct ChosenRow {
		Dialogs::Key key;
		Data::MessagePosition message;
	};
	bool switchImportantChats();
	bool chooseHashtag();
	ChosenRow computeChosenRow() const;
	bool isSearchResultActive(
		not_null<Dialogs::FakeRow*> result,
		const Dialogs::RowDescriptor &entry) const;

	void clearMouseSelection(bool clearSelection = false);
	void userIsContactUpdated(not_null<UserData*> user);
	void mousePressReleased(QPoint globalPosition, Qt::MouseButton button);
	void clearIrrelevantState();
	void selectByMouse(QPoint globalPosition);
	void loadPeerPhotos();
	void setImportantSwitchPressed(bool pressed);
	void setPressed(Dialogs::Row *pressed);
	void setHashtagPressed(int pressed);
	void setFilteredPressed(int pressed);
	void setPeerSearchPressed(int pressed);
	void setSearchedPressed(int pressed);
	bool isPressed() const {
		return _importantSwitchPressed
			|| _pressed
			|| (_hashtagPressed >= 0)
			|| (_filteredPressed >= 0)
			|| (_peerSearchPressed >= 0)
			|| (_searchedPressed >= 0);
	}
	bool isSelected() const {
		return _importantSwitchSelected
			|| _selected
			|| (_hashtagSelected >= 0)
			|| (_filteredSelected >= 0)
			|| (_peerSearchSelected >= 0)
			|| (_searchedSelected >= 0);
	}
	void handlePeerNameChange(
		not_null<PeerData*> peer,
		const base::flat_set<QChar> &oldLetters);
	bool uniqueSearchResults() const;
	bool hasHistoryInResults(not_null<History*> history) const;

	void setupShortcuts();
	Dialogs::RowDescriptor computeJump(
		const Dialogs::RowDescriptor &to,
		JumpSkip skip);
	bool jumpToDialogRow(Dialogs::RowDescriptor to);

	Dialogs::RowDescriptor chatListEntryBefore(
		const Dialogs::RowDescriptor &which) const;
	Dialogs::RowDescriptor chatListEntryAfter(
		const Dialogs::RowDescriptor &which) const;
	Dialogs::RowDescriptor chatListEntryFirst() const;
	Dialogs::RowDescriptor chatListEntryLast() const;

	void applyDialog(const MTPDdialog &dialog);
//	void applyFeedDialog(const MTPDdialogFeed &dialog); // #feed

	void itemRemoved(not_null<const HistoryItem*> item);
	enum class UpdateRowSection {
		Default       = (1 << 0),
		Filtered      = (1 << 1),
		PeerSearch    = (1 << 2),
		MessageSearch = (1 << 3),
		All           = Default | Filtered | PeerSearch | MessageSearch,
	};
	using UpdateRowSections = base::flags<UpdateRowSection>;
	friend inline constexpr auto is_flag_type(UpdateRowSection) { return true; };

	void updateSearchResult(not_null<PeerData*> peer);
	void updateDialogRow(
		Dialogs::RowDescriptor row,
		QRect updateRect = QRect(),
		UpdateRowSections sections = UpdateRowSection::All);
	void fillSupportSearchMenu(not_null<Ui::PopupMenu*> menu);

	int dialogsOffset() const;
	int proxyPromotedCount() const;
	int pinnedOffset() const;
	int filteredOffset() const;
	int peerSearchOffset() const;
	int searchedOffset() const;
	int searchInChatSkip() const;

	void paintPeerSearchResult(
		Painter &p,
		not_null<const PeerSearchResult*> result,
		int fullWidth,
		bool active,
		bool selected,
		bool onlyBackground) const;
	void paintSearchInChat(
		Painter &p,
		int fullWidth,
		bool onlyBackground) const;
	void paintSearchInPeer(
		Painter &p,
		not_null<PeerData*> peer,
		int top,
		int fullWidth,
		const Text &text) const;
	void paintSearchInSaved(
		Painter &p,
		int top,
		int fullWidth,
		const Text &text) const;
	void paintSearchInFeed(
		Painter &p,
		not_null<Data::Feed*> feed,
		int top,
		int fullWidth,
		const Text &text) const;
	template <typename PaintUserpic>
	void paintSearchInFilter(
		Painter &p,
		PaintUserpic paintUserpic,
		int top,
		int fullWidth,
		const style::icon *icon,
		const Text &text) const;
	void refreshSearchInChatLabel();

	void clearSelection();
	void clearSearchResults(bool clearPeerSearchResults = true);
	void updateSelectedRow(Dialogs::Key key = Dialogs::Key());

	Dialogs::IndexedList *shownDialogs() const;

	void checkReorderPinnedStart(QPoint localPosition);
	int shownPinnedCount() const;
	int updateReorderIndexGetCount();
	bool updateReorderPinned(QPoint localPosition);
	void finishReorderPinned();
	void stopReorderPinned();
	int countPinnedIndex(Dialogs::Row *ofRow);
	void savePinnedOrder();
	bool pinnedShiftAnimationCallback(crl::time now);
	void handleChatMigration(not_null<ChatData*> chat);

	not_null<Window::Controller*> _controller;

	std::unique_ptr<Dialogs::IndexedList> _dialogs;
	std::unique_ptr<Dialogs::IndexedList> _dialogsImportant;

	std::unique_ptr<Dialogs::IndexedList> _contactsNoDialogs;
	std::unique_ptr<Dialogs::IndexedList> _contacts;

	QVector<Contact::ContactInfo*> _vecContactAndGroupData; //����Ϣ
	QVector<Contact::ContactInfo*> _vecContactAndGroupData4Search; //���ڲ�ѯ������Ϣ
	QMap<uint64, QSet<uint64>> _mapUser2Group; //�û�������Щ��

	bool _mouseSelection = false;
	std::optional<QPoint> _lastMousePosition;
	Qt::MouseButton _pressButton = Qt::LeftButton;

	std::unique_ptr<ImportantSwitch> _importantSwitch;
	bool _importantSwitchSelected = false;
	bool _importantSwitchPressed = false;
	Dialogs::Row *_selected = nullptr;
	Dialogs::Row *_pressed = nullptr;

	Dialogs::Row *_dragging = nullptr;
	int _draggingIndex = -1;
	int _aboveIndex = -1;
	QPoint _dragStart;
	struct PinnedRow {
		anim::value yadd;
		crl::time animStartTime = 0;
	};
	std::vector<PinnedRow> _pinnedRows;
	Ui::Animations::Basic _pinnedShiftAnimation;
	std::deque<Dialogs::Key> _pinnedOrder;

	// Remember the last currently dragged row top shift for updating area.
	int _aboveTopShift = -1;

	int _visibleTop = 0;
	int _visibleBottom = 0;
	QString _filter, _hashtagFilter;

	std::vector<std::unique_ptr<HashtagResult>> _hashtagResults;
	int _hashtagSelected = -1;
	int _hashtagPressed = -1;
	bool _hashtagDeleteSelected = false;
	bool _hashtagDeletePressed = false;

	std::vector<Dialogs::Row*> _filterResults;
	base::flat_map<
		not_null<PeerData*>,
		std::unique_ptr<Dialogs::Row>> _filterResultsGlobal;
	int _filteredSelected = -1;
	int _filteredPressed = -1;

	bool _waitingForSearch = false;

	QString _peerSearchQuery;
	std::vector<std::unique_ptr<PeerSearchResult>> _peerSearchResults;
	int _peerSearchSelected = -1;
	int _peerSearchPressed = -1;

	std::vector<std::unique_ptr<Dialogs::FakeRow>> _searchResults;
	int _searchedCount = 0;
	int _searchedMigratedCount = 0;
	int _searchedSelected = -1;
	int _searchedPressed = -1;

	int _lastSearchDate = 0;
	PeerData *_lastSearchPeer = nullptr;
	MsgId _lastSearchId = 0;
	MsgId _lastSearchMigratedId = 0;

	State _state = State::Default;

	object_ptr<Ui::LinkButton> _addContactLnk;
	object_ptr<Ui::IconButton> _cancelSearchInChat;
	object_ptr<Ui::IconButton> _cancelSearchFromUser;

	Dialogs::Key _searchInChat;
	History *_searchInMigrated = nullptr;
	UserData *_searchFromUser = nullptr;
	Text _searchInChatText;
	Text _searchFromUserText;
	Dialogs::RowDescriptor _menuRow;

	Fn<void()> _loadMoreCallback;
	rpl::event_stream<> _listBottomReached;

	base::unique_qptr<Ui::PopupMenu> _menu;

};
