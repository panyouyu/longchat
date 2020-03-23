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
#include "contactgroupdialog.h"


namespace Contact {

Dialog::Dialog(QWidget *parent) : QDialog(parent) {
	setObjectName(QStringLiteral("_contactdialog"));
	//setWindowFlags(Qt::CustomizeWindowHint | Qt::Dialog | Qt::FramelessWindowHint);
	setWindowFlags(Qt::CustomizeWindowHint);
	resize(365, 560);
	setFixedSize(this->width(), this->height());

	//freshData_test();
	updateGroupInfoData();
	init();	
}

Dialog::~Dialog() {
	//_vLayout->deleteLater();
	//_btnClose->deleteLater();
}



void Dialog::accept()
{
	return QDialog::accept();
}


void Dialog::on__btnNewGroup_clicked()
{
	GroupDialog dlg(nullptr, nullptr);
	if (QDialog::Accepted == dlg.exec()) {
		//freshData();
		App::main()->loadGroupDialogs();		
	}
}

void Dialog::textFilterChanged()
{
	QString searchText = _filterWidget->text().trimmed();
	if (searchText.isEmpty() || searchText.isNull())
	{
		_contactTree->loadDatas(_vecContactPData);
	}
	else
	{
		_contactTree->loadDatas(_vecContactPData4Search);
	}
	_contactTree->setSearchKey(searchText);
}

void Dialog::slotAddGroup()
{
	GroupDialog dlg(nullptr, nullptr);
	if (QDialog::Accepted == dlg.exec()) {
		//freshData();
		App::main()->loadGroupDialogs();
	}
	
}


void Dialog::slotModGroup(ContactInfo* pCI)
{
	GroupDialog dlg(nullptr, pCI, GOWT_MOD);
	if (QDialog::Accepted == dlg.exec()) {
		//freshData();
		App::main()->loadGroupDialogs();
	}
}

void Dialog::slotDelGroup(ContactInfo* pCI)
{
	_allUserTagDelRequest = MTP::send(MTPcontacts_DelUserGroups(MTP_long(pCI->id)), rpcDone(&Dialog::userGroupDelDone), rpcFail(&Dialog::userGroupDelFail));
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

	_contactTree = new ContactTreeView(CTT_FULL, this);
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

	_btnNewContact = new QPushButton(this);
	_btnNewContact->setObjectName(QStringLiteral("_btnNewContact"));
	_btnNewContact->setText(lang(lng_dlg_contact_group_add));//lng_confirm_contact_data
	_hLayoutStyle->addWidget(_btnNewContact);


	_vLayout->addLayout(_hLayoutStyle);


	connect(_btnNewContact, SIGNAL(clicked()), this, SLOT(on__btnNewGroup_clicked()));
	connect(_btnClose, SIGNAL(clicked()), this, SLOT(accept()));
	connect(_filterWidget, &FilterWidget::filterChanged, this, &Dialog::textFilterChanged);
	connect(_contactTree, SIGNAL(startChat()), this, SLOT(accept()));
	connect(_contactTree, SIGNAL(addGroup()), this, SLOT(slotAddGroup()));
	connect(_contactTree, SIGNAL(modGroup(ContactInfo*)), this, SLOT(slotModGroup(ContactInfo*)));
	connect(_contactTree, SIGNAL(delGroup(ContactInfo*)), this, SLOT(slotDelGroup(ContactInfo*)));

	subscribe(App::main()->signalGroupChanged(), [this](int value) {
		updateGroupInfoData();
		});

	setStyleSheet(getAllFileContent(":/style/qss/contactdialog.qss"));
}


void Dialog::updateGroupInfoData()
{
	_mapUser2Group = App::main()->getUserGroupInfo();
	_vecContactPData = App::main()->getGroupInfo();
	_vecContactPData4Search = App::main()->getGroupInfo4Search();
	if (_contactTree != nullptr)
	{
		_contactTree->loadDatas(_vecContactPData);
	}
	
}

void Dialog::showCodeError(Fn<QString()> textFactory)
{
	qDebug() << textFactory();
}



void Dialog::userGroupDelDone(const  MTPUserGroupReturn& result)
{
	_allUserTagDelRequest = 0;
	int32 succeed = result.c_userGroupReturn().vis_success.v;
	if (succeed == 0)
	{
		App::main()->loadGroupDialogs();
		//freshData();
	}
}

bool Dialog::userGroupDelFail(const RPCError& error)
{
	if (MTP::isFloodError(error)) {
		//stopCheck();
		_allUserTagDelRequest = 0;
		showCodeError(langFactory(lng_flood_error));
		return true;
	}
	if (MTP::isDefaultHandledError(error)) {
		return false;
	}

	//stopCheck();
	_allUserTagDelRequest = 0;
	auto& err = error.type();
	if (err == qstr("PASSWORD_WRONG")) {
		showCodeError(langFactory(lng_signin_bad_password));
		return true;
	}
	if (Logs::DebugEnabled()) { // internal server error
		auto text = err + ": " + error.description();
		showCodeError([text] { return text; });
	}
	else {
		showCodeError(&Lang::Hard::ServerError);
	}
	return false;
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
