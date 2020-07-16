/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "ui/rp_widget.h"

namespace Ui {
class UserpicButton;
class MainMenuButton;
class MainMenuMsgButton;
} // namespace Ui

namespace Window {

class Controller;

class MainMenu : public Ui::RpWidget, private base::Subscriber {
public:
	MainMenu(QWidget *parent, not_null<Controller*> controller);

	void setInnerFocus() {
		setFocus();
	}

protected:
	void paintEvent(QPaintEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;

private:
	void updateControlsGeometry();

	not_null<Controller*> _controller;
	object_ptr<Ui::UserpicButton> _userpicButton = { nullptr };	
	object_ptr<Ui::MainMenuMsgButton> _mseeage;
	object_ptr<Ui::MainMenuMsgButton> _contact;
	object_ptr<Ui::MainMenuButton> _netdisc;
	object_ptr<Ui::MainMenuButton> _newChat;
	object_ptr<Ui::MainMenuButton> _setting;
};

} // namespace Window
