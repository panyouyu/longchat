#include "introuserlogin.h"

#include "styles/style_intro.h"
#include "styles/style_boxes.h"
#include "core/file_utilities.h"
#include "storage/localstorage.h"
#include "core/core_cloud_password.h"
#include "boxes/confirm_box.h"
#include "lang/lang_keys.h"
#include "intro/introsignup.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/input_fields.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/checkbox.h"
#include "base/openssl_help.h"
#include "intropwdcheck.h"
#include "ui/wrap/fade_wrap.h"

namespace Intro {

UserLoginWidget::UserLoginWidget(QWidget* parent, Widget::Data* data)
	: Step(parent, data)
	, _unameField(this, st::introUserLoginName, langFactory(lng_login_input_username))
	, _pwdField(this, st::introUserLoginName, langFactory(lng_login_input_password))
	, _codeField(this, st::introUserLoginCode, langFactory(lng_login_input_code))
	, _picCode(this, st::introUserLoginPicCode)
	, _changeCode(this, lang(lng_login_code_change)) {
	_checkRequest = new QTimer(this);
	connect(_unameField, SIGNAL(changed()), this, SLOT(onInputUnameChange()));
	connect(_pwdField, SIGNAL(changed()), this, SLOT(onInputPwdChange()));
	connect(_changeCode, SIGNAL(clicked()), this, SLOT(onChangeCode()));
	connect(_unameField, &Ui::InputField::submitted, this, [this] { submit(); });
	connect(_pwdField, &Ui::PasswordInput::submitted, this, [this] { submit(); });
	connect(_codeField, &Ui::InputField::submitted, this, [this] { submit(); });

	connect(_checkRequest, SIGNAL(timeout()), this, SLOT(onCheckRequest()));
	hideDescription();
	initData();

	_imageLogo = QImageReader(":/gui/icons/pic_signin_logo.png", Q_NULLPTR).read();
	setMouseTracking(true);
}

UserLoginWidget::~UserLoginWidget() {
}

void UserLoginWidget::setInnerFocus() {
	_unameField->setFocus();
}

void UserLoginWidget::onInputPwdChange() {
	hideError();
}

void UserLoginWidget::onChangeCode() {
	_picCode->onReflushVerification();
	_codeField->setFocus();
}

void UserLoginWidget::onInputUnameChange() {
	hideError();
}

void UserLoginWidget::onCheckRequest() {
	auto status = MTP::state(_kefuLoginRequest);
	if (status < 0) {
		auto leftms = -status;
		if (leftms >= 1000) {
			if (_kefuLoginRequest) {
				MTP::cancel(base::take(_kefuLoginRequest));
			}
		}
	}
	if (!_kefuLoginRequest && status == MTP::RequestSent) {
		stopCheck();
	}
}

void UserLoginWidget::updateControlsGeometry() {
	_unameField->moveToLeft(contentLeft(), contentTop() + st::introUserLoginNameTop);
	_pwdField->moveToLeft(contentLeft(), contentTop() + st::introUserLoginPwdTop);
	_codeField->moveToLeft(contentLeft(), contentTop() + st::introUserLoginCodeTop);
	_picCode->moveToLeft(contentLeft() + st::introUserLoginCodePicLeft, contentTop() + st::introUserLoginCodeTop );
	_changeCode->moveToLeft(contentLeft() + st::introUserLoginCodeChangeLeft, contentTop() + st::introUserLoginCodeTop + st::introUserLoginCodeMarginTop);
	auto remUserTop = contentTop() + st::introUserLoginCodeTop;
	remUserTop += _codeField->height();
}

void UserLoginWidget::initData() {
	if (Global::RemberUserName()) {
		_unameField->setText(Global::KefuUserName());
	}
}

void UserLoginWidget::saveSets() {
	if (Global::RemberUserName()) {
		Global::SetKefuUserName(_unameField->getLastText());
		Local::writeUserSettings();
	}
}

void UserLoginWidget::activate() {	
	Step::activate();
}

void UserLoginWidget::cancelled() {
	MTP::cancel(base::take(_kefuLoginRequest));
}

void UserLoginWidget::finished() {
	Step::finished();
	_checkRequest->stop();
	rpcInvalidate();
	cancelled();
}

void UserLoginWidget::codeSubmitDone(const MTPauth_Authorization& result) {
	_kefuLoginRequest = 0;
	auto& d = result.c_auth_authorization();
	if (d.vuser.type() != mtpc_user || !d.vuser.c_user().is_self()) { // wtf?
		showCodeError(&Lang::Hard::ServerError);
		return;
	}
	QString phone = qs(d.vuser.c_user().vphone);
	cSetLoggedPhoneNumber(phone);
	
	finish(d.vuser);
}

bool UserLoginWidget::codeSubmitFail(const RPCError& error) {
	if (MTP::isFloodError(error)) {
		stopCheck();
		_kefuLoginRequest = 0;
		showCodeError(langFactory(lng_flood_error));
		return true;
	}
	if (MTP::isDefaultHandledError(error)) return false;

	stopCheck();
	_kefuLoginRequest = 0;
	auto& err = error.type();
	if (err == qstr("PASSWORD_WRONG")) {
		showCodeError(langFactory(lng_signin_bad_password), [&] {
			_pwdField->showError();
			_pwdField->setFocus();
		});		
		return true;
	} else if (err == qstr("USERNAME_INVALID")) {
		showCodeError(langFactory(lng_signin_username_invalid), [&] {
			_unameField->showError();
			_unameField->setFocus();
		});
		return true;
	} else if (err == qstr("USERNAME_FREEZED")) {
		showCodeError(langFactory(lng_signin_username_freezed), [&] {
			_unameField->showError();
			_unameField->setFocus();
		});
		return true;
	}
	if (Logs::DebugEnabled()) { // internal server error
		auto text = err + ": " + error.description();
		showCodeError([text] { return text; });
	}
	else {
		showCodeError(&Lang::Hard::ServerError);
	}

	return false;
}

void UserLoginWidget::showCodeError(Fn<QString()> textFactory, Fn<void()> closedCallback) {
	Ui::show(Box<InformBox>(textFactory(), closedCallback));
}

void UserLoginWidget::stopCheck() {
	_checkRequest->stop();
}

void UserLoginWidget::submit() {
	if (_kefuLoginRequest) {
		return;
	}
	if (_unameField->getLastText().isEmpty()) {
		_unameField->showError();
		_unameField->setFocus();
		return;
	}
	if (_pwdField->getLastText().isEmpty()) {
		_pwdField->showError();
		_pwdField->setFocus();
		return;
	}
	if (_codeField->getLastText().isEmpty()) {
		_codeField->showError();
		_codeField->setFocus();
		return;
	}
	if (QString::compare(_picCode->getVerificationCode(), _codeField->getLastText(), Qt::CaseInsensitive) != 0) {
		showCodeError(langFactory(lng_login_input_code_error), [&] {
			_codeField->showError();
			_codeField->setFocus();
		});		
		return;
	}

	QString userName = _unameField->getLastText();
	QString userPwd = _pwdField->getLastText();

	char h[33] = { 0 };
	QByteArray dataPwd = userPwd.toLatin1();
	hashMd5Hex(dataPwd.constData(), dataPwd.size(), h);
	
	_checkRequest->start(1000);

	_kefuLoginRequest = MTP::send(MTPauth_KefuLogin(MTP_string(userName), MTP_string(h), MTP_string(_picCode->getVerificationCode())),
		rpcDone(&UserLoginWidget::codeSubmitDone), 
		rpcFail(&UserLoginWidget::codeSubmitFail));
}

void UserLoginWidget::resizeEvent(QResizeEvent* e) {
	Step::resizeEvent(e);
	updateControlsGeometry();
}

void UserLoginWidget::paintEvent(QPaintEvent* e) {
	QPainter p(this);
	auto x = contentLeft() + st::introUserLoginLogoIconLeft;
	auto y = contentTop();
	p.setCompositionMode(QPainter::CompositionMode_SourceOver);
	p.drawImage(x, y, _imageLogo);
}

QString UserLoginWidget::nextButtonText() const {
	return lang(lng_login_login);
}

} // namespace Intro
