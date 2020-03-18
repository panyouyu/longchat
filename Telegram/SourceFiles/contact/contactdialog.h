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

class Dialog : public QDialog , public RPCSender {
	Q_OBJECT

public:
	Dialog(QWidget *parent = 0);
	~Dialog();

	virtual void accept() override;

private slots:
	void on__btnNewGroup_clicked();
	void textFilterChanged();
	void slotAddGroup();
	void slotModGroup(ContactInfo* pCI);

private:
	QLabel* _labTitle;
	FilterWidget* _filterWidget;
	ContactTreeView* _contactTree;
	QVBoxLayout* _vLayout;
	QHBoxLayout* _hLayoutStyle;

	
	QPushButton* _btnNewContact;
	QPushButton* _btnClose;
	QVector<ContactInfo*> _vecContactPData;
	QVector<ContactInfo*> _vecContactPData4Search; //qt5.10�Ժ��֧���¼���ѯ������������¼�����Ϊ1����ѯ �Ұ�parentid����Ϊ0
protected:
	virtual void closeEvent(QCloseEvent* event) override;

private:
	void init();
	void freshData();
	void freshData_test();
	void genContact(ContactInfo* ci, UserData* user, PeerData* peer, uint64 parentId);

private:
	mtpRequestId _kefuLoginRequest = 0;
	void codeSubmitDone(const MTPauth_Authorization& result);
	bool codeSubmitFail(const RPCError& error);
	void showCodeError(Fn<QString()> textFactory);

	void userGroupDone(const MTPVector<MTPUserGroup>& result);
	bool userGroupFail(const RPCError& error);

	void userGroupDelDone(const MTPVector<MTPUserGroup>& result);
	bool userGroupDelFail(const RPCError& error);

	mtpRequestId _allUserTagRequest = 0;

	mtpRequestId _allUserTagDelRequest = 0;

};

} // namespace Intro