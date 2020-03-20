/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "contact/contactSwitchDialog.h"
#include "datadefine.h"
#include "lang/lang_keys.h"
#include "contact/filterwidget.h"
#include "mainwidget.h"
#include "Dialogs/Dialogs_indexed_list.h"
#include "history/history.h"
#include "data/data_user.h"
#include "data/data_peer_values.h"

namespace Contact {

SwitchDialog::SwitchDialog(QWidget *parent) : QDialog(parent) {
	setObjectName(QStringLiteral("_contactSwitchDialog"));
	//setWindowFlags(Qt::CustomizeWindowHint | Qt::SwitchDialog | Qt::FramelessWindowHint);
	setWindowFlags(Qt::CustomizeWindowHint);
	resize(365, 560);
	setFixedSize(this->width(), this->height());

	freshData_test();
	init();	
}

SwitchDialog::~SwitchDialog() {
	clearData();
}

void SwitchDialog::accept()
{
	return QDialog::accept();
}



void SwitchDialog::closeEvent(QCloseEvent* event)
{
	//QMessageBox::question(this,
	//	tr("Quit"),
	//	tr("Are you sure to quit this application?"),
	//	QMessageBox::Yes, QMessageBox::No)
	//	== QMessageBox::Yes
	if (true) {
		event->accept();//不会将事件传递给组件的父组件
	}
	else
		event->ignore();
}


void SwitchDialog::init()
{
	_vLayout = new QVBoxLayout(this);
	_vLayout->setSpacing(6);
	_vLayout->setContentsMargins(11, 11, 11, 11);
	_vLayout->setObjectName(QStringLiteral("_vLayout"));

	_labTitle = new QLabel(this);
	_labTitle->setText(lang(lng_switchboard_online_user));
	_labTitle->setObjectName(QStringLiteral("_labTitle")); 
	_vLayout->addWidget(_labTitle);


	_contactTree = new ContactTreeView(CTT_SWITCH, this);
	_contactTree->setObjectName(QStringLiteral("_contactTree"));
	_contactTree->loadDatas(_vecContactPData);



	_vLayout->addWidget(_contactTree);

	_hLayoutStyle = new QHBoxLayout(this);
	_hLayoutStyle->setSpacing(6);
	_hLayoutStyle->setObjectName(QStringLiteral("_hLayoutStyle"));
	_hLayoutStyle->addStretch();
	

	_btnClose = new QPushButton(this);
	_btnClose->setObjectName(QStringLiteral("_btnClose"));
	_btnClose->setText(lang(lng_dlg_contact_close));
	_hLayoutStyle->addWidget(_btnClose);


	_vLayout->addLayout(_hLayoutStyle);


	
	connect(_btnClose, SIGNAL(clicked()), this, SLOT(accept()));

	setStyleSheet(getAllFileContent(":/style/qss/contactDialog.qss"));
}


void SwitchDialog::clearData()
{
	qDeleteAll(_vecContactPData);
	_vecContactPData.clear();
}

void SwitchDialog::freshData_test()
{
	ContactInfo* cig1 = new ContactInfo();
	cig1->id = 1;
	cig1->firstName = "kefu1";
	cig1->parentId = 0;
	cig1->serverCount = QString(lang(lng_switchboard_num_server)) + "5";
	cig1->queueCount = QString(lang(lng_switchboard_num_queue)) + "3";
	cig1->lastName = lang(lng_switchboard_user);
	_vecContactPData.push_back(cig1);

	ContactInfo* cig2 = new ContactInfo();
	cig2->id = 2;
	cig2->firstName = qsl("kefu2");
	cig2->parentId = 0;
	cig2->serverCount = QString(lang(lng_switchboard_num_server)) + "3";
	cig2->queueCount = QString(lang(lng_switchboard_num_queue)) + "1";
	cig2->lastName = lang(lng_switchboard_user);
	_vecContactPData.push_back(cig2);
}

} // namespace Contact
