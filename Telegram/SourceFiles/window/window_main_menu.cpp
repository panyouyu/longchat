/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "window/window_main_menu.h"

#include "window/themes/window_theme.h"
#include "window/window_controller.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/menu.h"
#include "ui/special_buttons.h"
#include "ui/empty_userpic.h"
#include "mainwindow.h"
#include "storage/localstorage.h"
#include "support/support_templates.h"
#include "settings/settings_common.h"
#include "contact/contactbox.h"
#include "core/qt_signal_producer.h"
#include "boxes/about_box.h"
#include "boxes/peer_list_controllers.h"
#include "calls/calls_box_controller.h"
#include "lang/lang_keys.h"
#include "core/click_handler_types.h"
#include "observer_peer.h"
#include "auth_session.h"
#include "data/data_user.h"
#include "mainwidget.h"
#include "styles/style_window.h"
#include "styles/style_dialogs.h"
#include "styles/style_settings.h"
#include "styles/style_boxes.h"

namespace Window {

MainMenu::MainMenu(
	QWidget *parent,
	not_null<Controller*> controller)
: RpWidget(parent)
, _controller(controller)
, _mseeage(this, st::mainMenuMessageButton, lang(lng_settings_messages))
, _contact(this, st::mainMenuContactButton, lang(lng_menu_contacts))
, _call(this, st::mainMenuCallButton, lang(lng_menu_calls))
, _setting(this, st::mainMenuSettingButton, lang(lng_menu_settings)){
	setAttribute(Qt::WA_OpaquePaintEvent);
	resize(st::mainMenuWidth, parentWidget()->height());
	auto showSelfChat = [] {
		App::main()->choosePeer(Auth().userPeerId(), ShowAtUnreadMsgId);
	};
	_userpicButton.create(
		this,
		_controller,
		Auth().user(),
		Ui::UserpicButton::Role::Custom,
		st::mainMenuUserpic);
	_userpicButton->setClickedCallback(showSelfChat);
	_userpicButton->show();

	_mseeage->setClickedCallback([] {
		App::wnd()->activate();
	});
	_contact->setClickedCallback([] {
		Ui::show(Box<Contact::ContactBox>(App::wnd()->controller()));
	});
	_call->setClickedCallback([] {
		Ui::show(Box<PeerListBox>(std::make_unique<Calls::BoxController>(), [](not_null<PeerListBox*> box) {
			box->addButton(langFactory(lng_close), [=] {
				box->closeBox();
				});
			box->addTopButton(st::callSettingsButton, [=] {
				App::wnd()->controller()->showSettings(
					Settings::Type::Calls,
					Window::SectionShow(anim::type::instant));
				});
			}));
 	});
	_setting->setClickedCallback([] { App::wnd()->showSettings(); });
	subscribe(Global::RefPhoneCallsEnabledChanged(), [this] { updateControlsGeometry(); });

	subscribe(Global::RefUnreadCounterUpdate(), [=] {
		_mseeage->updateUnreadCounter();
 	});
}

void MainMenu::resizeEvent(QResizeEvent *e) {
	updateControlsGeometry();
}

void MainMenu::updateControlsGeometry() {
	auto left = 0, top = 0;
	if (_userpicButton) {
		top += st::mainMenuUserpicTop;
		_userpicButton->moveToLeft((st::mainMenuWidth - _userpicButton->width()) >> 1, st::mainMenuUserpicTop);
		top += _userpicButton->height();
	}
	_mseeage->resizeToWidth(width());
	_mseeage->moveToLeft(left, top);
	top += _mseeage->height();

	_contact->resizeToWidth(width());
	_contact->moveToLeft(left, top);
	top += _contact->height();

	if (Global::PhoneCallsEnabled()) {
		_call->resizeToWidth(width());
		_call->moveToLeft(left, top);
		top += _call->height();
	} else {
		_call->hide();
 	}
	
	_setting->resizeToWidth(width());
	_setting->moveToLeft(left, height() - _setting->height());
}

void MainMenu::paintEvent(QPaintEvent *e) {
	Painter p(this);
	p.fillRect(rect(), st::leftMenuBg);
	int x = (width() - st::mainMenuLogo.width()) >> 1;
	int y = (st::mainMenuUserpicTop - st::mainMenuLogo.height()) >> 1;
	st::mainMenuLogo.paint(p, QPoint(x, y), width());	
}

void MainMenu::showAnimated(const QPixmap& bgAnimCache, bool back)
{
	show();
	_a_show.stop();
	_a_show.start([this] { update(); }, 0., 1., st::slideDuration, anim::easeOutCirc);
}

} // namespace Window
