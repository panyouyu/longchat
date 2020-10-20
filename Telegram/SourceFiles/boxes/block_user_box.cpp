#include "boxes/block_user_box.h"

#include "mtproto/sender.h"
#include "data/data_user.h"
#include "ui/widgets/input_fields.h"
#include "lang/lang_keys.h"
#include "styles/style_boxes.h"

namespace {
	constexpr auto kMaxBlockReasonLegth = 64;
}

BlockUserBox::BlockUserBox(QWidget*, not_null<UserData*> user)
	: _user(user)
	, _userName(this, 
		st::blockUserNameField,
		Ui::InputField::Mode::SingleLine,
		langFactory(lng_signup_firstname), 
		_user->name)
	, _reason(this, 
		st::blockUserReasonField, 
		Ui::InputField::Mode::MultiLine,
		langFactory(lng_guest_pull_blacked_reason),
		_user->isShieldBlack()
			? _user->shieldBlackReason()
			: QString()){
	_userName->setDisabled(true);
	_reason->setDisabled(_user->isShieldBlack());
}

void BlockUserBox::prepare() {
	setTitle(langFactory(_user->isShieldBlack()
		? lng_guest_pull_blacked_guest
		: lng_guest_pull_black_guest));
	
	if (!_user->isShieldBlack()) {
		addButton(langFactory(lng_guest_pull_black), [=] { submit(); });
	}
	addButton(langFactory(lng_close), [=] { closeBox(); });

	connect(_reason, &Ui::InputField::submitted, [this] { submit(); });

	auto height = st::blockUserPadding.top() + _userName->height() + st::blockUserReasonSkip + _reason->height() + 
		st::blockUserPadding.bottom() + st::boxPadding.bottom();

	if (_reason->isEnabled()) {
		_reason->heightValue(
		) | rpl::start_with_next([=](auto reasonHeight) {
			auto h = st::blockUserPadding.top() + _userName->height() + st::blockUserReasonSkip + 
				reasonHeight + st::blockUserPadding.bottom() + st::boxPadding.bottom();
			setDimensions(st::boxWideWidth, h);
		}, lifetime());
	}

	setDimensions(st::boxWideWidth, height);
}

void BlockUserBox::paintEvent(QPaintEvent* e) {
	BoxContent::paintEvent(e);

	Painter p(this);
	st::contactUserIcon.paint(
		p,
		st::boxPadding.left() + st::contactIconPosition.x(),
		_userName->y() + st::contactIconPosition.y(),
		width());
	st::blockUserReason.paint(
		p,
		st::boxPadding.left() + st::contactIconPosition.x(),
		_reason->y() + st::contactIconPosition.y(),
		width());
}

void BlockUserBox::resizeEvent(QResizeEvent* e) {
	BoxContent::resizeEvent(e);

	auto w = width() - st::boxPadding.left() - st::blockUserPadding.left() - st::boxPadding.right();
	_userName->resizeToWidth(w);
	_reason->resizeToWidth(w);

	auto left = st::boxPadding.left() + st::blockUserPadding.left();
	auto top = st::blockUserPadding.top();
	_userName->moveToLeft(left, top); top += _userName->height() + st::blockUserReasonSkip;
	_reason->moveToLeft(left, top);
}

void BlockUserBox::setInnerFocus() {
	if (!_user->isShieldBlack()) {
		_reason->setFocus();
	}
}

void BlockUserBox::submit() {
	if (_blockId)
		return;

	auto reason = _reason->getLastText().trimmed();
	if (reason.isEmpty() || reason.size() > kMaxBlockReasonLegth) {
		_reason->showError();
		_reason->setFocus();
		return;
	}
	
	_blockId = MTP::send(MTPkefu_BlockUser(MTP_int(_user->bareId()), MTP_string(reason)),
		rpcDone(&BlockUserBox::onBlockDone), rpcFail(&BlockUserBox::onBlockFail));
}

void BlockUserBox::onBlockDone(const MTPBool &result) {
	_blockId = 0;
	if (mtpIsTrue(result)) {
		_user->setShieldBlack(true);
		_user->setShieldBlackReason(_reason->getLastText().trimmed());
		closeBox();
	}
}

bool BlockUserBox::onBlockFail(const RPCError &error) {
	if (MTP::isDefaultHandledError(error)) return false;
	_blockId = 0;
	return true;
}
