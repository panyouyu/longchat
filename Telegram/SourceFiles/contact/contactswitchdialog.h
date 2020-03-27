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

class SwitchDialog : public QDialog , public RPCSender {
	Q_OBJECT

public:
	SwitchDialog(uint64 playerId, QWidget *parent = 0);
	~SwitchDialog();

	virtual void accept() override;



private:
	QLabel* _labTitle;
	ContactTreeView* _contactTree;
	QVBoxLayout* _vLayout;
	QHBoxLayout* _hLayoutStyle;

	
	QPushButton* _btnClose;
	QVector<ContactInfo*> _vecContactPData;
	uint64 _playerId = 0;
private slots:
	void on_switchUser(ContactInfo* pCI);
	
protected:
	virtual void closeEvent(QCloseEvent* event) override;

private:
	void init();
	void clearData();
	void freshData_test();
	void freshData();

	void getSwitchKefusDone(const MTPSwitchKefuList& result);
	bool getSwitchKefusFail(const RPCError& error);

	void switchKefuDone(const MTPBool& result);
	bool switchKefuFail(const RPCError& error);

	mtpRequestId _getSwitchKefusRequest = 0;
	mtpRequestId _switchKefuRequest = 0;

};

} // namespace Intro
