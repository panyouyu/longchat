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
#include "base/observer.h"
#include "mtproto/sender.h"



namespace Contact {

class FilterWidget;

class Dialog : public QDialog , public RPCSender, private base::Subscriber {
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
	void slotDelGroup(ContactInfo* pCI);

private:
	QLabel* _labTitle;
	FilterWidget* _filterWidget;
	ContactTreeView* _contactTree{nullptr};
	QVBoxLayout* _vLayout;
	QHBoxLayout* _hLayoutStyle;

	
	QPushButton* _btnNewContact;
	QPushButton* _btnClose;
	QVector<ContactInfo*> _vecContactPData;
	QVector<ContactInfo*> _vecContactPData4Search; //qt5.10以后才支持下级查询，所以这里把下级提升为1级查询 且把parentid设置为0
	QMap<uint64, QSet<uint64>> _mapUser2Group;
protected:
	virtual void closeEvent(QCloseEvent* event) override;

private:
	void init();
	void freshData_test();
	void updateGroupInfoData();
private:
	void showCodeError(Fn<QString()> textFactory);

	void userGroupDone(const MTPUserGroupList& result);

	void userGroupDelDone(const MTPUserGroupReturn& result);
	bool userGroupDelFail(const RPCError& error);
	
	mtpRequestId _allUserTagDelRequest = 0;

};

} // namespace Intro
