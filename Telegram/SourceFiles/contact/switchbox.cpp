/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "contact/switchbox.h"
#include "datadefine.h"
#include "lang/lang_keys.h"
#include "contact/filterwidget.h"
#include "mainwidget.h"
#include "Dialogs/Dialogs_indexed_list.h"
#include "history/history.h"
#include "data/data_user.h"
#include "data/data_peer_values.h"


namespace Contact {

	SwitchBox::SwitchBox(QWidget*, UserData* user): _user(user)
	{
		_playerId = user->id;
		freshData();
		init();
	}

	
	SwitchBox::~SwitchBox()
	{
		clearData();
	}

	void SwitchBox::on_switchUser(ContactInfo* pCI)
	{
		if (_switchKefuRequest)
			return;
		if (pCI)
		{
			_switchKefuRequest = MTP::send(MTPkefu_SwitchKefu(MTP_int(pCI->id), MTP_int(_playerId)), rpcDone(&SwitchBox::switchKefuDone), rpcFail(&SwitchBox::switchKefuFail));
		}
	}


	void SwitchBox::init()
	{
		_vLayout = new QVBoxLayout(this);
		_vLayout->setSpacing(0);
		_vLayout->setContentsMargins(0, 0, 0, 0);
		_vLayout->setObjectName(QStringLiteral("_vLayout"));
		
		_contactTree = new ContactTreeView(CTT_SWITCH, this);
		_contactTree->setObjectName(QStringLiteral("_contactTree"));
		_contactTree->loadDatas(_vecContactPData);
		_vLayout->addWidget(_contactTree);
	
		setStyleSheet("QTreeView { border:none;	background:#ffffff;	}");
	}



	void SwitchBox::prepare() {
		setTitle([] { return lang(lng_switchboard_online_user); });

	    addButton(langFactory(lng_close), [this] { closeBox(); });

		//addButton(langFactory(lng_box_ok), [this] { slotSave(); });

		//connect(_contactTree, SIGNAL(switchUser(ContactInfo*)), this, SLOT(on_switchUser(ContactInfo*)));
		//connect(_contactTree, &ContactTreeView::switchUser, [=] { on_switchUser; });
		connect(_contactTree, SIGNAL(switchUser(ContactInfo*)), this, SLOT(on_switchUser(ContactInfo*)));

		setDimensions(410, 600);
		//connect(_btnCancel, &QPushButton::clicked, [=] { slotClose(); });
	}

	

	void SwitchBox::clearData()
	{
		qDeleteAll(_vecContactPData);
		_vecContactPData.clear();
	}



	void SwitchBox::freshData()
	{
		if (_getSwitchKefusRequest)
			return;
		_getSwitchKefusRequest = MTP::send(MTPkefu_GetSwitchKefus(MTP_int(_playerId)), rpcDone(&SwitchBox::getSwitchKefusDone), rpcFail(&SwitchBox::getSwitchKefusFail));
	}


	void SwitchBox::getSwitchKefusDone(const MTPSwitchKefuList& result)
	{
		_getSwitchKefusRequest = 0;
		clearData();
		for (const auto& kefu : result.c_switchKefuList().vkefus.v) {
			//ContactInfo* info = new Contact::ContactInfo();
			//qDebug() << kefu.c_switchKefu().vkefu_id.v << kefu.c_switchKefu().vwaiting_count.v << kefu.c_switchKefu().vservice_count.v << kefu.c_switchKefu().vnick_name.v;
			ContactInfo* info = new ContactInfo();
			info->id = kefu.c_switchKefu().vkefu_id.v;
			info->firstName = kefu.c_switchKefu().vnick_name.v;
			info->parentId = 0;
			info->serverNum = kefu.c_switchKefu().vservice_count.v;
			info->serviceMax = kefu.c_switchKefu().vservice_max.v;
			info->queueNum = kefu.c_switchKefu().vwaiting_count.v;
			info->serverCount = QString(lang(lng_switchboard_num_server)) + QString::number(info->serverNum);
			info->queueCount = QString(lang(lng_switchboard_num_queue)) + QString::number(info->queueNum);
			info->lastName = lang(lng_switchboard_user);
			_vecContactPData.push_back(info);
		}
		if (_vecContactPData.size() > 0)
		{
			_contactTree->loadDatas(_vecContactPData);
		}
	}

	bool SwitchBox::getSwitchKefusFail(const RPCError& error)
	{
		if (MTP::isDefaultHandledError(error)) {
			return false;
		}

		LOG(("RPC Error: %1 %2: %3").arg(error.code()).arg(error.type()).arg(error.description()));

		_getSwitchKefusRequest = 0;

		return true;
	}

	void SwitchBox::switchKefuDone(const MTPBool& result)
	{
		App::main()->slotSwitchUser(_user);
		closeBox();
	}

	bool SwitchBox::switchKefuFail(const RPCError& error)
	{
		if (MTP::isDefaultHandledError(error)) {
			return false;
		}

		LOG(("RPC Error: %1 %2: %3").arg(error.code()).arg(error.type()).arg(error.description()));

		_switchKefuRequest = 0;

		return true;
	}


}