/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "contact/usergroupbox.h"

#include "lang/lang_keys.h"
#include "mtproto/sender.h"
#include "mainwidget.h"


namespace Contact {

	UserGroupBox::UserGroupBox(QWidget*, UserData* user)
	{
		_user = user;
		reshData();
		_vLayout = new QVBoxLayout(this);
		_vLayout->setSpacing(0);
		_vLayout->setContentsMargins(0, 0, 0, 0);
		_vLayout->setObjectName(QStringLiteral("_vLayout"));

		_contactTree = new ContactTreeView(CTT_USERINGROUP, this);
		_contactTree->setObjectName(QStringLiteral("_mailFolderTree"));

		_contactTree->loadDatas(_vecContactPData);


		_vLayout->addWidget(_contactTree);

		setStyleSheet(getAllFileContent(":/style/qss/contactdialog.qss"));
	}

	
	void UserGroupBox::reshData()
	{
		_allGroup = App::main()->getGroupInfo();
		QMutexLocker lock(&App::main()->getUserGroupMutex());
		//qDebug() << "user:" << _user->id << _user->name;
		for (int i = 0; i < _allGroup.size(); ++i) {
			//qDebug() << "group:" << _allGroup[i]->id << _allGroup[i]->firstName;
			if (_allGroup[i]->isGroup && _allGroup[i]->otherId == 0)
			{
				if (isInTheGroup(_allGroup[i]->id)) {
					_allGroup[i]->userInGroup = true;
				}
				_vecContactPData.push_back(_allGroup[i]);
			}

		}
	}

	void UserGroupBox::prepare() {
		setTitle([] { return lang(lng_dlg_contact_group_select); });

	    addButton(langFactory(lng_close), [this] { slotClose(); });

		addButton(langFactory(lng_box_ok), [this] { slotSave(); });

		setDimensions(364, 435);
		//connect(_btnCancel, &QPushButton::clicked, [=] { slotClose(); });
	}

	bool UserGroupBox::isInTheGroup(int64 gid)
	{
		_mapUser2Group = App::main()->getUserGroupInfo();
		QMap<uint64, QSet<uint64>>::const_iterator i = _mapUser2Group.find(_user->id);
		if (i != _mapUser2Group.end()) {
			foreach(const uint64 & value, i.value()) {
				if (gid == value)
				{
					return true;
				}
			}
		}
		return false;
	}

	bool UserGroupBox::onSaveUserFail(const RPCError& e)
	{
		_modRequest = 0;
		resetOrgGroup();
		
		return true;
	}

	void UserGroupBox::onSaveUserDone(const MTPcontacts_ImportedContacts& res)
	{
		_modRequest = 0;
		QString userGroupInfo = "";
		QVector<MTPlong> userGroups = _contactTree->getCheckedGroup();
		for (int i = 0; i < userGroups.size(); ++i) {
			userGroupInfo = userGroupInfo + App::main()->getGroupName(userGroups.at(i).v) + " ";
		}
		_user->setGroup(userGroupInfo);
		App::main()->loadGroupDialogs();
		closeBox();
	}

	void UserGroupBox::slotSave()
	{
	    if (_modRequest) 
		    return;

		QVector<MTPlong> userGroups = _contactTree->getCheckedGroup();
		//MTPint _is_update, const MTPVector<MTPlong>& _tag_id
		QVector<MTPInputContact> v(1, MTP_inputPhoneContact(MTP_long(_user->id), MTP_string(_user->phone()), MTP_string(_user->firstName), MTP_string(_user->lastName)));
		_modRequest = MTP::send(MTPcontacts_ImportContacts(MTP_vector<MTPInputContact>(v), MTP_int(1), MTP_vector<MTPlong>(userGroups)), 
			rpcDone(&UserGroupBox::onSaveUserDone), 
			rpcFail(&UserGroupBox::onSaveUserFail));
	}


	void UserGroupBox::slotClose()
	{
		resetOrgGroup();
		closeBox();
	}

	void UserGroupBox::resetOrgGroup()
	{
		//失败要把修改的值改回去
		QMutexLocker lock(&App::main()->getUserGroupMutex());
		for (int i = 0; i < _allGroup.size(); ++i) {
			if (_allGroup[i]->isGroup && _allGroup[i]->otherId == 0)
			{
				_allGroup[i]->userInGroup = _allGroup[i]->userInGroupTemp;
			}
		}
	}

}