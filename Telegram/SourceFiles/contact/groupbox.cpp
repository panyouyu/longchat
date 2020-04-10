/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "contact/groupbox.h"
#include "datadefine.h"
#include "lang/lang_keys.h"
#include "contact/filterwidget.h"
#include "mainwidget.h"
#include "Dialogs/Dialogs_indexed_list.h"
#include "history/history.h"
#include "data/data_user.h"
#include "data/data_peer_values.h"
#include "auth_session.h"
#include "data/data_session.h"


namespace Contact {

	GroupBox::GroupBox(QWidget*, ContactInfo* pCI, GroupOperWindowType gowt): _pCI(pCI), _gowt(gowt)
	{
		freshData();
		init();
		bindData();
	}

	
	GroupBox::~GroupBox()
	{
		qDeleteAll(_vecContactSelected);
		qDeleteAll(_vecContactPData);
	}

	void GroupBox::setResultHandler(Fn<void(int)> handler)
	{
		_resultHandler = handler;
	}

	void GroupBox::slotSaveClicked()
	{
		QVector<MTPlong> userIdVec;
		for (int i = 0; i < _vecContactSelected.size(); ++i) {
			userIdVec.push_back(MTP_long(_vecContactSelected.at(i)->id));
		}
		QString groupName = _lineGroupName->text().trimmed();
		if (_gowt == GOWT_ADD)
		{
			_allUserTagAddRequest = MTP::send(MTPcontacts_AddUserGroups(MTP_string(groupName), MTP_vector<MTPlong>(userIdVec)), rpcDone(&GroupBox::userGroupDone), rpcFail(&GroupBox::userGroupFail));
		}
		else {
			_allUserTagModRequest = MTP::send(MTPcontacts_ModUserGroups(MTP_long(_pCI->id), MTP_string(groupName), MTP_vector<MTPlong>(userIdVec)), rpcDone(&GroupBox::userGroupDone), rpcFail(&GroupBox::userGroupFail));
		}
	}

	void GroupBox::slotTextFilterChanged()
	{
		QString searchText = _filterWidget->text().trimmed();
		_contactTree->setSearchKey(searchText);
	}

	void GroupBox::slotSelectedUser(ContactInfo* pCI)
	{
		ContactInfo* pCi = new ContactInfo();
		genContact(pCi, pCI->peerId, 0);
		_vecContactSelected.push_back(pCi);
		eraseFromVector(_vecContactPData, pCI);
		freshTree();
	}

	void GroupBox::slotRemoveSelectedUser(ContactInfo* pCI)
	{
		ContactInfo* pCi = new ContactInfo();
		genContact(pCi, pCI->peerId, 0);
		_vecContactPData.push_back(pCi);
		eraseFromVector(_vecContactSelected, pCI);
		freshTree();
	}



	void GroupBox::prepare() {
		setTitle([] { return lang(lng_dlg_contact_group_edit); });

		addButton(langFactory(lng_dlg_contact_close), [this] { closeBox(); });

		addButton(langFactory(lng_settings_save), [this] { slotSaveClicked(); });

	
		setDimensions(550, 560);

		connect(_filterWidget, &FilterWidget::filterChanged, this, &GroupBox::slotTextFilterChanged);
		connect(_contactTree, SIGNAL(selectedUser(ContactInfo*)), this, SLOT(slotSelectedUser(ContactInfo*)));
		connect(_contactSelectedTree, SIGNAL(selectedUser(ContactInfo*)), this, SLOT(slotRemoveSelectedUser(ContactInfo*)));
		
		setStyleSheet(getAllFileContent(":/style/qss/contactdialog.qss"));
	}

	void GroupBox::init()
	{
		_vLayout = new QVBoxLayout(this);
		_vLayout->setContentsMargins(12, 0, 12, 0);
		_vLayout->setObjectName(QStringLiteral("_vLayout"));

		//分组名
		_hGroupNameLayout = new QHBoxLayout(this);
		//_hGroupNameLayout->setSpacing(spacing);
		_hGroupNameLayout->setObjectName(QStringLiteral("_hGroupNameLayout"));


		_lineGroupName = new QLineEdit(this);
		_lineGroupName->setObjectName(QStringLiteral("_lineGroupName"));
		_lineGroupName->setPlaceholderText(lang(lng_dlg_contact_group_name_input));
		_hGroupNameLayout->addWidget(_lineGroupName);
		_vLayout->addLayout(_hGroupNameLayout);

		//中间两颗树
		_hMiddleLayout = new QHBoxLayout(this);
		_hMiddleLayout->setObjectName(QStringLiteral("_hMiddleLayout"));


		//左侧树
		_vLeftTreeLayout = new QVBoxLayout(this);
		//_vLeftTreeLayout->setSpacing(spacing);
		_vLeftTreeLayout->setContentsMargins(0, 0, 0, 0);
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
		
	}	

	void GroupBox::freshData()
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
							genContact(ci, peer->id, 0);
							_vecContactPData.push_back(ci);
						}
						else
						{
							ContactInfo* ci = new ContactInfo();
							genContact(ci, peer->id, 0);
							_vecContactSelected.push_back(ci);
						}
					}
				}
			}
			return count;
		};
		appendList(App::main()->contactsList());
	}

	void GroupBox::bindData()
	{
		if (nullptr != _pCI && nullptr != _lineGroupName)
		{
			_lineGroupName->setText(_pCI->firstName);
		}
	}

	void GroupBox::genContact(ContactInfo* ci, uint64 peerId, uint64 parentId)
	{
		auto time = unixtime();
		PeerData* peer = Auth().data().peerLoaded(peerId);
		UserData* user = peer->asUser();
		//qDebug() << user->id << peer->name << Data::OnlineText(user, time);
		ci->id = user->id;
		ci->firstName = peer->name;
		ci->lastName = qsl("");
		if (auto userpic = peer->currentUserpic()) {
			ci->hasAvatar = true;
		}
		ci->peerId = peerId;
		ci->parentId = parentId;
		ci->online = Data::OnlineTextActive(user, time);
		ci->lastLoginTime = Data::OnlineText(user, time);
	}

	void GroupBox::freshTree()
	{
		_contactTree->loadDatas(_vecContactPData);
		_contactSelectedTree->loadDatas(_vecContactSelected);
	}

	void GroupBox::eraseFromVector(QVector<ContactInfo*>& vecData, ContactInfo* pCI)
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

	bool GroupBox::userInGroup(uint64 uId)
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

	void GroupBox::showCodeError(Fn<QString()> textFactory)
	{
		qDebug() << textFactory();
		//QMessageBox::StandardButton reply;
		//reply = QMessageBox::information(this, tr("QMessageBox::information()"), textFactory());
	}

	void GroupBox::userGroupDone(const MTPUserGroupReturn& result)
	{
		_allUserTagAddRequest = 0;
		_allUserTagModRequest = 0;
		int32 succeed = result.c_userGroupReturn().vis_success.v;
		if (succeed == 0)
		{
			if (_resultHandler)
			{
				_resultHandler(succeed);
			}
			closeBox();
		}
	}

	bool GroupBox::userGroupFail(const RPCError& error)
	{
		if (MTP::isFloodError(error)) {
			_allUserTagAddRequest = 0;
			_allUserTagModRequest = 0;
			showCodeError(langFactory(lng_flood_error));
			return true;
		}
		if (MTP::isDefaultHandledError(error)) 
			return false;

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

}