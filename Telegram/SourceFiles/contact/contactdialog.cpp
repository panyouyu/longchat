/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "contact/contactdialog.h"
#include "datadefine.h"
#include "lang/lang_keys.h"
#include "contact/filterwidget.h"
#include "mainwidget.h"
#include "dialogs/dialogs_indexed_list.h"
#include "history/history.h"
#include "data/data_user.h"
#include "data/data_peer_values.h"

namespace Contact {


Dialog::Dialog(QWidget *parent) : QDialog(parent) {
	setObjectName(QStringLiteral("_contactdialog"));
	//setWindowFlags(Qt::CustomizeWindowHint | Qt::Dialog | Qt::FramelessWindowHint);
	resize(364, 600);
	setFixedSize(this->width(), this->height());

	
	
	//freshData_test();
	freshData();
	init();


	
}

Dialog::~Dialog() {
	//_vLayout->deleteLater();
	//_btnClose->deleteLater();
	qDeleteAll(_vecContactPData);
}



void Dialog::accept()
{
	return QDialog::accept();
}


void Dialog::on__btnNewGroup_clicked()
{
	_contactTree->loadDatas(_vecContactPData);
	//setStyleSheet(getAllFileContent(":/style/style1.qss"));
}

void Dialog::textFilterChanged()
{
	_contactTree->setSearchKey(_filterWidget->text().trimmed());
}

void Dialog::closeEvent(QCloseEvent* event)
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


void Dialog::init()
{
	_vLayout = new QVBoxLayout(this);
	_vLayout->setSpacing(6);
	_vLayout->setContentsMargins(11, 11, 11, 11);
	_vLayout->setObjectName(QStringLiteral("_vLayout"));

	_labTitle = new QLabel(this);
	_labTitle->setText(lang(lng_contacts_header));//qsl("联系人分组")
	_labTitle->setObjectName(QStringLiteral("_labTitle")); //用于qss调用
	_vLayout->addWidget(_labTitle);

	_filterWidget = new FilterWidget(this);
	_filterWidget->setObjectName(QStringLiteral("_filterWidget"));
	_vLayout->addWidget(_filterWidget);

	_contactTree = new ContactTreeView(this);
	_contactTree->setObjectName(QStringLiteral("_contactTree"));
	_contactTree->loadDatas(_vecContactPData);



	_vLayout->addWidget(_contactTree);

	_hLayoutStyle = new QHBoxLayout(this);
	_hLayoutStyle->setSpacing(6);
	_hLayoutStyle->setObjectName(QStringLiteral("_hLayoutStyle"));

	_btnNewContact = new QPushButton(this);
	_btnNewContact->setObjectName(QStringLiteral("_btnNewContact"));
	_btnNewContact->setText(lang(lng_confirm_contact_data));

	_btnClose = new QPushButton(this);
	_btnClose->setObjectName(QStringLiteral("_btnClose"));
	_btnClose->setText(lang(lng_dlg_contact_close));



	_hLayoutStyle->addWidget(_btnNewContact);
	_hLayoutStyle->addWidget(_btnClose);
	_hLayoutStyle->addStretch();




	_vLayout->addLayout(_hLayoutStyle);


	connect(_btnNewContact, SIGNAL(clicked()), this, SLOT(on__btnNewGroup_clicked()));
	connect(_btnClose, SIGNAL(clicked()), this, SLOT(accept()));
	connect(_filterWidget, &FilterWidget::filterChanged, this, &Dialog::textFilterChanged);
	connect(_contactTree, SIGNAL(startChat()), this, SLOT(accept()));

	setStyleSheet(getAllFileContent(":/style/qss/contactdialog.qss"));
}

void Dialog::freshData()
{
	ContactInfo* cig1 = new ContactInfo();
	cig1->id = 1;
	cig1->firstName = lang(lng_dlg_contact_group_asking);
	cig1->parentId = 0;
	cig1->showUserCount = "(1/2)";
	cig1->userTotalCount = 2;
	_vecContactPData.push_back(cig1);

	ContactInfo* cig2 = new ContactInfo();
	cig2->id = 4;
	cig2->firstName = lang(lng_dlg_contact_group_askover);
	cig2->parentId = 0;
	cig2->showUserCount = "(0/0)";
	_vecContactPData.push_back(cig2);

	auto appendList = [this](auto chats) {
		auto count = 0;
		for (const auto row : chats->all()) {
			if (const auto history = row->history()) {
				auto peer = history->peer;
				if (const auto user = history->peer->asUser()) {
					auto time = unixtime();
					qDebug() << user->id << peer->name << Data::OnlineText(user, time);
					ContactInfo* ci = new ContactInfo();
					ci->id = user->id;
					ci->firstName = peer->name;
					ci->lastName = qsl("");
					if (auto userpic = peer->currentUserpic()) {
						ci->hasAvatar = true;
					}
					ci->peerData = peer;
					ci->parentId = 1;
					ci->online = Data::OnlineTextActive(user, time);
					ci->lastLoginTime = Data::OnlineText(user, time);
					_vecContactPData.push_back(ci);
				}
			}
		}
		return count;
	};
	appendList(App::main()->contactsList());

	//_contactTree->loadDatas(_vecContactPData);
}

void Dialog::freshData_test()
{
	ContactInfo* cig1 = new ContactInfo();
	cig1->id = 1;
	cig1->firstName = lang(lng_dlg_contact_group_asking);
	cig1->parentId = 0;
	cig1->showUserCount = "(1/2)";
	cig1->userTotalCount = 2;
	_vecContactPData.push_back(cig1);
	//qDebug() << "###" << cig1 << cig1->firstName << " :" << cig1->expanded;

	ContactInfo* ci1 = new ContactInfo();
	ci1->id = 2;
	ci1->firstName = qsl("user1");
	ci1->lastName = qsl("he");
	ci1->parentId = 1;
	ci1->online = true;
	ci1->lastLoginTime = qsl("last login 2020.02.18");
	_vecContactPData.push_back(ci1);
	//qDebug() << "###" << ci1 << ci1->firstName << " :" << ci1->expanded;

	ContactInfo* ci2 = new ContactInfo();
	ci2->id = 3;
	ci2->firstName = qsl("user2");
	ci2->lastName = qsl("liu");
	ci2->parentId = 1;
	ci2->hasAvatar = true;
	ci2->lastLoginTime = qsl("last login 2020.01.18");
	_vecContactPData.push_back(ci2);
	//qDebug() << "###" << ci2 << ci2->firstName << " :" << ci2->expanded;

	ContactInfo* cig2 = new ContactInfo();
	cig2->id = 4;
	cig2->firstName = lang(lng_dlg_contact_group_askover);
	cig2->parentId = 0;
	cig2->showUserCount = "(0/0)";
	_vecContactPData.push_back(cig2);
	//qDebug() << "###" << cig2 << cig2->firstName << " :" << cig2->expanded;


	ContactInfo* cig3 = new ContactInfo();
	cig3->id = 5;
	cig3->firstName = lang(lng_dlg_contact_group_pay);
	cig3->parentId = 0;
	cig3->showUserCount = "(1/1)";
	cig3->userTotalCount = 1;
	_vecContactPData.push_back(cig3);
	//qDebug() << "###" << cig3 << cig3->firstName << " :" << cig3->expanded;


	ContactInfo* ci31 = new ContactInfo();
	ci31->id = 6;
	ci31->firstName = qsl("question");
	ci31->lastName = qsl("1");
	ci31->parentId = 5;
	ci31->hasAvatar = false;
	ci31->lastLoginTime = qsl("last login 2020.01.18");
	_vecContactPData.push_back(ci31);
	//qDebug() << "###" << ci31 << ci31->firstName << " :" << ci31->expanded;
}

} // namespace Contact
