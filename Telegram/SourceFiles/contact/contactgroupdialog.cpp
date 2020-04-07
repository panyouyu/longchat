/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "contact/contactgroupdialog.h"
#include "datadefine.h"
#include "lang/lang_keys.h"
#include "contact/filterwidget.h"
#include "mainwidget.h"
#include "dialogs/dialogs_indexed_list.h"
#include "history/history.h"
#include "data/data_user.h"
#include "data/data_peer_values.h"


namespace Contact {

GroupDialog::GroupDialog(QWidget *parent, ContactInfo* pCI, GroupOperWindowType gowt) : QDialog(parent), _pCI(pCI), _gowt(gowt) {
	setObjectName(QStringLiteral("_contactgroupdialog"));
	//setWindowFlags(Qt::CustomizeWindowHint | Qt::Dialog | Qt::FramelessWindowHint);
	setWindowFlags( Qt::CustomizeWindowHint);
	resize(550, 560);
	setFixedSize(this->width(), this->height());
	
	freshData();
	init();	
	bindData();
}

GroupDialog::~GroupDialog() {
	//_vLayout->deleteLater();
	//_btnClose->deleteLater();
	qDeleteAll(_vecContactSelected);
	qDeleteAll(_vecContactPData);
}


void GroupDialog::reject()
{
	return QDialog::reject();
}


void GroupDialog::on__btnSave_clicked()
{
	QVector<MTPlong> userIdVec;
	for (int i = 0; i < _vecContactSelected.size(); ++i) {
		userIdVec.push_back(MTP_long(_vecContactSelected.at(i)->id));
	}
	QString groupName = _lineGroupName->text().trimmed();
	if (_gowt == GOWT_ADD)
	{
		_allUserTagAddRequest = MTP::send(MTPcontacts_AddUserGroups(MTP_string(groupName), MTP_vector<MTPlong>(userIdVec)), rpcDone(&GroupDialog::userGroupDone), rpcFail(&GroupDialog::userGroupFail));
	}
	else {
		_allUserTagModRequest = MTP::send(MTPcontacts_ModUserGroups(MTP_long(_pCI->id),MTP_string(groupName), MTP_vector<MTPlong>(userIdVec)), rpcDone(&GroupDialog::userGroupDone), rpcFail(&GroupDialog::userGroupFail));
	}
	//return QDialog::accept();
}

void GroupDialog::textFilterChanged()
{
	QString searchText = _filterWidget->text().trimmed();
	_contactTree->setSearchKey(searchText);
}



void GroupDialog::on_selectedUser(ContactInfo* pCI)
{
	ContactInfo* pCi = new ContactInfo();
	genContact(pCi, pCI->peerData->asUser(), pCI->peerData, 0);
	_vecContactSelected.push_back(pCi);
	eraseFromVector(_vecContactPData, pCI);
	freshTree();
}

void GroupDialog::on_removeSelectedUser(ContactInfo* pCI)
{
	ContactInfo* pCi = new ContactInfo();
	genContact(pCi, pCI->peerData->asUser(), pCI->peerData, 0);
	_vecContactPData.push_back(pCi);
	eraseFromVector(_vecContactSelected, pCI);
	freshTree();
}

void GroupDialog::closeEvent(QCloseEvent* event)
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


void GroupDialog::init()
{
	//int spacing = 6;
	_vLayout = new QVBoxLayout(this);
	//_vLayout->setSpacing(spacing);
	_vLayout->setContentsMargins(11, 11, 11, 11);
	_vLayout->setObjectName(QStringLiteral("_vLayout"));

	//标题
	_hTitleLayout = new QHBoxLayout(this);
	//_hTitleLayout->setSpacing(spacing);
	_hTitleLayout->setObjectName(QStringLiteral("_hTitleLayout"));

	_labTitle = new QLabel(this);
	_labTitle->setText(lang(lng_dlg_contact_group_edit));
	_labTitle->setObjectName(QStringLiteral("_labTitle"));
	_hTitleLayout->addStretch();
	_hTitleLayout->addWidget(_labTitle);
	_hTitleLayout->addStretch();
	_vLayout->addLayout(_hTitleLayout);
	
	//分组名
	_hGroupNameLayout = new QHBoxLayout(this);
	//_hGroupNameLayout->setSpacing(spacing);
	_hGroupNameLayout->setObjectName(QStringLiteral("_hGroupNameLayout"));

	//_labGroupName = new QLabel(this);
	//_labGroupName->setText(lang(lng_dlg_contact_group_name_input));
	//_labGroupName->setObjectName(QStringLiteral("_labGroupName"));
	//_hGroupNameLayout->addWidget(_labGroupName);

	_lineGroupName = new QLineEdit(this);
	_lineGroupName->setObjectName(QStringLiteral("_lineGroupName"));
	_lineGroupName->setPlaceholderText(lang(lng_dlg_contact_group_name_input));
	_hGroupNameLayout->addWidget(_lineGroupName);
	_vLayout->addLayout(_hGroupNameLayout);

	//中间两颗树
	_hMiddleLayout = new QHBoxLayout(this);
	//_hMiddleLayout->setSpacing(spacing);
	_hMiddleLayout->setObjectName(QStringLiteral("_hMiddleLayout"));


	//左侧树
	_vLeftTreeLayout = new QVBoxLayout(this);
	//_vLeftTreeLayout->setSpacing(spacing);
	_vLeftTreeLayout->setContentsMargins(11, 11, 11, 11);
	_vLeftTreeLayout->setObjectName(QStringLiteral("_vLeftTreeLayout"));

	_filterWidget = new FilterWidget(this);
	_filterWidget->setObjectName(QStringLiteral("_filterWidget"));
	_vLeftTreeLayout->addWidget(_filterWidget);

	_contactTree = new ContactTreeView(CTT_TOSELECT, this);
	_contactTree->setObjectName(QStringLiteral("_contactTree"));
	_contactTree->loadDatas(_vecContactPData);
	_vLeftTreeLayout->addWidget(_contactTree);

	_hMiddleLayout->addLayout(_vLeftTreeLayout);

	_contactSelectedTree = new ContactTreeView(CTT_SHOW, this);
	_contactSelectedTree->setObjectName(QStringLiteral("_contactSelectedTree"));
	_contactSelectedTree->loadDatas(_vecContactSelected);
	_hMiddleLayout->addWidget(_contactSelectedTree);

	_vLayout->addLayout(_hMiddleLayout);

	//底部
	_hBottomLayout = new QHBoxLayout(this);
	//_hBottomLayout->setSpacing(spacing);
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
	connect(_filterWidget, &FilterWidget::filterChanged, this, &GroupDialog::textFilterChanged);
	connect(_contactTree, SIGNAL(selectedUser(ContactInfo*)), this, SLOT(on_selectedUser(ContactInfo*)));
	connect(_contactSelectedTree, SIGNAL(selectedUser(ContactInfo*)), this, SLOT(on_removeSelectedUser(ContactInfo*)));

	setStyleSheet(getAllFileContent(":/style/qss/contactdialog.qss"));
}

void GroupDialog::genContact(ContactInfo* ci, UserData* user, PeerData* peer, uint64 parentId)
{
	auto time = unixtime();
	//qDebug() << user->id << peer->name << Data::OnlineText(user, time);
	ci->id = user->id;
	ci->firstName = peer->name;
	ci->lastName = qsl("");
	if (auto userpic = peer->currentUserpic()) {
		ci->hasAvatar = true;
	}
	ci->peerData = peer;
	ci->parentId = parentId;
	ci->online = Data::OnlineTextActive(user, time);
	ci->lastLoginTime = Data::OnlineText(user, time);
}

void GroupDialog::freshTree()
{
	_contactTree->loadDatas(_vecContactPData);
	_contactSelectedTree->loadDatas(_vecContactSelected);
}

void GroupDialog::eraseFromVector(QVector<ContactInfo*>& vecData, ContactInfo* pCI)
{
	QVector<ContactInfo*>::iterator i = vecData.begin();
	for (; i != vecData.end(); ++i) {
		if ((*i)->id == pCI->id)
		{
			delete (*i);  //指针需要删除内存
			vecData.removeOne(*i);
			//vecData.erase(i);
			i--;  // 注意i的位置要回退一位
		}
	}
}

bool GroupDialog::userInGroup(uint64 uId)
{
	if (_pCI != nullptr)
	{
		for (int i = 0; i < _pCI->userIds.size(); ++i) {
			if (_pCI->userIds.at(i) == uId)
			{
				return true;
			}
		}
	}
	return false;
}

void GroupDialog::showCodeError(Fn<QString()> textFactory)
{
	qDebug() << textFactory();
	QMessageBox::StandardButton reply;
    reply = QMessageBox::information(this, tr("QMessageBox::information()"), textFactory());
}

void GroupDialog::userGroupDone(const MTPUserGroupReturn& result)
{
	_allUserTagAddRequest = 0;
	_allUserTagModRequest = 0;
	int32 succeed = result.c_userGroupReturn().vis_success.v;
	if (succeed == 0)
	{
		return QDialog::accept();
	}
	QMessageBox::StandardButton reply;
	reply = QMessageBox::information(this, tr("QMessageBox::information()"), "fail!");
}

bool GroupDialog::userGroupFail(const RPCError& error)
{
	if (MTP::isFloodError(error)) {
		//stopCheck();
		_allUserTagAddRequest = 0;
		_allUserTagModRequest = 0;
		showCodeError(langFactory(lng_flood_error));
		return true;
	}
	if (MTP::isDefaultHandledError(error)) return false;

	//stopCheck();
	_allUserTagAddRequest = 0;
	_allUserTagModRequest = 0;
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



void GroupDialog::freshData()
{
	auto appendList = [this](auto chats) {
		auto count = 0;
		for (const auto row : chats->all()) {
			if (const auto history = row->history()) {
				auto peer = history->peer;
				if (const auto user = history->peer->asUser()) {
					if (!userInGroup(user->id))
					{
						ContactInfo* ci = new ContactInfo();
						genContact(ci, user, peer, 0);
						_vecContactPData.push_back(ci);
					}
					else
					{
						ContactInfo* ci = new ContactInfo();
						genContact(ci, user, peer, 0);
						_vecContactSelected.push_back(ci);
					}
				}
			}
		}
		return count;
	};
	appendList(App::main()->contactsList());
}


void GroupDialog::bindData()
{
	if (nullptr != _pCI && nullptr != _lineGroupName)
	{
		_lineGroupName->setText(_pCI->firstName);
	}
}

} // namespace Contact
