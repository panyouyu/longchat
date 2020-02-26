#pragma once
#include "intro/introwidget.h"
#include "mtproto/sender.h"

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
	class UserLoginWidget : public Widget::Step, private MTP::Sender {
		Q_OBJECT

	public:
		UserLoginWidget(QWidget* parent, Widget::Data* data);

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
		void onRemberUser();
		void onInputUnameChange();

	private:
		void refreshLang();
		void updateControlsGeometry();
		void initData();

		QImage _imageLogo;
		object_ptr<Ui::InputField> _unameField;//用户名输入框
		object_ptr<Ui::PasswordInput> _pwdField;//密码输入框
		object_ptr<Ui::InputField> _codeField;//验证码输入框
		object_ptr<Ui::LabelVerificationCode> _picCode;//图片验证码
		//object_ptr<Ui::FlatLabel> _picCode;//图片验证码
		object_ptr<Ui::LinkButton> _changeCode; //换一个
		object_ptr<Ui::Checkbox> _remberUser; //记住账号

	};
}// namespace Intro


