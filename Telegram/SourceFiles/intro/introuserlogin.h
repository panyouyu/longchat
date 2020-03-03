#pragma once
#include "intro/introwidget.h"

namespace Ui {
	class IconButton;
	class InputField;
	class PasswordInput;
	class RoundButton;
	class LinkButton;
	class Checkbox;
	class FlatLabel;
	class LabelVerificationCode;
	template <typename Widget>
	class FadeWrap;
} // namespace Ui

namespace Intro {
	class UserLoginWidget : public Widget::Step {
		Q_OBJECT

	public:
		UserLoginWidget(QWidget* parent, Widget::Data* data);
		~UserLoginWidget();

		virtual void setInnerFocus() override;


		virtual void activate() override;


		virtual void cancelled() override;


		virtual void finished() override;


		virtual void submit() override;

		virtual QString nextButtonText() const override;

	protected:
		virtual void resizeEvent(QResizeEvent* e) override;


		virtual void paintEvent(QPaintEvent* e) override;

	private slots:
		void onInputPwdChange();
		void onChangeCode();
		void onInputUnameChange();
		void onCheckRequest();

	private:
		void refreshLang();
		void updateControlsGeometry();
		void initData();
		void saveSets();

		void codeSubmitDone(const MTPauth_Authorization& result);
		bool codeSubmitFail(const RPCError& error);
		void showCodeError(Fn<QString()> textFactory);
		void stopCheck();

		QImage _imageLogo;
		object_ptr<Ui::InputField> _unameField;//用户名输入框
		object_ptr<Ui::PasswordInput> _pwdField;//密码输入框
		object_ptr<Ui::InputField> _codeField;//验证码输入框
		object_ptr<Ui::LabelVerificationCode> _picCode;//图片验证码
		//object_ptr<Ui::FlatLabel> _picCode;//图片验证码
		object_ptr<Ui::LinkButton> _changeCode; //换一个
		

		mtpRequestId _kefuLoginRequest = 0;

		//object_ptr<QTimer> _checkRequest;
		QTimer* _checkRequest;

	};
}// namespace Intro


