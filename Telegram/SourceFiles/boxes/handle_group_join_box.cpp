#include "boxes/handle_group_join_box.h"

#include "data/data_group_join_apply.h"
#include "data/data_user.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "ui/widgets/input_fields.h"
#include "ui/toast/toast.h"
#include "boxes/confirm_box.h"
#include "boxes/peer_list_controllers.h"
#include "boxes/peer_list_box.h"
#include "auth_session.h"
#include "apiwrap.h"
#include "lang/lang_keys.h"
#include "styles/style_boxes.h"
#include "styles/style_widgets.h"

namespace{
	constexpr auto kRemarkMaxHeight = 50;
	enum : int32 {
		Agree = 1,
		Refuse = 2,
	};
	enum ResponseType : int32 {
		Succeed = 0,
		HaveVerified = 1,
		InvalidId = 2,
	};
}

HandleGroupJoin::HandleGroupJoin(QWidget*, 
	not_null<PeerListController*> controller,
	not_null<GroupJoinApply*> apply)
	: BoxContent()
	, _controller(controller)
	, _apply(apply)
	, _applicant(this, st::defaultInputField, langFactory(lng_group_join_applicant), apply->applicant()->name)
	, _group(this, st::defaultInputField, langFactory(lng_group_join_group), apply->group()->name)
	, _remark(this, st::defaultInputField, Ui::InputField::Mode::MultiLine, langFactory(lng_group_join_remark), apply->remark()) {
	_applicant->setEnabled(false);
	_group->setEnabled(false);
	_remark->setEnabled(false);
	_remark->setMaxHeight(kRemarkMaxHeight);
}

void HandleGroupJoin::prepare() {
	setTitle(langFactory(lng_group_join_request));

	addButton(langFactory(lng_group_join_agree), [this] { agree(); });
	addButton(langFactory(lng_group_join_refuse), [this] {
		_confirmRefuse = Ui::show(Box<ConfirmBox>(lng_group_join_verify_refuse(lt_name, _apply->applicant()->name),
			[=] {
				refuse();
				if (_confirmRefuse) {
					_confirmRefuse->closeBox();
				}
			}), LayerOption::KeepOther);
		});
	addLeftButton(langFactory(lng_cancel), [this] { closeBox(); });

	_remark->resizeToWidth(st::boxWideWidth - st::boxPadding.left() - st::contactPadding.left() - st::boxPadding.right());
	auto height = st::boxPadding.top() + st::contactPadding.top() + _applicant->height();
	height += st::contactPhoneSkip + _group->height();
	height += st::contactPhoneSkip + _remark->height() + st::contactPadding.bottom() + st::boxPadding.bottom();
	setDimensions(st::boxWideWidth, height);
}

void HandleGroupJoin::paintEvent(QPaintEvent* e) {
	BoxContent::paintEvent(e);

	Painter p(this);
	auto left = st::boxPadding.left() + st::contactIconPosition.x();
	st::contactUserIcon.paint(
		p,
		left,
		_applicant->y() + st::contactIconPosition.y(),
		width());
	st::groupJoinChatIcon.paint(
		p,
		left,
		_group->y() + st::contactIconPosition.y(),
		width());
	st::friendRequestVerifyInfoIcon.paint(
		p,
		left,
		_remark->y() + st::contactIconPosition.y(),
		width());
}

void HandleGroupJoin::resizeEvent(QResizeEvent* e) {
	BoxContent::resizeEvent(e);

	auto left = st::boxPadding.left() + st::contactPadding.left();
	auto top = st::boxPadding.top() + st::contactPadding.top();
	auto w = width() - st::boxPadding.left() - st::contactPadding.left() - st::boxPadding.right();

	_applicant->resize(w, _applicant->height());
	_applicant->moveToLeft(left, top);
	top += _applicant->height() + st::contactPhoneSkip;

	_group->resize(w, _group->height());
	_group->moveToLeft(left, top);
	top += _group->height() + st::contactPhoneSkip;

	_remark->moveToLeft(left, top);
}

void HandleGroupJoin::setInnerFocus() {
}

void HandleGroupJoin::agree() {
	if (_agreeRequest) return;

	_agreeRequest = MTP::send(MTPaccount_GroupJoinVerify(
		MTP_flags(MTPaccount_groupJoinVerify::Flags(0)),
		MTP_int(_apply->id()),
		MTP_int(Agree)),
		rpcDone(&HandleGroupJoin::onAgreeDone),
		rpcFail(&HandleGroupJoin::onAgreeFail));
}

void HandleGroupJoin::refuse() {
	if (_refuseRequest) return;

	_refuseRequest = MTP::send(MTPaccount_GroupJoinVerify(
		MTP_flags(MTPaccount_groupJoinVerify::Flags(0)),
		MTP_int(_apply->id()),
		MTP_int(Refuse)),
		rpcDone(&HandleGroupJoin::onRefuseDone),
		rpcFail(&HandleGroupJoin::onRefuseFail));
}

void HandleGroupJoin::onAgreeDone(const MTPGroupJoinVerifyResponse& data) {
	_agreeRequest = 0;
	data.match([=](const MTPDgroupJoinVerifyResponse& response) {
		auto code = response.vcode.v;
		if (code == ResponseType::Succeed) {
			_apply->setStatus(GroupJoinApply::Accepted);
			_apply->setVerifyUserName(std::move(Auth().user()->name));
		} else if (code == ResponseType::HaveVerified) {
			_apply->setStatus(GroupJoinApply::Accepted);
			Ui::Toast::Show(lang(lng_group_join_have_verified));
		} else if (code == ResponseType::InvalidId) {
			_apply->setStatus(GroupJoinApply::Invalid);
			Ui::Toast::Show(lang(lng_group_join_invalid));
		} else {
			Unexpected("unexpected response code in GroupJoinVerifyResponse");
		}		
	});
	static_cast<ChatsNotifyBoxController*>(_controller.get())->refreshRow(_apply);
	closeBox();
	auto count = Auth().data().groupUnReadCount();
	Auth().data().setGroupUnReadCount(--count);
}

bool HandleGroupJoin::onAgreeFail(const RPCError& error) {
	if (MTP::isDefaultHandledError(error)) 
		return false;

	_agreeRequest = 0;
	return true;
}

void HandleGroupJoin::onRefuseDone(const MTPGroupJoinVerifyResponse& data) {
		data.match([=](const MTPDgroupJoinVerifyResponse& response) {
		auto code = response.vcode.v;
		if (code == ResponseType::Succeed) {
			_apply->setStatus(GroupJoinApply::Refused);
			_apply->setVerifyUserName(std::move(Auth().user()->name));
		} else if (code == ResponseType::HaveVerified) {
			_apply->setStatus(GroupJoinApply::Refused);
			Ui::Toast::Show(lang(lng_group_join_have_verified));
		} else if (code == ResponseType::InvalidId) {
			_apply->setStatus(GroupJoinApply::Invalid);
			Ui::Toast::Show(lang(lng_group_join_invalid));
		} else {
			Unexpected("unexpected response code in GroupJoinVerifyResponse");
		}
	});
	static_cast<ChatsNotifyBoxController*>(_controller.get())->refreshRow(_apply);
	closeBox();
	auto count = Auth().data().groupUnReadCount();
	Auth().data().setGroupUnReadCount(--count);
}

bool HandleGroupJoin::onRefuseFail(const RPCError& error) {
	if (MTP::isDefaultHandledError(error)) 
		return false;

	_refuseRequest = 0;
	return true;
}
