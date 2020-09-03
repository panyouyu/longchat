#pragma once

#include "data/data_types.h"

namespace Data {
class Session;
} // namespace Data

class GroupJoinApply {
public:
	GroupJoinApply(
		GroupJoinApplyId id)
		: _id(id) {}

	GroupJoinApplyId id() const {
		return _id;
	}

	void setApplicant(not_null<UserData*> applicant) {
		_applicant = applicant;
	}
	not_null<UserData*> applicant() const {
		Ensures(_applicant != nullptr);
		return _applicant;
	}

	void setGroup(not_null<PeerData*> group) {		
		_group = group;
	}
	not_null<PeerData*> group() const {
		Ensures(_group != nullptr);
		return _group;
	}

	const QString &remark() const {
		return _remark;
	}
	void setRemark(QString&& remark) {
		_remark = remark;
	}
	
	enum Status : char {
		Verifing = 0,
		Accepted,
		Refused,
		Invalid,
	};
	Status status() const {
		return _status;
	}
	void setStatus(Status status) {
		_status = status;
	}

	const QString& verifyUserName() const {
		return _verifyUserName;
	}
	void setVerifyUserName(QString&& verifyUserName) {
		_verifyUserName = verifyUserName;
	}
private:
	GroupJoinApplyId _id = 0;
	UserData* _applicant = nullptr;
	PeerData* _group = nullptr;
	QString _remark;
	Status _status = Verifing;
	QString _verifyUserName;	
};