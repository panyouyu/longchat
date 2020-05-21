/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "contact/contactbox.h"
#include "contact/groupbox.h"
#include "datadefine.h"
#include "lang/lang_keys.h"
#include "contact/filterwidget.h"
#include "mainwidget.h"
#include "auth_session.h"
#include "data/data_session.h"
#include "dialogs/dialogs_indexed_list.h"
#include "history/history.h"
#include "data/data_user.h"
#include "data/data_peer_values.h"

namespace Contact {

	ContactBox::ContactBox(QWidget* parent, Window::Controller* controller): _controller(controller), _closeWait(this)
	{
		updateGroupInfoData();
		init();
	}
	
	ContactBox::~ContactBox()
	{
		clearData();
	}

	void ContactBox::init()
	{
		_vLayout = new QVBoxLayout(this);
		_vLayout->setSpacing(0);
		_vLayout->setContentsMargins(10, 0, 17, 0);
		_vLayout->setObjectName(QStringLiteral("_vLayout"));


		_filterWidget = new FilterWidget(this);
		_filterWidget->setObjectName(QStringLiteral("_filterWidget"));
		_filterWidget->setPlaceholderText(lang(lng_dlg_filter));
		_vLayout->addWidget(_filterWidget);

		_contactTree = new ContactTreeView(CTT_FULL, this);
		_contactTree->setObjectName(QStringLiteral("_contactTree"));
		_contactTree->loadDatas(_vecContactPData);

		_vLayout->addWidget(_contactTree);

		if (AuthSession::Exists() && Auth().data().contactsLoaded().value()) {
			App::main()->loadGroupDialogs();
		}
	}

	void ContactBox::slotChat(int64 peerId)
	{
		emit startChat(peerId);
		//App::main()->choosePeer(peerId, ShowAtUnreadMsgId);
		_closeWait->start(100);
		//closeBox();
	}

	void ContactBox::slotSaveGroup()
	{
		auto addBox = Ui::show(Box<GroupBox>(), LayerOption::KeepOther);
	}

	void ContactBox::prepare() {
		setTitle([] { return lang(lng_contacts_header); });

		addButton(langFactory(lng_close), [this] { closeBox(); });

		addButton(langFactory(lng_dlg_contact_group_add), [this] { slotSaveGroup(); });

		setDimensions(365, 560);

		connect(_filterWidget, SIGNAL(filterChanged()), this, SLOT(textFilterChanged()));
		connect(_contactTree, SIGNAL(startChat(int64)), this, SLOT(slotChat(int64)));
		//connect(_contactTree, &ContactTreeView::startChat, this, &ContactBox::slotChat);
		connect(_contactTree, SIGNAL(addGroup()), this, SLOT(slotAddGroup()));
		connect(_contactTree, SIGNAL(modGroup(ContactInfo*)), this, SLOT(slotModGroup(ContactInfo*)));
		connect(_contactTree, SIGNAL(delGroup(ContactInfo*)), this, SLOT(slotDelGroup(ContactInfo*)));
		connect(_contactTree, SIGNAL(showUserInfo(ContactInfo*)), this, SLOT(slotShowUserInfo(ContactInfo*)));
		connect(this, SIGNAL(startChat(int64)), App::main(), SLOT(slotChat(int64)));
		connect(_closeWait, SIGNAL(timeout()), this, SLOT(onCloseWait()));
		
		subscribe(App::main()->signalGroupChanged(), [this](int value) {
			DEBUG_LOG(("UserGroup: start update UI of contract! therad_id(%1)").arg((uint32)QThread::currentThreadId()));
			updateGroupInfoData();
			DEBUG_LOG(("UserGroup: end update UI of contract! therad_id(%1)").arg((uint32)QThread::currentThreadId()));
		});

		setStyleSheet(getAllFileContent(":/style/qss/contactdialog.qss"));
	}

	void ContactBox::textFilterChanged()
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

	void ContactBox::slotAddGroup()
	{
		auto addBox = Ui::show(Box<GroupBox>(), LayerOption::KeepOther);
		addBox->setResultHandler(std::bind(&ContactBox::resultHandler, this, std::placeholders::_1));
	}


	void ContactBox::slotModGroup(ContactInfo* pCI)
	{
		auto addBox = Ui::show(Box<GroupBox>(pCI, GOWT_MOD), LayerOption::KeepOther); 
	}

	void ContactBox::slotDelGroup(ContactInfo* pCI)
	{
		_allUserTagDelRequest = MTP::send(MTPcontacts_DelUserGroups(MTP_long(pCI->id)), rpcDone(&ContactBox::userGroupDelDone), rpcFail(&ContactBox::userGroupDelFail));
	}

	void ContactBox::slotShowUserInfo(ContactInfo* pCI)
	{
		_controller->showPeerInfo(pCI->peerId);
	}

	void ContactBox::onCloseWait()
	{
		closeBox();
	}

	void ContactBox::updateGroupInfoData()
	{		
		clearData();
		_mapUser2Group = App::main()->getUserGroupInfo();
		//防止网络更新树中数据找不到原来的地址，这里做一个副本
		for (int i = 0; i < App::main()->getGroupInfo().size(); ++i) {
			ContactInfo* pCI = new ContactInfo();
			(*pCI) = (*App::main()->getGroupInfo().at(i));
			_vecContactPData.push_back(pCI);
		}
		for (int i = 0; i < App::main()->getGroupInfo4Search().size(); ++i) {
			ContactInfo* pCI = new ContactInfo();
			(*pCI) = (*App::main()->getGroupInfo4Search().at(i));
			_vecContactPData4Search.push_back(pCI);
		}
		//_vecContactPData = App::main()->getGroupInfo();
		//_vecContactPData4Search = App::main()->getGroupInfo4Search();
		if (_contactTree != nullptr)
		{
			//qDebug() << "---ContactBox::updateGroupInfoData";
			_contactTree->loadDatas(_vecContactPData);
		}		
	}

	void ContactBox::clearData()
	{
		qDeleteAll(_vecContactPData);
		qDeleteAll(_vecContactPData4Search);
		_vecContactPData.clear();
		_vecContactPData4Search.clear();
	}

	void ContactBox::showCodeError(Fn<QString()> textFactory)
	{
		qDebug() << textFactory();
	}

	void ContactBox::userGroupDelDone(const  MTPUserGroupReturn& result)
	{
		_allUserTagDelRequest = 0;
		int32 succeed = result.c_userGroupReturn().vis_success.v;
		if (succeed == 0)
		{
			App::main()->loadGroupDialogs();
		}
	}

	bool ContactBox::userGroupDelFail(const RPCError& error)
	{
		if (MTP::isFloodError(error)) {
			_allUserTagDelRequest = 0;
			showCodeError(langFactory(lng_flood_error));
			return true;
		}
		if (MTP::isDefaultHandledError(error)) {
			return false;
		}

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

	void ContactBox::resultHandler(int result)
	{
		qDebug() << result;
	}

}
