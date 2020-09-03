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
#include "ui/toast/toast.h"
#include "mainwindow.h"
#include "auth_session.h"
#include "data/data_session.h"
#include "storage/localstorage.h"
#include "support/support_templates.h"
#include "settings/settings_common.h"
#include "core/qt_signal_producer.h"
#include "boxes/peer_list_controllers.h"
#include "calls/calls_box_controller.h"
#include "lang/lang_keys.h"
#include "core/click_handler_types.h"
#include "observer_peer.h"
#include "auth_session.h"
#include "data/data_user.h"
#include "mainwidget.h"
#include "core/application.h"
#include "app.h"
#include "facades.h"
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
, _mseeage(this, st::mainMenuMessageButton)
, _contact(this, st::mainMenuContactButton, false)
, _netdisc(this, st::mainMenuNetDiscButton)
, _newChat(this, st::mainMenuNewChatButton)
, _mail(this, st::mainMenuMailButton)
, _setting(this, st::mainMenuSettingButton)
, _longChatMail(this) {
	setAttribute(Qt::WA_OpaquePaintEvent);
	auto showSelfChat = [] {
		App::main()->choosePeer(Auth().userPeerId(), ShowAtUnreadMsgId);
	};
	auto hideSettingAndLayer = [] {
		if (App::wnd()->ui_isLayerShown()) {
			App::wnd()->ui_hideSettingsAndLayer(anim::type::instant);
		}
	};
	_userpicButton.create(
		this,
		_controller,
		Auth().user(),
		Ui::UserpicButton::Role::Custom,
		st::mainMenuUserpic);
	_userpicButton->setClickedCallback(showSelfChat);
	_userpicButton->show();	
	_mseeage->setClickedCallback([=] {
		hideSettingAndLayer();
		App::wnd()->activate();
	});	
	_contact->setClickedCallback([=] {
		hideSettingAndLayer();
		Ui::show(Box<PeerListBox>(std::make_unique<ContactsBoxController>(), [](not_null<PeerListBox*> box) {
			box->addButton(langFactory(lng_close), [box] { box->closeBox(); });
			box->addLeftButton(langFactory(lng_profile_add_contact), [] { App::wnd()->onShowAddContact(); });			
		}));		
	});
	_netdisc->setClickedCallback([=] {
		hideSettingAndLayer();
		showSelfChat();
	});
	_newChat->setClickedCallback([=] {
		hideSettingAndLayer();
		App::wnd()->onShowNewGroup();
	});
	_mail->setHidden(Global::LongChatMailArguments().isEmpty());
	_mail->setClickedCallback([=] {
		if (_longChatMail->state() == QProcess::Starting) {
			Ui::Toast::Show(lang(lng_mail_is_starting));
			return;
		} else if (_longChatMail->state() == QProcess::Running) {
			Ui::Toast::Show(lang(lng_mail_is_runging));
			return;
		}

		auto program = cWorkingDir() + qsl("LongChatMail/LongChatMail.exe");
		auto arguments = { qsl("-proxyinfo"), Global::LongChatMailArguments() };
		_longChatMail->start(program, arguments);
		connect(_longChatMail, &QProcess::started, [] {
			DEBUG_LOG(("LongChatMail Info: started"));
		});
		connect(_longChatMail, &QProcess::errorOccurred, [](auto error) {
			DEBUG_LOG(("LongChatMail Error: error(%1)").arg(error));
		});
	});
	_setting->setClickedCallback([=] {
		hideSettingAndLayer();
		App::wnd()->showSettings();
	});

	subscribe(Global::RefUnreadCounterUpdate(), [=] {
		const auto counter = Core::App().unreadBadge();
		const auto muted = Core::App().unreadBadgeMuted();
		auto& bg = (muted ? st::trayCounterBgMute : st::trayCounterBg);
		auto& fg = st::trayCounterFg;
		_mseeage->updateUnReadCount(20, counter, bg, fg);
	});
	using namespace rpl::mappers;
	const auto &sessionData = Auth().data();
	rpl::combine(
		sessionData.friendRequestValue(),
		sessionData.groupUnReadCountValue(),
		_1 + _2
	) | rpl::start_with_next([=](auto count) {
		_contact->updateUnReadCount(20, count, st::trayCounterBg, st::trayCounterFg);
	}, lifetime());
	subscribe(Global::RefLongChatMailArgumentsChanged(), [=] {
		_mail->setHidden(Global::LongChatMailArguments().isEmpty());
		updateControlsGeometry();
		update();
	});
}

void MainMenu::resizeEvent(QResizeEvent *e) {
	updateControlsGeometry();
}

void MainMenu::updateControlsGeometry() {
	auto left = 0, top = 0;
#ifdef Q_OS_MAC
	top += 20; // for close/min/max menu
#endif // Q_OS_MAC

	if (_userpicButton) {
		top += st::mainMenuUserpicTop;
		_userpicButton->moveToLeft((width() - _userpicButton->width()) >> 1, top);
		top += _userpicButton->height();
		top += st::mainMenuUserpcoBottom;
	}	

	_mseeage->resizeToWidth(width());
	_mseeage->moveToLeft(left, top);
	top += _mseeage->height();
	
	_contact->resizeToWidth(width());
	_contact->moveToLeft(left, top);
	top += _contact->height();
	
	_netdisc->resizeToWidth(width());
	_netdisc->moveToLeft(left, top);
	top += _netdisc->height();

	_newChat->resizeToWidth(width());
	_newChat->moveToLeft(left, top);
	top += _newChat->height();


#ifdef Q_OS_WIN
	if (!_mail->isHidden()) {
		_mail->resizeToWidth(width());
		_mail->moveToLeft(left, top);
		top += _mail->height();
	}
#endif // Q_OS_WIN
		_setting->resizeToWidth(width());
	_setting->moveToLeft(left, height() - _setting->height());
}

void MainMenu::paintEvent(QPaintEvent *e) {
	Painter p(this);
	p.fillRect(rect(), st::leftMenuBg);
}

} // namespace Window
