#include "boxes/handle_friend_request_box.h"

#include "styles/style_boxes.h"
#include "styles/style_dialogs.h"
#include "lang/lang_keys.h"
#include "data/data_user.h"
#include "data/data_session.h"
#include "ui/widgets/input_fields.h"
#include "ui/toast/toast.h"
#include "boxes/confirm_box.h"
#include "base/assertion.h"
#include "auth_session.h"
#include "apiwrap.h"

namespace {
enum State {
	Accept = 1,
	Refuse = 2
};
enum Response {
	Success = 0,
	Fail = 1
};
}

HandleFriendRequestBox::HandleFriendRequestBox(QWidget*, not_null<UserData*> user)
: _user(user)
, _first(this, st::defaultInputField, langFactory(lng_signup_firstname), user->firstName)
, _verify_info(
	this, 
	st::defaultInputField, 
	Ui::InputField::Mode::MultiLine, 
	langFactory(lng_friend_request_verify_info),
	user->verifyInfo()) {
	_verify_info->setEnabled(false);
	_verify_info->setVisible(!user->verifyInfo().isEmpty());
}

void HandleFriendRequestBox::prepare() {
	setTitle(langFactory(lng_friend_request_add_contact));

	addButton(langFactory(lng_friend_request_accept), [this] { accept(); });
	addButton(langFactory(lng_friend_request_refuse), [this] { 
		_confirmRefuse = Ui::show(Box<ConfirmBox>(lng_friend_request_verify_refuse(lt_name, _user->name),
			[=] {
				refuse();
				if (_confirmRefuse) {
					_confirmRefuse->closeBox();
				}				
			}), LayerOption::KeepOther);
	});
	addLeftButton(langFactory(lng_cancel), [this] {closeBox(); });

	connect(_first, &Ui::InputField::submitted, [=] { accept(); });

	auto height = st::contactPadding.top() + _first->height() + st::contactPadding.bottom() + st::boxPadding.bottom();
	if (!_verify_info->isHidden()) {
		height += st::contactPhoneSkip + _verify_info->height();
	}
	setDimensions(st::boxWideWidth, height);
}

void HandleFriendRequestBox::paintEvent(QPaintEvent* e) {
	BoxContent::paintEvent(e);

	Painter p(this);

	st::contactUserIcon.paint(
		p,
		st::boxPadding.left() + st::contactIconPosition.x(),
		_first->y() + st::contactIconPosition.y(),
		width());
	if (!_verify_info->isHidden()) {
		st::friendRequestVerifyInfoIcon.paint(
			p,
			st::boxPadding.left() + st::contactIconPosition.x(),
			_verify_info->y() + st::contactIconPosition.y(),
			width());
	}
}

void HandleFriendRequestBox::resizeEvent(QResizeEvent* e) {
	BoxContent::resizeEvent(e);

	_first->resize(width() - st::boxPadding.left() - st::contactPadding.left() - st::boxPadding.right(), _first->height());
	_verify_info->resize(_first->width(), _verify_info->height());
	_first->moveToLeft(st::boxPadding.left() + st::contactPadding.left(), st::contactPadding.top());
	if (!_verify_info->isHidden()) {
		_verify_info->moveToLeft(st::boxPadding.left() + st::contactPadding.left(), _first->y() + _first->height() + st::contactPhoneSkip);
	}
}

void HandleFriendRequestBox::keyPressEvent(QKeyEvent* e) {
	if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
		accept();
	} else {
		BoxContent::keyPressEvent(e);
	}
}

void HandleFriendRequestBox::setInnerFocus() {
	_first->setFocusFast();
}

void HandleFriendRequestBox::accept() {
	if (_acceptRequest) return;

	auto firstName = TextUtilities::PrepareForSending(_first->getLastText());
	if (firstName.isEmpty()) {
		_first->setFocus();
		_first->showError();
		return;
	}

	auto request = MTPcontacts_HandelFriendRequest();
	request.vuser_id = MTP_long(_user->id);
	request.vstate = MTP_int(State::Accept);
	request.vtag_ids.v.reserve(0);
	request.vfirst_name = MTP_string(firstName);
	_acceptRequest = MTP::send(
		request,
		rpcDone(&HandleFriendRequestBox::onAcceptDone),
		rpcFail(&HandleFriendRequestBox::onAcceptFail));
}

void HandleFriendRequestBox::refuse() {
	if (_refuseRequest) return;

	auto request = MTPcontacts_HandelFriendRequest();
	request.vuser_id = MTP_long(_user->id);
	request.vstate = MTP_int(State::Refuse);
	request.vtag_ids.v.reserve(0);
	request.vfirst_name = MTP_string(qsl(""));
	_refuseRequest = MTP::send(
		request,
		rpcDone(&HandleFriendRequestBox::onRefuseDone),
		rpcFail(&HandleFriendRequestBox::onRefuseFail));
}

void HandleFriendRequestBox::onAcceptDone(
	const MTPcontacts_HandleFriendRequestResponse &response) {
	auto &result = response.c_contacts_handleFriendRequestResponse().vsuccess.v;
	if (result == int32(Response::Success)) {
		_user->setContactStatus(UserData::ContactStatus::Contact);
		_user->setVerifyStatus(UserData::VerifyStatus::Accepted);
		Ui::showPeerHistory(_user, ShowAtTheEndMsgId);
	} else if (result == int32(Response::Fail)) {
		_acceptRequest = 0;
		Ui::Toast::Show(lang(lng_friend_request_accept_fail));
	} else {
		Unexpected("HandleFriendRequestBox::onAcceptDone");
	}
}

bool HandleFriendRequestBox::onAcceptFail(const RPCError &error) {
	if (MTP::isDefaultHandledError(error)) return false;

	_acceptRequest = 0;
	return true;
}

void HandleFriendRequestBox::onRefuseDone(
	const MTPcontacts_HandleFriendRequestResponse &response) {
	auto &result = response.c_contacts_handleFriendRequestResponse().vsuccess.v;
	if (result == int32(Response::Success)) {
		_user->setContactStatus(UserData::ContactStatus::CanAdd);
		_user->setVerifyStatus(UserData::VerifyStatus::Refused);
		closeBox();
	} else if (result == int32(Response::Fail)) {
		_refuseRequest = 0;
		Ui::Toast::Show(lang(lng_friend_request_refuse_fail));
	} else {
		Unexpected("HandleFriendRequestBox::onRefuseDone");
	}
}

bool HandleFriendRequestBox::onRefuseFail(const RPCError &error) {
	if (MTP::isDefaultHandledError(error)) return false;

	_refuseRequest = 0;
	return true;
}
