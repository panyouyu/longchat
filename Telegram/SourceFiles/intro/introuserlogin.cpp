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
	, _unameField(this, st::introUserLoginName, langFactory(lng_login_input_email))
	, _pwdField(this, st::introUserLoginName, langFactory(lng_login_input_password))
	, _codeField(this, st::introUserLoginCode, langFactory(lng_login_input_code))
	, _picCode(this, st::introUserLoginPicCode)
	, _changeCode(this, lang(lng_login_code_change))
	, _remberUser(this, lang(lng_login_keep_pwd), false, st::introUserLoginCheckbox)
{

			subscribe(Lang::Current().updated(), [this] { refreshLang(); });

			connect(_unameField, SIGNAL(changed()), this, SLOT(onInputUnameChange()));
			connect(_pwdField, SIGNAL(changed()), this, SLOT(onInputPwdChange()));
			connect(_changeCode, SIGNAL(clicked()), this, SLOT(onChangeCode()));
			connect(_remberUser, SIGNAL(clicked()), this, SLOT(onRemberUser()));
			hideDescription();
			//setTitleText(langFactory(lng_login_input_title));
			
			//setErrorBelowLink(true);
			initData();
			
			_imageLogo = QImageReader(":/gui/icons/pic_signin_logo.png", Q_NULLPTR).read();
			setMouseTracking(true);
}



void UserLoginWidget::setInnerFocus()
{

}

void UserLoginWidget::onInputPwdChange()
{
	hideError();
}

void UserLoginWidget::onChangeCode()
{
	_picCode->onReflushVerification();
}

void UserLoginWidget::onRemberUser()
{
		//Ui::show(Box<InformBox>(QString("%d").arg(_remberUser->checked())));
	Global::SetRemberUserName(_remberUser->checked());
	Local::writeUserSettings();
}

void UserLoginWidget::onInputUnameChange()
{
	hideError();
}





void UserLoginWidget::refreshLang()
{
	updateControlsGeometry();
}


void UserLoginWidget::updateControlsGeometry()
{
	_unameField->moveToLeft(contentLeft(), contentTop() + st::introUserLoginNameTop);
	_pwdField->moveToLeft(contentLeft(), contentTop() + st::introUserLoginPwdTop);
	_codeField->moveToLeft(contentLeft(), contentTop() + st::introUserLoginCodeTop);
	_picCode->moveToLeft(contentLeft() + st::introUserLoginCodePicLeft, contentTop() + st::introUserLoginCodeTop );
	_changeCode->moveToLeft(contentLeft() + st::introUserLoginCodeChangeLeft, contentTop() + st::introUserLoginCodeTop + st::introUserLoginCodeMarginTop);
	auto remUserTop = contentTop() + st::introUserLoginCodeTop;
	remUserTop += _codeField->height();
	//remUserTop += st::introUserLoginRemberUserTop; //³¬³ö´°¿Ú·¶Î§
	_remberUser->moveToLeft(contentLeft() + st::introUserLoginCodePicLeft + st::introUserLoginCodeMarginTop, remUserTop);//introUserLoginRemberUserLeft
	//qDebug() << "rect.height:" << rect().height() << "" << rect().width() << remUserTop;
	
	
}

void UserLoginWidget::initData()
{
	_remberUser->setChecked(Global::RemberUserName());
}

void UserLoginWidget::activate()
{
	
	Step::activate();
}

void UserLoginWidget::cancelled()
{
}

void UserLoginWidget::finished()
{
	Step::finished();
	//_checkRequest->stop();
	rpcInvalidate();
	cancelled();
}

void UserLoginWidget::submit()
{
	//goNext(new Intro::PwdCheckWidget(parentWidget(), getData()));
	hideError();


	
}


void UserLoginWidget::resizeEvent(QResizeEvent* e)
{
	Step::resizeEvent(e);
	updateControlsGeometry();
}


void UserLoginWidget::paintEvent(QPaintEvent* e)
{
	QPainter p(this);
	auto x = contentLeft() + st::introUserLoginLogoIconLeft;
	auto y = contentTop();
	//st::introUserLoginLogoIcon.paint(p, x, y, st::introUserLoginLogoIconWidth);
	p.setCompositionMode(QPainter::CompositionMode_SourceOver);
	//st::introUserLoginLogoIcon.instance(Qt::white);
	p.drawImage(x, y, _imageLogo);
	//p.drawPixmap(x, y, _thumb);
	//qDebug() << x << y << rect().width() << rect().height();
}

QString UserLoginWidget::nextButtonText() const
{
	return lang(lng_login_login);
}

} // namespace Intro
