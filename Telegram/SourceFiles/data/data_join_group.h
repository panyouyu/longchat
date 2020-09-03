#pragma once

#include "data/data_types.h"

namespace Data {
class Session;
} // namespace Data

class GroupJoinApply {
public:
	GroupJoinApply(
		not_null<Data::Session*> owner, 
		GroupJoinApplyId id)
		: _owner(owner)
		, _id(id) {}

	void setUser(not_null<UserData*> user) {
		_user = user;
	}

	void setChat(not_null<PeerData*> chat) {
		_chat = chat;
	}

	const QString &remark() const {
		return _remark;
	}
	void setRemark(QString& remark) {
		_remark = remark;
	}
	
	enum Status : char {
		Verifing = 0,
		Agreed,
		Refused,
	};
	Status status() {
		return _status;
	}
	void setStatus(Status status) {
		_status = status;
	}

	const QString& verifyUserName() const {
		return _verifyUserName;
	}
	void setVerifyUserName(QString& verifyUserName) {
		_verifyUserName = verifyUserName;
	}
private:
	GroupJoinApplyId _id = 0;
	UserData* _user;
	PeerData* _chat;
	QString _remark;
	Status _status = Verifing;
	QString _verifyUserName;
	not_null<Data::Session*> _owner;
};