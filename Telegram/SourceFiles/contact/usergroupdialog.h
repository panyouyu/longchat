/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once
#include "scheme.h"

#include "mtproto/sender.h"



namespace Contact {

class UserGroupDialog : public QDialog , public RPCSender {
	Q_OBJECT

public:
	UserGroupDialog(UserData* user, QWidget *parent = 0);
	~UserGroupDialog();


private slots:
	void on__btnSave_clicked();

private:
	QHBoxLayout* _hTitleLayout;
	QLabel* _labTitle;

	QVBoxLayout* _vLayout;
	QHBoxLayout* _hBottomLayout;

	QPushButton* _btnClose;
	QPushButton* _btnSave;

private:
	void init();
	
private:
	UserData* _user;
};

} // namespace Intro
