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
	class FilterWidget;
	class GroupBox : public BoxContent, public RPCSender {
		Q_OBJECT
	public:
		GroupBox(QWidget*, ContactInfo* pCI = nullptr, GroupOperWindowType gowt = GOWT_ADD);
		~GroupBox();
		
		void setResultHandler(Fn<void(int)> handler);

		//setResultHandler(Fn<void(int result)> handler) {
		//	_resultHandler = std::move(handler);
		//}
	protected:
		void prepare() override;


	private:
		void init();
		void freshData();
		void bindData();
		void genContact(ContactInfo* ci, uint64 peerId, uint64 parentId);
		void freshTree();
		void eraseFromVector(QVector<ContactInfo*>& vecData, ContactInfo* pCI);
		bool userInGroup(uint64 uId);


		void showCodeError(Fn<QString()> textFactory);

		void userGroupDone(const MTPUserGroupReturn& result);
		bool userGroupFail(const RPCError& error);

	
	signals:
		void signSucess();

	private slots:
		void slotSaveClicked();
		void slotTextFilterChanged();
		void slotSelectedUser(ContactInfo* pCI);
		void slotRemoveSelectedUser(ContactInfo* pCI);
		
	private:
		QHBoxLayout* _hGroupNameLayout;
		QLineEdit* _lineGroupName;
		QLabel* _errorLabel;
		QVBoxLayout* _vLeftTreeLayout;
		FilterWidget* _filterWidget;
		ContactTreeView* _contactTree;
		ContactTreeView* _contactSelectedTree;
		QHBoxLayout* _hMiddleLayout;

		QVBoxLayout* _vLayout;

		QVector<ContactInfo*> _vecContactPData;
		QVector<ContactInfo*> _vecContactSelected;

		mtpRequestId _allUserTagAddRequest = 0;
		mtpRequestId _allUserTagModRequest = 0;

		ContactInfo* _pCI{ nullptr };
		GroupOperWindowType _gowt;

		Fn<void(int)> _resultHandler{ nullptr };

	};

}