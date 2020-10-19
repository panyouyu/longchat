/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "storage/storage_databases.h"
#include "chat_helpers/stickers.h"
#include "dialogs/dialogs_key.h"
#include "data/data_groups.h"
#include "data/data_notify_settings.h"
#include "history/history_location_manager.h"
#include "base/timer.h"
#include "ui/effects/animations.h"

class Image;
class HistoryItem;
class BoxContent;
class GroupJoinApply;
struct WebPageCollage;
enum class WebPageType;

namespace HistoryView {
struct Group;
class Element;
} // namespace HistoryView

class AuthSession;

namespace Media {
namespace Clip {
class Reader;
} // namespace Clip
} // namespace Media

namespace Export {
class Controller;
namespace View {
class PanelController;
} // namespace View
} // namespace Export

namespace Passport {
struct SavedCredentials;
} // namespace Passport

namespace Data {

class Feed;
enum class FeedUpdateFlag;
struct FeedUpdate;

class WallPaper;

class Session final {
public:
	using ViewElement = HistoryView::Element;

	explicit Session(not_null<AuthSession*> session);
	~Session();

	[[nodiscard]] AuthSession &session() const {
		return *_session;
	}

	void clear();

	void startExport(PeerData *peer = nullptr);
	void startExport(const MTPInputPeer &singlePeer);
	void suggestStartExport(TimeId availableAt);
	void clearExportSuggestion();
	[[nodiscard]] auto currentExportView() const
	-> rpl::producer<Export::View::PanelController*>;
	bool exportInProgress() const;
	void stopExportWithConfirmation(FnMut<void()> callback);
	void stopExport();

	[[nodiscard]] auto passportCredentials() const
	-> const Passport::SavedCredentials*;
	void rememberPassportCredentials(
		Passport::SavedCredentials data,
		crl::time rememberFor);
	void forgetPassportCredentials();

	[[nodiscard]] Storage::Cache::Database &cache();
	[[nodiscard]] Storage::Cache::Database &cacheBigFile();

	[[nodiscard]] not_null<PeerData*> peer(PeerId id);
	[[nodiscard]] not_null<PeerData*> peer(UserId id) = delete;
	[[nodiscard]] not_null<UserData*> user(UserId id);
	[[nodiscard]] not_null<ChatData*> chat(ChatId id);
	[[nodiscard]] not_null<ChannelData*> channel(ChannelId id);
	[[nodiscard]] not_null<UserData*> user(PeerId id) = delete;
	[[nodiscard]] not_null<ChatData*> chat(PeerId id) = delete;
	[[nodiscard]] not_null<ChannelData*> channel(PeerId id) = delete;

	[[nodiscard]] PeerData *peerLoaded(PeerId id) const;
	[[nodiscard]] PeerData *peerLoaded(UserId id) const = delete;
	[[nodiscard]] UserData *userLoaded(UserId id) const;
	[[nodiscard]] ChatData *chatLoaded(ChatId id) const;
	[[nodiscard]] ChannelData *channelLoaded(ChannelId id) const;
	[[nodiscard]] UserData *userLoaded(PeerId id) const = delete;
	[[nodiscard]] ChatData *chatLoaded(PeerId id) const = delete;
	[[nodiscard]] ChannelData *channelLoaded(PeerId id) const = delete;

	not_null<UserData*> processUser(const MTPUser &data);
	not_null<PeerData*> processChat(const MTPChat &data);
	not_null<UserData*> processFriendRequest(const MTPFriendRequest &data);

	// Returns last user, if there were any.
	UserData *processUsers(const MTPVector<MTPUser> &data);
	PeerData *processChats(const MTPVector<MTPChat> &data);
	UserData * processFriendRequests(const MTPVector<MTPFriendRequest> &data);
	std::list<not_null<UserData*>> &friendRequests() {
		return _friendRequests;
	}
	const std::list<not_null<PeerData*>> &savedGroups() const {
		return _savedGroups;
	}

	void applyMaximumChatVersions(const MTPVector<MTPChat> &data);

	void enumerateUsers(Fn<void(not_null<UserData*>)> action) const;
	void enumerateGroups(Fn<void(not_null<PeerData*>)> action) const;
	void enumerateChannels(Fn<void(not_null<ChannelData*>)> action) const;
	[[nodiscard]] PeerData *peerByUsername(const QString &username) const;

	[[nodiscard]] not_null<History*> history(PeerId peerId);
	[[nodiscard]] History *historyLoaded(PeerId peerId) const;
	[[nodiscard]] not_null<History*> history(UserId userId) = delete;
	[[nodiscard]] History *historyLoaded(UserId userId) const = delete;
	[[nodiscard]] not_null<History*> history(not_null<const PeerData*> peer);
	[[nodiscard]] History *historyLoaded(const PeerData *peer);

	void deleteConversationLocally(not_null<PeerData*> peer);

	void registerSendAction(
		not_null<History*> history,
		not_null<UserData*> user,
		const MTPSendMessageAction &action,
		TimeId when);

	[[nodiscard]] base::Variable<bool> &contactsLoaded() {
		return _contactsLoaded;
	}
	[[nodiscard]] base::Variable<bool> &allChatsLoaded() {
		return _allChatsLoaded;
	}
	[[nodiscard]] base::Observable<void> &moreChatsLoaded() {
		return _moreChatsLoaded;
	}

	struct ItemVisibilityQuery {
		not_null<HistoryItem*> item;
		not_null<bool*> isVisible;
	};
	[[nodiscard]] base::Observable<ItemVisibilityQuery> &queryItemVisibility() {
		return _queryItemVisibility;
	}
	struct IdChange {
		not_null<HistoryItem*> item;
		MsgId oldId = 0;
	};
	void notifyItemIdChange(IdChange event);
	[[nodiscard]] rpl::producer<IdChange> itemIdChanged() const;
	void notifyItemLayoutChange(not_null<const HistoryItem*> item);
	[[nodiscard]] rpl::producer<not_null<const HistoryItem*>> itemLayoutChanged() const;
	void notifyViewLayoutChange(not_null<const ViewElement*> view);
	[[nodiscard]] rpl::producer<not_null<const ViewElement*>> viewLayoutChanged() const;
	void requestItemRepaint(not_null<const HistoryItem*> item);
	[[nodiscard]] rpl::producer<not_null<const HistoryItem*>> itemRepaintRequest() const;
	void requestViewRepaint(not_null<const ViewElement*> view);
	[[nodiscard]] rpl::producer<not_null<const ViewElement*>> viewRepaintRequest() const;
	void requestItemResize(not_null<const HistoryItem*> item);
	[[nodiscard]] rpl::producer<not_null<const HistoryItem*>> itemResizeRequest() const;
	void requestViewResize(not_null<ViewElement*> view);
	[[nodiscard]] rpl::producer<not_null<ViewElement*>> viewResizeRequest() const;
	void requestItemViewRefresh(not_null<HistoryItem*> item);
	[[nodiscard]] rpl::producer<not_null<HistoryItem*>> itemViewRefreshRequest() const;
	void requestItemTextRefresh(not_null<HistoryItem*> item);
	void requestAnimationPlayInline(not_null<HistoryItem*> item);
	[[nodiscard]] rpl::producer<not_null<HistoryItem*>> animationPlayInlineRequest() const;
	void notifyHistoryUnloaded(not_null<const History*> history);
	[[nodiscard]] rpl::producer<not_null<const History*>> historyUnloaded() const;

	void notifyItemRemoved(not_null<const HistoryItem*> item);
	[[nodiscard]] rpl::producer<not_null<const HistoryItem*>> itemRemoved() const;
	void notifyViewRemoved(not_null<const ViewElement*> view);
	[[nodiscard]] rpl::producer<not_null<const ViewElement*>> viewRemoved() const;
	void notifyHistoryCleared(not_null<const History*> history);
	[[nodiscard]] rpl::producer<not_null<const History*>> historyCleared() const;
	void notifyHistoryChangeDelayed(not_null<History*> history);
	[[nodiscard]] rpl::producer<not_null<History*>> historyChanged() const;
	void sendHistoryChangeNotifications();

	using MegagroupParticipant = std::tuple<
		not_null<ChannelData*>,
		not_null<UserData*>>;
	void removeMegagroupParticipant(
		not_null<ChannelData*> channel,
		not_null<UserData*> user);
	[[nodiscard]] rpl::producer<MegagroupParticipant> megagroupParticipantRemoved() const;
	[[nodiscard]] rpl::producer<not_null<UserData*>> megagroupParticipantRemoved(
		not_null<ChannelData*> channel) const;
	void addNewMegagroupParticipant(
		not_null<ChannelData*> channel,
		not_null<UserData*> user);
	[[nodiscard]] rpl::producer<MegagroupParticipant> megagroupParticipantAdded() const;
	[[nodiscard]] rpl::producer<not_null<UserData*>> megagroupParticipantAdded(
		not_null<ChannelData*> channel) const;

	void notifyFeedUpdated(not_null<Feed*> feed, FeedUpdateFlag update);
	[[nodiscard]] rpl::producer<FeedUpdate> feedUpdated() const;

	void notifyStickersUpdated();
	[[nodiscard]] rpl::producer<> stickersUpdated() const;
	void notifySavedGifsUpdated();
	[[nodiscard]] rpl::producer<> savedGifsUpdated() const;

	void notifyFriendRequestChanged();
	[[nodiscard]] rpl::producer<int> friendRequestValue() const {
		return _friendsRequestChanged.value();
	}

	void setGroupUnReadCount(int count) {
		if (count >= 0) {
			_groupUnReadCount = count;
		}		
	}
	int groupUnReadCount() const {
		return _groupUnReadCount.current();
	}
	[[nodiscard]] rpl::producer<int> groupUnReadCountValue() const {
		return _groupUnReadCount.value();
	}

	bool hasSaved(not_null<PeerData*> group) const {
		return std::find(_savedGroups.cbegin(), _savedGroups.cend(), group) != _savedGroups.cend();
	}
	void addSavedGroups(std::list<not_null<PeerData*>> groups);
	void removeSavedGroups(not_null<PeerData*> group);
	[[nodiscard]] rpl::producer<> savedGroupsChanged() const {
		return _savedGroupsStream.events();
	}

	bool stickersUpdateNeeded(crl::time now) const {
		return stickersUpdateNeeded(_lastStickersUpdate, now);
	}
	void setLastStickersUpdate(crl::time update) {
		_lastStickersUpdate = update;
	}
	bool recentStickersUpdateNeeded(crl::time now) const {
		return stickersUpdateNeeded(_lastRecentStickersUpdate, now);
	}
	void setLastRecentStickersUpdate(crl::time update) {
		_lastRecentStickersUpdate = update;
	}
	bool favedStickersUpdateNeeded(crl::time now) const {
		return stickersUpdateNeeded(_lastFavedStickersUpdate, now);
	}
	void setLastFavedStickersUpdate(crl::time update) {
		_lastFavedStickersUpdate = update;
	}
	bool featuredStickersUpdateNeeded(crl::time now) const {
		return stickersUpdateNeeded(_lastFeaturedStickersUpdate, now);
	}
	void setLastFeaturedStickersUpdate(crl::time update) {
		_lastFeaturedStickersUpdate = update;
	}
	bool savedGifsUpdateNeeded(crl::time now) const {
		return stickersUpdateNeeded(_lastSavedGifsUpdate, now);
	}
	void setLastSavedGifsUpdate(crl::time update) {
		_lastSavedGifsUpdate = update;
	}
	int featuredStickerSetsUnreadCount() const {
		return _featuredStickerSetsUnreadCount.current();
	}
	void setFeaturedStickerSetsUnreadCount(int count) {
		_featuredStickerSetsUnreadCount = count;
	}
	[[nodiscard]] rpl::producer<int> featuredStickerSetsUnreadCountValue() const {
		return _featuredStickerSetsUnreadCount.value();
	}
	const Stickers::Sets &stickerSets() const {
		return _stickerSets;
	}
	Stickers::Sets &stickerSetsRef() {
		return _stickerSets;
	}
	const Stickers::Order &stickerSetsOrder() const {
		return _stickerSetsOrder;
	}
	Stickers::Order &stickerSetsOrderRef() {
		return _stickerSetsOrder;
	}
	const Stickers::Order &featuredStickerSetsOrder() const {
		return _featuredStickerSetsOrder;
	}
	Stickers::Order &featuredStickerSetsOrderRef() {
		return _featuredStickerSetsOrder;
	}
	const Stickers::Order &archivedStickerSetsOrder() const {
		return _archivedStickerSetsOrder;
	}
	Stickers::Order &archivedStickerSetsOrderRef() {
		return _archivedStickerSetsOrder;
	}
	const Stickers::SavedGifs &savedGifs() const {
		return _savedGifs;
	}
	Stickers::SavedGifs &savedGifsRef() {
		return _savedGifs;
	}

	HistoryItemsList idsToItems(const MessageIdsList &ids) const;
	MessageIdsList itemsToIds(const HistoryItemsList &items) const;
	MessageIdsList itemOrItsGroup(not_null<HistoryItem*> item) const;

	void applyUpdate(const MTPDupdateMessagePoll &update);
	void applyUpdate(const MTPDupdateChatParticipants &update);
	void applyUpdate(const MTPDupdateChatParticipantAdd &update);
	void applyUpdate(const MTPDupdateChatParticipantDelete &update);
	void applyUpdate(const MTPDupdateChatParticipantAdmin &update);
	void applyUpdate(const MTPDupdateChatDefaultBannedRights &update);

	int pinnedDialogsCount() const;
	const std::deque<Dialogs::Key> &pinnedDialogsOrder() const;
	void setPinnedDialog(const Dialogs::Key &key, bool pinned);
	void applyPinnedDialogs(const QVector<MTPDialog> &list);
	void applyPinnedDialogs(const QVector<MTPDialogPeer> &list);
	void reorderTwoPinnedDialogs(
		const Dialogs::Key &key1,
		const Dialogs::Key &key2);

	void photoLoadSettingsChanged();
	void documentLoadSettingsChanged();

	void notifyPhotoLayoutChanged(not_null<const PhotoData*> photo);
	void notifyDocumentLayoutChanged(
		not_null<const DocumentData*> document);
	void requestDocumentViewRepaint(not_null<const DocumentData*> document);
	void markMediaRead(not_null<const DocumentData*> document);
	void requestPollViewRepaint(not_null<const PollData*> poll);

	HistoryItem *addNewMessage(const MTPMessage &data, NewMessageType type);

	struct SendActionAnimationUpdate {
		not_null<History*> history;
		int width = 0;
		int height = 0;
		bool textUpdated = false;
	};
	[[nodiscard]] auto sendActionAnimationUpdated() const
		-> rpl::producer<SendActionAnimationUpdate>;
	void updateSendActionAnimation(SendActionAnimationUpdate &&update);

	int unreadBadge() const;
	bool unreadBadgeMuted() const;
	int unreadBadgeIgnoreOne(History *history) const;
	bool unreadBadgeMutedIgnoreOne(History *history) const;
	int unreadOnlyMutedBadge() const;

	void unreadIncrement(int count, bool muted);
	void unreadMuteChanged(int count, bool muted);
	void unreadEntriesChanged(
		int withUnreadDelta,
		int mutedWithUnreadDelta);

	void selfDestructIn(not_null<HistoryItem*> item, crl::time delay);

	[[nodiscard]] not_null<PhotoData*> photo(PhotoId id);
	not_null<PhotoData*> processPhoto(const MTPPhoto &data);
	not_null<PhotoData*> processPhoto(const MTPDphoto &data);
	not_null<PhotoData*> processPhoto(
		const MTPPhoto &data,
		const PreparedPhotoThumbs &thumbs);
	[[nodiscard]] not_null<PhotoData*> photo(
		PhotoId id,
		const uint64 &access,
		const QByteArray &fileReference,
		TimeId date,
		int32 dc,
		bool hasSticker,
		const ImagePtr &thumbnailInline,
		const ImagePtr &thumbnailSmall,
		const ImagePtr &thumbnail,
		const ImagePtr &large);
	void photoConvert(
		not_null<PhotoData*> original,
		const MTPPhoto &data);
	[[nodiscard]] PhotoData *photoFromWeb(
		const MTPWebDocument &data,
		ImagePtr thumbnail = ImagePtr(),
		bool willBecomeNormal = false);

	[[nodiscard]] not_null<DocumentData*> document(DocumentId id);
	not_null<DocumentData*> processDocument(const MTPDocument &data);
	not_null<DocumentData*> processDocument(const MTPDdocument &data);
	not_null<DocumentData*> processDocument(
		const MTPdocument &data,
		QImage &&thumb);
	[[nodiscard]] not_null<DocumentData*> document(
		DocumentId id,
		const uint64 &access,
		const QByteArray &fileReference,
		TimeId date,
		const QVector<MTPDocumentAttribute> &attributes,
		const QString &mime,
		const ImagePtr &thumbnailInline,
		const ImagePtr &thumbnail,
		int32 dc,
		int32 size,
		const StorageImageLocation &thumbLocation);
	void documentConvert(
		not_null<DocumentData*> original,
		const MTPDocument &data);
	[[nodiscard]] DocumentData *documentFromWeb(
		const MTPWebDocument &data,
		ImagePtr thumb);

	[[nodiscard]] not_null<WebPageData*> webpage(WebPageId id);
	not_null<WebPageData*> processWebpage(const MTPWebPage &data);
	not_null<WebPageData*> processWebpage(const MTPDwebPage &data);
	not_null<WebPageData*> processWebpage(const MTPDwebPagePending &data);
	[[nodiscard]] not_null<WebPageData*> webpage(
		WebPageId id,
		const QString &siteName,
		const TextWithEntities &content);
	[[nodiscard]] not_null<WebPageData*> webpage(
		WebPageId id,
		WebPageType type,
		const QString &url,
		const QString &displayUrl,
		const QString &siteName,
		const QString &title,
		const TextWithEntities &description,
		PhotoData *photo,
		DocumentData *document,
		WebPageCollage &&collage,
		int duration,
		const QString &author,
		TimeId pendingTill);

	[[nodiscard]] not_null<GameData*> game(GameId id);
	not_null<GameData*> processGame(const MTPDgame &data);
	[[nodiscard]] not_null<GameData*> game(
		GameId id,
		const uint64 &accessHash,
		const QString &shortName,
		const QString &title,
		const QString &description,
		PhotoData *photo,
		DocumentData *document);
	void gameConvert(
		not_null<GameData*> original,
		const MTPGame &data);

	[[nodiscard]] not_null<PollData*> poll(PollId id);
	not_null<PollData*> processPoll(const MTPPoll &data);
	not_null<PollData*> processPoll(const MTPDmessageMediaPoll &data);

	[[nodiscard]] not_null<GroupJoinApply*> groupJoinApply(GroupJoinApplyId id);
	not_null<GroupJoinApply*> processGroupJionApply(const MTPDgroupJoinApplyInfo &apply);

	[[nodiscard]] not_null<LocationData*> location(
		const LocationCoords &coords);

	void registerPhotoItem(
		not_null<const PhotoData*> photo,
		not_null<HistoryItem*> item);
	void unregisterPhotoItem(
		not_null<const PhotoData*> photo,
		not_null<HistoryItem*> item);
	void registerDocumentItem(
		not_null<const DocumentData*> document,
		not_null<HistoryItem*> item);
	void unregisterDocumentItem(
		not_null<const DocumentData*> document,
		not_null<HistoryItem*> item);
	void registerWebPageView(
		not_null<const WebPageData*> page,
		not_null<ViewElement*> view);
	void unregisterWebPageView(
		not_null<const WebPageData*> page,
		not_null<ViewElement*> view);
	void registerWebPageItem(
		not_null<const WebPageData*> page,
		not_null<HistoryItem*> item);
	void unregisterWebPageItem(
		not_null<const WebPageData*> page,
		not_null<HistoryItem*> item);
	void registerGameView(
		not_null<const GameData*> game,
		not_null<ViewElement*> view);
	void unregisterGameView(
		not_null<const GameData*> game,
		not_null<ViewElement*> view);
	void registerPollView(
		not_null<const PollData*> poll,
		not_null<ViewElement*> view);
	void unregisterPollView(
		not_null<const PollData*> poll,
		not_null<ViewElement*> view);
	void registerContactView(
		UserId contactId,
		not_null<ViewElement*> view);
	void unregisterContactView(
		UserId contactId,
		not_null<ViewElement*> view);
	void registerContactItem(
		UserId contactId,
		not_null<HistoryItem*> item);
	void unregisterContactItem(
		UserId contactId,
		not_null<HistoryItem*> item);
	void registerAutoplayAnimation(
		not_null<::Media::Clip::Reader*> reader,
		not_null<ViewElement*> view);
	void unregisterAutoplayAnimation(
		not_null<::Media::Clip::Reader*> reader);

	HistoryItem *findWebPageItem(not_null<WebPageData*> page) const;
	QString findContactPhone(not_null<UserData*> contact) const;
	QString findContactPhone(UserId contactId) const;

	void notifyWebPageUpdateDelayed(not_null<WebPageData*> page);
	void notifyGameUpdateDelayed(not_null<GameData*> game);
	void notifyPollUpdateDelayed(not_null<PollData*> poll);
	bool hasPendingWebPageGamePollNotification() const;
	void sendWebPageGamePollNotifications();

	void stopAutoplayAnimations();

	void registerItemView(not_null<ViewElement*> view);
	void unregisterItemView(not_null<ViewElement*> view);

	not_null<Feed*> feed(FeedId id);
	Feed *feedLoaded(FeedId id);
	void setDefaultFeedId(FeedId id);
	FeedId defaultFeedId() const;
	rpl::producer<FeedId> defaultFeedIdValue() const;

	void requestNotifySettings(not_null<PeerData*> peer);
	void applyNotifySetting(
		const MTPNotifyPeer &notifyPeer,
		const MTPPeerNotifySettings &settings);
	void updateNotifySettings(
		not_null<PeerData*> peer,
		std::optional<int> muteForSeconds,
		std::optional<bool> silentPosts = std::nullopt);
	bool notifyIsMuted(
		not_null<const PeerData*> peer,
		crl::time *changesIn = nullptr) const;
	bool notifySilentPosts(not_null<const PeerData*> peer) const;
	bool notifyMuteUnknown(not_null<const PeerData*> peer) const;
	bool notifySilentPostsUnknown(not_null<const PeerData*> peer) const;
	bool notifySettingsUnknown(not_null<const PeerData*> peer) const;
	rpl::producer<> defaultUserNotifyUpdates() const;
	rpl::producer<> defaultChatNotifyUpdates() const;
	rpl::producer<> defaultBroadcastNotifyUpdates() const;
	rpl::producer<> defaultNotifyUpdates(
		not_null<const PeerData*> peer) const;

	void serviceNotification(
		const TextWithEntities &message,
		const MTPMessageMedia &media = MTP_messageMediaEmpty());
	void checkNewAuthorization();
	rpl::producer<> newAuthorizationChecks() const;

	void setMimeForwardIds(MessageIdsList &&list);
	MessageIdsList takeMimeForwardIds();

	void setProxyPromoted(PeerData *promoted);
	PeerData *proxyPromoted() const;

	Groups &groups() {
		return _groups;
	}
	const Groups &groups() const {
		return _groups;
	}

	bool updateWallpapers(const MTPaccount_WallPapers &data);
	void removeWallpaper(const WallPaper &paper);
	const std::vector<WallPaper> &wallpapers() const;
	int32 wallpapersHash() const;

	void clearLocalStorage();

private:
	void suggestStartExport();

	void setupContactViewsViewer();
	void setupChannelLeavingViewer();

	void checkSelfDestructItems();
	int computeUnreadBadge(
		int full,
		int muted,
		int entriesFull,
		int entriesMuted) const;
	bool computeUnreadBadgeMuted(
		int full,
		int muted,
		int entriesFull,
		int entriesMuted) const;

	void photoApplyFields(
		not_null<PhotoData*> photo,
		const MTPPhoto &data);
	void photoApplyFields(
		not_null<PhotoData*> photo,
		const MTPDphoto &data);
	void photoApplyFields(
		not_null<PhotoData*> photo,
		const uint64 &access,
		const QByteArray &fileReference,
		TimeId date,
		int32 dc,
		bool hasSticker,
		const ImagePtr &thumbnailInline,
		const ImagePtr &thumbnailSmall,
		const ImagePtr &thumbnail,
		const ImagePtr &large);

	void documentApplyFields(
		not_null<DocumentData*> document,
		const MTPDocument &data);
	void documentApplyFields(
		not_null<DocumentData*> document,
		const MTPDdocument &data);
	void documentApplyFields(
		not_null<DocumentData*> document,
		const uint64 &access,
		const QByteArray &fileReference,
		TimeId date,
		const QVector<MTPDocumentAttribute> &attributes,
		const QString &mime,
		const ImagePtr &thumbnailInline,
		const ImagePtr &thumbnail,
		int32 dc,
		int32 size,
		const StorageImageLocation &thumbLocation);
	DocumentData *documentFromWeb(
		const MTPDwebDocument &data,
		ImagePtr thumb);
	DocumentData *documentFromWeb(
		const MTPDwebDocumentNoProxy &data,
		ImagePtr thumb);

	void webpageApplyFields(
		not_null<WebPageData*> page,
		const MTPDwebPage &data);
	void webpageApplyFields(
		not_null<WebPageData*> page,
		WebPageType type,
		const QString &url,
		const QString &displayUrl,
		const QString &siteName,
		const QString &title,
		const TextWithEntities &description,
		PhotoData *photo,
		DocumentData *document,
		WebPageCollage &&collage,
		int duration,
		const QString &author,
		TimeId pendingTill);

	void gameApplyFields(
		not_null<GameData*> game,
		const MTPDgame &data);
	void gameApplyFields(
		not_null<GameData*> game,
		const uint64 &accessHash,
		const QString &shortName,
		const QString &title,
		const QString &description,
		PhotoData *photo,
		DocumentData *document);

	bool stickersUpdateNeeded(crl::time lastUpdate, crl::time now) const {
		constexpr auto kStickersUpdateTimeout = crl::time(3600'000);
		return (lastUpdate == 0)
			|| (now >= lastUpdate + kStickersUpdateTimeout);
	}
	void userIsContactUpdated(not_null<UserData*> user);

	void clearPinnedDialogs();
	void setIsPinned(const Dialogs::Key &key, bool pinned);

	NotifySettings &defaultNotifySettings(not_null<const PeerData*> peer);
	const NotifySettings &defaultNotifySettings(
		not_null<const PeerData*> peer) const;
	void unmuteByFinished();
	void unmuteByFinishedDelayed(crl::time delay);
	void updateNotifySettingsLocal(not_null<PeerData*> peer);

	template <typename Method>
	void enumerateItemViews(
		not_null<const HistoryItem*> item,
		Method method);

	void insertCheckedServiceNotification(
		const TextWithEntities &message,
		const MTPMessageMedia &media,
		TimeId date);

	bool sendActionsAnimationCallback(crl::time now);

	void setWallpapers(const QVector<MTPWallPaper> &data, int32 hash);

	not_null<AuthSession*> _session;

	Storage::DatabasePointer _cache;
	Storage::DatabasePointer _bigFileCache;

	std::unique_ptr<Export::Controller> _export;
	std::unique_ptr<Export::View::PanelController> _exportPanel;
	rpl::event_stream<Export::View::PanelController*> _exportViewChanges;
	TimeId _exportAvailableAt = 0;
	QPointer<BoxContent> _exportSuggestion;

	base::Variable<bool> _contactsLoaded = { false };
	base::Variable<bool> _allChatsLoaded = { false };
	base::Observable<void> _moreChatsLoaded;
	base::Observable<ItemVisibilityQuery> _queryItemVisibility;
	rpl::event_stream<IdChange> _itemIdChanges;
	rpl::event_stream<not_null<const HistoryItem*>> _itemLayoutChanges;
	rpl::event_stream<not_null<const ViewElement*>> _viewLayoutChanges;
	rpl::event_stream<not_null<const HistoryItem*>> _itemRepaintRequest;
	rpl::event_stream<not_null<const ViewElement*>> _viewRepaintRequest;
	rpl::event_stream<not_null<const HistoryItem*>> _itemResizeRequest;
	rpl::event_stream<not_null<ViewElement*>> _viewResizeRequest;
	rpl::event_stream<not_null<HistoryItem*>> _itemViewRefreshRequest;
	rpl::event_stream<not_null<HistoryItem*>> _itemTextRefreshRequest;
	rpl::event_stream<not_null<HistoryItem*>> _animationPlayInlineRequest;
	rpl::event_stream<not_null<const HistoryItem*>> _itemRemoved;
	rpl::event_stream<not_null<const ViewElement*>> _viewRemoved;
	rpl::event_stream<not_null<const History*>> _historyUnloaded;
	rpl::event_stream<not_null<const History*>> _historyCleared;
	base::flat_set<not_null<History*>> _historiesChanged;
	rpl::event_stream<not_null<History*>> _historyChanged;
	rpl::event_stream<MegagroupParticipant> _megagroupParticipantRemoved;
	rpl::event_stream<MegagroupParticipant> _megagroupParticipantAdded;
	rpl::event_stream<FeedUpdate> _feedUpdates;

	rpl::event_stream<> _stickersUpdated;
	rpl::event_stream<> _savedGifsUpdated;
	rpl::variable<int> _friendsRequestChanged = 0;
	rpl::variable<int> _groupUnReadCount = 0;
	rpl::event_stream<> _savedGroupsStream;
	crl::time _lastStickersUpdate = 0;
	crl::time _lastRecentStickersUpdate = 0;
	crl::time _lastFavedStickersUpdate = 0;
	crl::time _lastFeaturedStickersUpdate = 0;
	crl::time _lastSavedGifsUpdate = 0;
	rpl::variable<int> _featuredStickerSetsUnreadCount = 0;
	Stickers::Sets _stickerSets;
	Stickers::Order _stickerSetsOrder;
	Stickers::Order _featuredStickerSetsOrder;
	Stickers::Order _archivedStickerSetsOrder;
	Stickers::SavedGifs _savedGifs;

	int _unreadFull = 0;
	int _unreadMuted = 0;
	int _unreadEntriesFull = 0;
	int _unreadEntriesMuted = 0;

	base::Timer _selfDestructTimer;
	std::vector<FullMsgId> _selfDestructItems;

	// When typing in this history started.
	base::flat_map<not_null<History*>, crl::time> _sendActions;
	Ui::Animations::Basic _sendActionsAnimation;

	std::unordered_map<
		PhotoId,
		std::unique_ptr<PhotoData>> _photos;
	std::unordered_map<
		not_null<const PhotoData*>,
		base::flat_set<not_null<HistoryItem*>>> _photoItems;
	std::unordered_map<
		DocumentId,
		std::unique_ptr<DocumentData>> _documents;
	std::unordered_map<
		not_null<const DocumentData*>,
		base::flat_set<not_null<HistoryItem*>>> _documentItems;
	std::unordered_map<
		WebPageId,
		std::unique_ptr<WebPageData>> _webpages;
	std::unordered_map<
		not_null<const WebPageData*>,
		base::flat_set<not_null<HistoryItem*>>> _webpageItems;
	std::unordered_map<
		not_null<const WebPageData*>,
		base::flat_set<not_null<ViewElement*>>> _webpageViews;
	std::unordered_map<
		LocationCoords,
		std::unique_ptr<LocationData>> _locations;
	std::unordered_map<
		PollId,
		std::unique_ptr<PollData>> _polls;
	std::unordered_map<
		GameId,
		std::unique_ptr<GameData>> _games;
	std::unordered_map<
		GroupJoinApplyId,
		std::unique_ptr<GroupJoinApply>> _groupJoinApplies;
	std::unordered_map<
		not_null<const GameData*>,
		base::flat_set<not_null<ViewElement*>>> _gameViews;
	std::unordered_map<
		not_null<const PollData*>,
		base::flat_set<not_null<ViewElement*>>> _pollViews;
	std::unordered_map<
		UserId,
		base::flat_set<not_null<HistoryItem*>>> _contactItems;
	std::unordered_map<
		UserId,
		base::flat_set<not_null<ViewElement*>>> _contactViews;
	base::flat_map<
		not_null<::Media::Clip::Reader*>,
		not_null<ViewElement*>> _autoplayAnimations;

	base::flat_set<not_null<WebPageData*>> _webpagesUpdated;
	base::flat_set<not_null<GameData*>> _gamesUpdated;
	base::flat_set<not_null<PollData*>> _pollsUpdated;

	std::deque<Dialogs::Key> _pinnedDialogs;
	base::flat_map<FeedId, std::unique_ptr<Feed>> _feeds;
	rpl::variable<FeedId> _defaultFeedId = FeedId();
	Groups _groups;
	std::unordered_map<
		not_null<const HistoryItem*>,
		std::vector<not_null<ViewElement*>>> _views;

	PeerData *_proxyPromoted = nullptr;

	NotifySettings _defaultUserNotifySettings;
	NotifySettings _defaultChatNotifySettings;
	NotifySettings _defaultBroadcastNotifySettings;
	rpl::event_stream<> _defaultUserNotifyUpdates;
	rpl::event_stream<> _defaultChatNotifyUpdates;
	rpl::event_stream<> _defaultBroadcastNotifyUpdates;
	std::unordered_set<not_null<const PeerData*>> _mutedPeers;
	base::Timer _unmuteByFinishedTimer;

	std::unordered_map<PeerId, std::unique_ptr<PeerData>> _peers;
	std::unordered_map<PeerId, std::unique_ptr<History>> _histories;
	std::list<not_null<UserData*>> _friendRequests;
	std::list<not_null<PeerData*>> _savedGroups;
	MessageIdsList _mimeForwardIds;

	using CredentialsWithGeneration = std::pair<
		const Passport::SavedCredentials,
		int>;
	std::unique_ptr<CredentialsWithGeneration> _passportCredentials;

	rpl::event_stream<> _newAuthorizationChecks;

	rpl::event_stream<SendActionAnimationUpdate> _sendActionAnimationUpdate;

	std::vector<WallPaper> _wallpapers;
	int32 _wallpapersHash = 0;

	rpl::lifetime _lifetime;

};

} // namespace Data
