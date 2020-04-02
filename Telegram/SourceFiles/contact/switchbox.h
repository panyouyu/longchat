/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once
#include "boxes/abstract_box.h"
#include "scheme.h"
#include "treeitem.h"
#include "treemodel.h"
#include "contact/contacttreeview.h"
#include "contactdelegate.h"

#include "mtproto/sender.h"


namespace Contact {
	class SwitchBox : public BoxContent, public RPCSender {
		Q_OBJECT
	public:
		SwitchBox(QWidget*, UserData* user);
		~SwitchBox();
		
	protected:
		void prepare() override;

	private:
		void init();
		void clearData();
		void freshData();

		void getSwitchKefusDone(const MTPSwitchKefuList& result);
		bool getSwitchKefusFail(const RPCError& error);

		void switchKefuDone(const MTPBool& result);
		bool switchKefuFail(const RPCError& error);

	private slots:
		void on_switchUser(ContactInfo* pCI);
		
	private:
		ContactTreeView* _contactTree;
		QVBoxLayout* _vLayout;
		QVector<ContactInfo*> _vecContactPData;
		UserData* _user;
		uint64 _playerId = 0;
		
		mtpRequestId _getSwitchKefusRequest = 0;
		mtpRequestId _switchKefuRequest = 0;

	};

}