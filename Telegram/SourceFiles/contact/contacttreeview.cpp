/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "contact/contacttreeview.h"
#include "contact/mysortfilterproxymodel.h"


namespace Contact {


ContactTreeView::ContactTreeView(QWidget *parent) : QTreeView(parent) {
	header()->hide();
	setIndentation(0);	// 左边距设置为0
	setAnimated(true);  // 展开时动画
	

	_contactModel = new TreeModel();
	_sortFilterProxyModel = new MySortFilterProxyModel();
	setModel(_sortFilterProxyModel);

	_contactDelegate = new ContactDelegate(this);
	setItemDelegate(_contactDelegate);

	initConnection();
}

ContactTreeView::~ContactTreeView() {
	
}

void ContactTreeView::loadDatas(const QVector<ContactInfo*> _vecContactPData)
{
	_contactModel->setupModelData(_vecContactPData);
	_sortFilterProxyModel->setSourceModel(_contactModel);
	hideColumn(1);
}

void ContactTreeView::setSearchKey(const QString& searchKey)
{
	_sortFilterProxyModel->setSearchKey(searchKey);
}

void ContactTreeView::initConnection()
{

	// 点击事件
	connect(this, &QTreeView::clicked, [&](const QModelIndex& index)
	{
		if (index.data(static_cast<int>(CustomRole::IsGroupRole)).toBool())
		{
			setExpanded(index, !isExpanded(index)); // 单击展开/收缩列表
		}
	});

	// 展开时更换左侧的展开图标
	connect(this, &QTreeView::expanded, [&](const QModelIndex& index)
	{
			_sortFilterProxyModel->setExtDataExpanded(index, true);  //_contactModel _sortFilterProxyModel
	});

	// 收起时更换左侧的展开图标
	connect(this, &QTreeView::collapsed, [&](const QModelIndex& index)
	{
			_sortFilterProxyModel->setExtDataExpanded(index, false); //_contactModel _sortFilterProxyModel
	});
}

} // namespace Contact
