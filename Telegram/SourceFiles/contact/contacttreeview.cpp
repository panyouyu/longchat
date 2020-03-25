/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "contact/contacttreeview.h"
#include "contact/mysortfilterproxymodel.h"
#include "lang/lang_keys.h"

namespace Contact {
	

ContactTreeView::ContactTreeView(CreatingTreeType ctt, QWidget *parent) : QTreeView(parent), _ctt(ctt) {
	header()->hide();
	setIndentation(0);	// 左边距设置为0
	setAnimated(true);  // 展开时动画
	

	_contactModel = new TreeModel();
	_sortFilterProxyModel = new MySortFilterProxyModel();
	setModel(_sortFilterProxyModel);

	_contactDelegate = new ContactDelegate(_ctt, this);
	setItemDelegate(_contactDelegate);

	initConnection();
}

ContactTreeView::~ContactTreeView() {
	
}

void ContactTreeView::loadDatas(const QVector<ContactInfo*> _vecContactPData)
{
	_sortFilterProxyModel->clear();
	_contactModel->setupModelData(_vecContactPData);
	_sortFilterProxyModel->setSourceModel(_contactModel);
	
}

void ContactTreeView::setSearchKey(const QString& searchKey)
{
	_searchKey = searchKey;
	_sortFilterProxyModel->setSearchKey(searchKey);
}


void ContactTreeView::slotCustomContextMenu(const QPoint& point)
{
	if (CTT_FULL == _ctt) {
		if (_searchKey.isEmpty() || _searchKey.isNull())
		{
			QMenu* menu = new QMenu(this);
			QModelIndex curIndex = indexAt(point);
			ContactInfo* pCI = (ContactInfo*)curIndex.data(Qt::DisplayRole).value<void*>();

			menu->addAction(lang(lng_dlg_contact_group_add), this, SLOT(slotAddGroup()));
			if (pCI && pCI->isGroup && pCI->otherId == 0)
			{
				QAction* pModGroup = new QAction(lang(lng_dlg_contact_group_rename), this);
				pModGroup->setData(QVariant::fromValue(pCI));
				connect(pModGroup, SIGNAL(triggered()), this, SLOT(slotModGroup()));
				menu->addAction(pModGroup);

				QAction* pDelGroup = new QAction(lang(lng_dlg_contact_group_del), this);
				pDelGroup->setData(QVariant::fromValue(pCI));
				connect(pDelGroup, SIGNAL(triggered()), this, SLOT(slotDelGroup()));
				menu->addAction(pDelGroup);
			}
			menu->exec(this->mapToGlobal(point));
		}
	}
}

void ContactTreeView::slotAddGroup()
{
	//bool ok;
	//QString text = QInputDialog::getText(this, lang(lng_dlg_contact_group_name_input),
	//	lang(lng_dlg_contact_group_name), QLineEdit::Normal,
	//	"", &ok);
	//if (ok && !text.isEmpty())
	//{
	//	
	//}
	emit addGroup();
}

void ContactTreeView::slotModGroup()
{
	QAction* pSendMsg = nullptr;
	ContactInfo* pCI = nullptr;
	do
	{
		pSendMsg = qobject_cast<QAction*>(sender());
		pCI = pSendMsg->data().value<ContactInfo*>();
	} while (false);
	emit modGroup(pCI);
}

void ContactTreeView::slotDelGroup()
{
	QModelIndex index = selectionModel()->currentIndex();
	_sortFilterProxyModel->removeRow(index.row(), index.parent());

	QAction* pSendMsg = nullptr;
	ContactInfo* pCI = nullptr;
	do
	{
		pSendMsg = qobject_cast<QAction*>(sender());
		pCI = pSendMsg->data().value<ContactInfo*>();
	} while (false);
	emit delGroup(pCI);
}

void ContactTreeView::initConnection()
{
	this->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(slotCustomContextMenu(const QPoint&)));
	// 点击事件
	connect(this, &QTreeView::clicked, [&](const QModelIndex& index)
	{
		if (CTT_FULL == _ctt)
		{
			if (index.data(static_cast<int>(CustomRole::IsGroupRole)).toBool())
			{
				setExpanded(index, !isExpanded(index)); // 单击展开/收缩列表
			}
			else //打开聊天
			{
				PeerData* peer = (PeerData*)index.data(static_cast<int>(CustomRole::PeerRole)).value<void*>();
				if (peer)
				{
					Ui::showPeerHistory(peer, ShowAtUnreadMsgId);
					emit startChat();
				}

			}
		}
		
	});

	// 双击打开聊天
	connect(this, &QTreeView::doubleClicked, [&](const QModelIndex& index)
	{
		if (CTT_TOSELECT == _ctt || CTT_SHOW == _ctt)
		{
			ContactInfo* pCI = (ContactInfo*)index.data(Qt::DisplayRole).value<void*>();
			if (pCI != nullptr)
			{
				emit selectedUser(pCI);
			}
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
Q_DECLARE_METATYPE(Contact::ContactInfo*);