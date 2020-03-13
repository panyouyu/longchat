/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once
#include "datadefine.h"
#include "contactdelegate.h"
#include "treemodel.h"


namespace Contact {

class MySortFilterProxyModel;

class ContactTreeView : public QTreeView {
	Q_OBJECT

public:
	ContactTreeView(QWidget *parent = 0);
	~ContactTreeView();

	// ¸³Öµ
	void loadDatas(const QVector<ContactInfo*> _vecContactPData);
	void setSearchKey(const QString& searchKey);

signals:
	void startChat();
private:
	void initConnection();

private:
	TreeModel* _contactModel{ nullptr };
	ContactDelegate* _contactDelegate{ nullptr };
	MySortFilterProxyModel* _sortFilterProxyModel{ nullptr };

};

} // namespace Intro
