/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "treeitem.h"
#include "treemodel.h"
#include "contact/contacttreeview.h"
#include "contactdelegate.h"

namespace Contact {

class FilterWidget;

class Dialog : public QDialog {
	Q_OBJECT

public:
	Dialog(QWidget *parent = 0);
	~Dialog();

	virtual void accept() override;

private slots:
	void on__btnNewGroup_clicked();
	void textFilterChanged();

private:
	QLabel* _labTitle;
	FilterWidget* _filterWidget;
	ContactTreeView* _contactTree;
	QVBoxLayout* _vLayout;
	QHBoxLayout* _hLayoutStyle;

	
	QPushButton* _btnNewContact;
	QPushButton* _btnClose;
	QVector<ContactInfo*> _vecContactPData;
protected:
	virtual void closeEvent(QCloseEvent* event) override;


private:
	void init();
	void freshData();
	void freshData_test();
};

} // namespace Intro
