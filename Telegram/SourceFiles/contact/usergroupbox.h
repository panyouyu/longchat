/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "boxes/abstract_box.h"
#include "mtproto/sender.h"
#include "styles/style_widgets.h"

#include "contact/datadefine.h"
#include "contact/contacttreeview.h"
#include "mainwidget.h"


namespace Contact {
	class UserGroupBox : public BoxContent, public RPCSender {
		
	public:
		UserGroupBox(QWidget*, UserData* user);
		void reshData();
	protected:
		void prepare() override;

	private:
		bool isInTheGroup(int64 gid);

		bool onSaveUserFail(const RPCError& e);


		void onSaveUserDone(const MTPcontacts_ImportedContacts& res);


		void slotSave();
		void slotClose();
		//回退修改前的分组信息
		void resetOrgGroup();
		
	private:

		UserData* _user;

		ContactTreeView* _contactTree;
		QVBoxLayout* _vLayout;

		QVector<ContactInfo*> _vecContactPData;
		QMap<uint64, QSet<uint64>> _mapUser2Group;
		QVector<ContactInfo*> _allGroup;

		mtpRequestId _modRequest = 0;
		
	};
}
