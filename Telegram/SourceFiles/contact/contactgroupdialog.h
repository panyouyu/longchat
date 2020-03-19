/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once
#include "scheme.h"
#include "treeitem.h"
#include "treemodel.h"
#include "contact/contacttreeview.h"
#include "contactdelegate.h"

#include "mtproto/sender.h"



namespace Contact {

class FilterWidget;

class GroupDialog : public QDialog , public RPCSender {
	Q_OBJECT

public:
	GroupDialog(QWidget *parent = 0, ContactInfo* pCI =nullptr, GroupOperWindowType gowt = GOWT_ADD);
	~GroupDialog();



	virtual void reject() override;


private slots:
	void on__btnSave_clicked();
	void textFilterChanged();
	void on_selectedUser(ContactInfo* pCI);
	void on_removeSelectedUser(ContactInfo* pCI);

private:
	QHBoxLayout* _hTitleLayout;
	QLabel* _labTitle;
	QHBoxLayout* _hGroupNameLayout;
	//QLabel* _labGroupName;
	QLineEdit* _lineGroupName;
	QVBoxLayout* _vLeftTreeLayout;
	FilterWidget* _filterWidget;
	ContactTreeView* _contactTree;
	ContactTreeView* _contactSelectedTree;
	QHBoxLayout* _hMiddleLayout;

	QVBoxLayout* _vLayout;
	QHBoxLayout* _hBottomLayout;

	QPushButton* _btnClose;
	QPushButton* _btnSave;
	QVector<ContactInfo*> _vecContactPData;
	QVector<ContactInfo*> _vecContactSelected; 
protected:
	virtual void closeEvent(QCloseEvent* event) override;

private:
	void init();
	void freshData();
	void bindData();
	void genContact(ContactInfo* ci, UserData* user, PeerData* peer, uint64 parentId);
	void freshTree();
	void eraseFromVector(QVector<ContactInfo*>& vecData, ContactInfo* pCI);
	bool userInGroup(uint64 uId);


	void showCodeError(Fn<QString()> textFactory);

	void userGroupDone(const MTPUserGroupReturn& result);
	bool userGroupFail(const RPCError& error);

	mtpRequestId _allUserTagAddRequest = 0;
	mtpRequestId _allUserTagModRequest = 0;

private:
	ContactInfo* _pCI { nullptr };
	GroupOperWindowType _gowt;

};

} // namespace Intro
