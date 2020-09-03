#pragma once

#include "boxes/abstract_box.h"
#include "mtproto/rpc_sender.h"

class GroupJoinApply;
class PeerListController;
class PeerListRow;
namespace Ui {
	class InputField;
}

class HandleGroupJoin : public BoxContent
	, public RPCSender {
public:
	HandleGroupJoin(QWidget*, 
		not_null<PeerListController*> controller,
		not_null<GroupJoinApply*> apply);

protected:
	void prepare() override;

	void paintEvent(QPaintEvent* e) override;
	void resizeEvent(QResizeEvent* e) override;

	void setInnerFocus() override;
private:
	void agree();
	void refuse();

	void onAgreeDone(const MTPGroupJoinVerifyResponse &data);
	bool onAgreeFail(const RPCError &error);

	void onRefuseDone(const MTPGroupJoinVerifyResponse& data);
	bool onRefuseFail(const RPCError& error);

	not_null<PeerListController*> _controller;

	not_null<GroupJoinApply*> _apply;
	object_ptr<Ui::InputField> _applicant;
	object_ptr<Ui::InputField> _group;
	object_ptr<Ui::InputField> _remark;

	mtpRequestId _agreeRequest = 0;
	mtpRequestId _refuseRequest = 0;
	QPointer<BoxContent> _confirmRefuse = nullptr;
};