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
	ContactTreeView(CreatingTreeType ctt, QWidget *parent = 0);
	~ContactTreeView();

	// ��ֵ
	void loadDatas(const QVector<ContactInfo*> _vecContactPData);
	void setSearchKey(const QString& searchKey);

signals:
	void startChat();
	void addGroup();
	void modGroup(ContactInfo* pCI);
	void delGroup(ContactInfo* pCI);
	void selectedUser(PeerData*);
	void selectedUser(ContactInfo* pCI);
	void switchUser(ContactInfo* pCI);
	void showUserInfo(ContactInfo* pCI);
	// ���Item�ź� ���嵽�㵽��Role
	void signalClicked(const QModelIndex& iIndex, int role);

public slots:
	void slotCustomContextMenu(const QPoint& point);//�����Ҽ��˵��Ĳۺ���
	void slotAddGroup();
	void slotModGroup();
	void slotDelGroup();
	void slotUserInfo();
private:
	void initConnection();
	// �Զ���ĵ���¼�����
	void onClickedHandle(const QModelIndex& index, int role);
private:
	TreeModel* _contactModel{ nullptr };
	ContactDelegate* _contactDelegate{ nullptr };
	MySortFilterProxyModel* _sortFilterProxyModel{ nullptr };
	QString _searchKey;
	CreatingTreeType _ctt;

protected:
	virtual bool viewportEvent(QEvent* pEvent) override;

};

} // namespace Intro
