/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "data/data_peer.h"

class BotCommand {
public:
	BotCommand(const QString &command, const QString &description);

	bool setDescription(const QString &description);
	const Text &descriptionText() const;

	QString command;

private:
	QString _description;
	mutable Text _descriptionText;

};

struct BotInfo {
	bool inited = false;
	bool readsAllHistory = false;
	bool cantJoinGroups = false;
	int version = 0;
	QString description, inlinePlaceholder;
	QList<BotCommand> commands;
	Text text = Text{ int(st::msgMinWidth) }; // description

	QString startToken, startGroupToken, shareGameShortName;
	PeerId inlineReturnPeerId = 0;
};

struct LabelInfo {
	QString label_name;
	QString label_desc;
	UserId from_id;
	bool operator<(const LabelInfo& label) const {
		return label_name < label.label_name;
	}
	bool operator==(const LabelInfo& label) const {
		return label_name == label.label_name;
	}

};

class UserData : public PeerData {
public:
	static constexpr auto kEssentialFlags = 0
		| MTPDuser::Flag::f_self
		| MTPDuser::Flag::f_contact
		| MTPDuser::Flag::f_mutual_contact
		| MTPDuser::Flag::f_deleted
		| MTPDuser::Flag::f_bot
		| MTPDuser::Flag::f_bot_chat_history
		| MTPDuser::Flag::f_bot_nochats
		| MTPDuser::Flag::f_verified
		| MTPDuser::Flag::f_restricted
		| MTPDuser::Flag::f_bot_inline_geo;
	using Flags = Data::Flags<
		MTPDuser::Flags,
		kEssentialFlags.value()>;

	static constexpr auto kEssentialFullFlags = 0
		| MTPDuserFull::Flag::f_blocked
		| MTPDuserFull::Flag::f_phone_calls_available
		| MTPDuserFull::Flag::f_phone_calls_private;
	using FullFlags = Data::Flags<
		MTPDuserFull::Flags,
		kEssentialFullFlags.value()>;

	UserData(not_null<Data::Session*> owner, PeerId id);
	void setPhoto(const MTPUserProfilePhoto &photo);

	void setName(
		const QString &newFirstName,
		const QString &newLastName,
		const QString &newPhoneName,
		const QString &newUsername);

	void setGroup(const QString& groupInfos);

	void setPhone(const QString &newPhone);

	void setUserInfo(const QList<QPair<QString, QStringList>> &userInfo);
	void setLabels(const QVector<MTPUserLabelData> &labels);
	void addLabel(const LabelInfo& label);
	void removeLabel(const LabelInfo& label);
	void setUrl(const QString &url);

	void setBotInfoVersion(int version);
	void setBotInfo(const MTPBotInfo &info);

	void setNameOrPhone(const QString &newNameOrPhone);

	void madeAction(TimeId when); // pseudo-online

	uint64 accessHash() const {
		return _accessHash;
	}
	void setAccessHash(uint64 accessHash);

	void setFlags(MTPDuser::Flags which) {
		_flags.set(which);
	}
	void addFlags(MTPDuser::Flags which) {
		_flags.add(which);
	}
	void removeFlags(MTPDuser::Flags which) {
		_flags.remove(which);
	}
	auto flags() const {
		return _flags.current();
	}
	auto flagsValue() const {
		return _flags.value();
	}

	void setFullFlags(MTPDuserFull::Flags which) {
		_fullFlags.set(which);
	}
	void addFullFlags(MTPDuserFull::Flags which) {
		_fullFlags.add(which);
	}
	void removeFullFlags(MTPDuserFull::Flags which) {
		_fullFlags.remove(which);
	}
	auto fullFlags() const {
		return _fullFlags.current();
	}
	auto fullFlagsValue() const {
		return _fullFlags.value();
	}

	bool isVerified() const {
		return flags() & MTPDuser::Flag::f_verified;
	}
	bool isBotInlineGeo() const {
		return flags() & MTPDuser::Flag::f_bot_inline_geo;
	}
	bool isBot() const {
		return botInfo != nullptr;
	}
	bool isSupport() const {
		return flags() & MTPDuser::Flag::f_support;
	}
	bool isInaccessible() const {
		constexpr auto inaccessible = 0
			| MTPDuser::Flag::f_deleted;
//			| MTPDuser_ClientFlag::f_inaccessible;
		return flags() & inaccessible;
	}
	bool canWrite() const {
		// Duplicated in Data::CanWriteValue().
		return !isInaccessible();
	}
	bool isContact() const {
		return (_contactStatus == ContactStatus::Contact);
	}

	bool canShareThisContact() const;
	bool canAddContact() const {
		return canShareThisContact() && !isContact();
	}

	// In Data::Session::processUsers() we check only that.
	// When actually trying to share contact we perform
	// a full check by canShareThisContact() call.
	bool canShareThisContactFast() const {
		return !_phone.isEmpty();
	}

	MTPInputUser inputUser;

	QString firstName;
	QString lastName;
	QString username;
	QString groupInfo;

	const QString &phone() const {
		return _phone;
	}
	const QList<QPair<QString, QStringList>>& userInfo() {
		return std::ref(_userInfo);
	}

	const auto& labels() const {
		return _labels;
	}
	const QString& url() const {
		return _url;
	}

	QString nameOrPhone;
	Text phoneText;
	TimeId onlineTill = 0;

	enum class ContactStatus : char {
		PhoneUnknown,
		CanAdd,
		Contact,
	};
	ContactStatus contactStatus() const {
		return _contactStatus;
	}
	void setContactStatus(ContactStatus status);

	bool isShieldBlack() const {
		return (_shieldBlack == true);
	}

	void setShieldBlack(bool flag);

	enum class BlockStatus : char {
		Unknown,
		Blocked,
		NotBlocked,
	};
	BlockStatus blockStatus() const {
		return _blockStatus;
	}
	bool isBlocked() const {
		return (blockStatus() == BlockStatus::Blocked);
	}
	void setBlockStatus(BlockStatus blockStatus);

	enum class CallsStatus : char {
		Unknown,
		Enabled,
		Disabled,
		Private,
	};
	CallsStatus callsStatus() const {
		return _callsStatus;
	}
	bool hasCalls() const;
	void setCallsStatus(CallsStatus callsStatus);

	std::unique_ptr<BotInfo> botInfo;

	QString unavailableReason() const override;
	void setUnavailableReason(const QString &reason);

	int commonChatsCount() const {
		return _commonChatsCount;
	}
	void setCommonChatsCount(int count);

private:
	Flags _flags;
	FullFlags _fullFlags;

	QString _unavailableReason;
	QString _phone;
	QList<QPair<QString, QStringList>> _userInfo;
	QVector<LabelInfo> _labels;
	QString _url;
	ContactStatus _contactStatus = ContactStatus::PhoneUnknown;
	BlockStatus _blockStatus = BlockStatus::Unknown;
	bool _shieldBlack = false;
	CallsStatus _callsStatus = CallsStatus::Unknown;
	int _commonChatsCount = 0;

	uint64 _accessHash = 0;
	static constexpr auto kInaccessibleAccessHashOld
		= 0xFFFFFFFFFFFFFFFFULL;

};
