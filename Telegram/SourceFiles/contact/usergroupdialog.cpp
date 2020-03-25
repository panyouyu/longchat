/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "contact/usergroupdialog.h"
#include "datadefine.h"
#include "lang/lang_keys.h"


namespace Contact {

UserGroupDialog::UserGroupDialog(UserData* user, QWidget *parent) : QDialog(parent), _user(user) {
	setObjectName(QStringLiteral("_contactUserGroupDialog"));
	//setWindowFlags(Qt::CustomizeWindowHint | Qt::Dialog | Qt::FramelessWindowHint);
	setWindowFlags( Qt::CustomizeWindowHint);
	resize(365, 560);
	setFixedSize(this->width(), this->height());

	init();	
}

UserGroupDialog::~UserGroupDialog() {

}





void UserGroupDialog::on__btnSave_clicked()
{
	return QDialog::accept();
}


void UserGroupDialog::init()
{
	_vLayout = new QVBoxLayout(this);
	_vLayout->setContentsMargins(11, 11, 11, 11);
	_vLayout->setObjectName(QStringLiteral("_vLayout"));

	//标题
	_hTitleLayout = new QHBoxLayout(this);
	_hTitleLayout->setObjectName(QStringLiteral("_hTitleLayout"));

	_labTitle = new QLabel(this);
	_labTitle->setText(lang(lng_dlg_contact_group_select));
	_labTitle->setObjectName(QStringLiteral("_labTitle"));
	_hTitleLayout->addStretch();
	_hTitleLayout->addWidget(_labTitle);
	_hTitleLayout->addStretch();
	_vLayout->addLayout(_hTitleLayout);
	
	

	

	//底部
	_hBottomLayout = new QHBoxLayout(this);
	_hBottomLayout->setObjectName(QStringLiteral("_hBottomLayout"));

	_btnClose = new QPushButton(this);
	_btnClose->setObjectName(QStringLiteral("_btnClose"));
	_btnClose->setText(lang(lng_dlg_contact_close));
	_hBottomLayout->addWidget(_btnClose);

	_btnSave = new QPushButton(this);
	_btnSave->setObjectName(QStringLiteral("_btnSave"));
	_btnSave->setText(lang(lng_settings_save));
	_hBottomLayout->addWidget(_btnSave);


	//_hBottomLayout->addStretch();
	_vLayout->addLayout(_hBottomLayout);


	connect(_btnSave, SIGNAL(clicked()), this, SLOT(on__btnSave_clicked()));
	connect(_btnClose, SIGNAL(clicked()), this, SLOT(reject()));

	setStyleSheet(getAllFileContent(":/style/qss/contactdialog.qss"));
}






} // namespace Contact
