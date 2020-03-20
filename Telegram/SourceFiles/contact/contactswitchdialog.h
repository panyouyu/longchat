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
	SwitchDialog(QWidget *parent = 0);
	~SwitchDialog();

	virtual void accept() override;



private:
	QLabel* _labTitle;
	ContactTreeView* _contactTree;
	QVBoxLayout* _vLayout;
	QHBoxLayout* _hLayoutStyle;

	
	QPushButton* _btnClose;
	QVector<ContactInfo*> _vecContactPData;
protected:
	virtual void closeEvent(QCloseEvent* event) override;

private:
	void init();
	void clearData();
	void freshData_test();


};

} // namespace Intro
