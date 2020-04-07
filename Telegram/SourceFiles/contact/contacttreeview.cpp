/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "contact/contacttreeview.h"
#include "contact/mysortfilterproxymodel.h"
#include "lang/lang_keys.h"
#include "auth_session.h"
#include "data/data_session.h"

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

void ContactTreeView::loadDatas(const QVector<ContactInfo*> vecContactPData)
{
	_vecContactPData = vecContactPData;
	_sortFilterProxyModel->clear();
	_contactModel->setupModelData(vecContactPData);
	_sortFilterProxyModel->setSourceModel(_contactModel);

	//设置父节点的折叠展开状态
	for (int i = 0; i < _sortFilterProxyModel->rowCount(); i++)
	{
		QModelIndex index = _sortFilterProxyModel->index(i, 0);
		ContactInfo* pCI = dynamic_cast<ContactInfo*>(index.data(Qt::DisplayRole).value<ContactInfo*>());
		if (pCI && pCI->isGroup && pCI->expanded)
		{
			setExpanded(index, true);
		}
		
	}
	
}

void ContactTreeView::setSearchKey(const QString& searchKey)
{
	_searchKey = searchKey;
	_sortFilterProxyModel->setSearchKey(searchKey);
}


QVector<MTPlong> ContactTreeView::getCheckedGroup()
{
	QVector<MTPlong> userGroups;

	for (int i = 0; i < _vecContactPData.size(); ++i) {
		if (_vecContactPData[i]->userInGroup)
		{
			userGroups.push_back(MTP_long(_vecContactPData[i]->id));
		}
	}
	return userGroups;
}

void ContactTreeView::slotCustomContextMenu(const QPoint& point)
{
	if (CTT_FULL == _ctt) {
		if (_searchKey.isEmpty() || _searchKey.isNull())
		{
			QMenu* menu = new QMenu(this);
			QModelIndex curIndex = indexAt(point);
			ContactInfo* pCI = dynamic_cast<ContactInfo*>(curIndex.data(Qt::DisplayRole).value<ContactInfo*>());

			
			if (pCI && pCI->isGroup && pCI->otherId == 0)
			{
				menu->addAction(lang(lng_dlg_contact_group_add), this, SLOT(slotAddGroup()));
				QAction* pModGroup = new QAction(lang(lng_dlg_contact_group_rename), this);
				pModGroup->setData(QVariant::fromValue(pCI));
				connect(pModGroup, SIGNAL(triggered()), this, SLOT(slotModGroup()));
				menu->addAction(pModGroup);

				QAction* pDelGroup = new QAction(lang(lng_dlg_contact_group_del), this);
				pDelGroup->setData(QVariant::fromValue(pCI));
				connect(pDelGroup, SIGNAL(triggered()), this, SLOT(slotDelGroup()));
				menu->addAction(pDelGroup);
			}
			//if (pCI && !pCI->isGroup) {
			//	QAction* pUserInfo = new QAction(lang(lng_context_view_profile), this);
			//	pUserInfo->setData(QVariant::fromValue(pCI));
			//	connect(pUserInfo, SIGNAL(triggered()), this, SLOT(slotUserInfo()));
			//	menu->addAction(pUserInfo);
			//}
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

void ContactTreeView::slotUserInfo()
{
	QAction* pSendMsg = nullptr;
	ContactInfo* pCI = nullptr;
	do
	{
		pSendMsg = qobject_cast<QAction*>(sender());
		pCI = pSendMsg->data().value<ContactInfo*>();
	} while (false);
	emit showUserInfo(pCI);
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
				if (peer != nullptr)
				{
					//qDebug() << peer->id << peer->name << peer->nameText.toString();
					//qDebug() << (Auth().data().peerLoaded(peer->id) == nullptr);
					//Ui::showPeerHistory(peer->id, ShowAtUnreadMsgId);
					emit startChat(peer->id);
				}

			}
		}
		
	});

	// 双击打开聊天
	connect(this, &QTreeView::doubleClicked, [&](const QModelIndex& index)
	{
		if (CTT_TOSELECT == _ctt || CTT_SHOW == _ctt)
		{
			ContactInfo* pCI = dynamic_cast<ContactInfo*>(index.data(Qt::DisplayRole).value<ContactInfo*>());
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
	if (_ctt == CTT_SWITCH) {
		// 自定义点击事件
			//connect(this, QOverload<const QModelIndex&, int>::of(&ContactTreeView::signalClicked), this, &ContactTreeView::onClickedHandle);
		connect(this, static_cast<void (ContactTreeView::*)(const QModelIndex&, int)>(&ContactTreeView::signalClicked), this, &ContactTreeView::onClickedHandle);
	}
	}
	

void ContactTreeView::onClickedHandle(const QModelIndex& index, int role)
{
	ContactInfo* pCI = dynamic_cast<ContactInfo*>(index.data(Qt::DisplayRole).value<ContactInfo*>());
	switch (role)
	{
		case static_cast<int>(CustomRole::SwitchRole) : 
		{
			emit switchUser(pCI);
			break;
		}
		default:
		{
			break;
		}
	}
}

bool ContactTreeView::viewportEvent(QEvent* pEvent)
{
	bool result = QTreeView::viewportEvent(pEvent);
	QEvent::Type eventType = pEvent->type();
	int role = -1;

	switch (eventType)
	{
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonRelease:
	case QEvent::MouseButtonDblClick:
	{
		int role = -1;
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(pEvent);
		QModelIndex modelIndex = indexAt(mouseEvent->pos());
		QStyleOptionViewItemV4 option = viewOptions();
		option.rect = visualRect(modelIndex);
		option.widget = this;

		QMetaObject::invokeMethod(qobject_cast<ContactDelegate*>(itemDelegate(modelIndex)), "mouseEvent",
			Q_RETURN_ARG(int, role),
			Q_ARG(QMouseEvent*, mouseEvent),
			Q_ARG(QAbstractItemView*, this),
			Q_ARG(QStyleOptionViewItem, option),
			Q_ARG(QModelIndex, modelIndex));

		if (modelIndex.isValid())
		{
			if (eventType == QEvent::MouseButtonRelease &&
				Qt::LeftButton == mouseEvent->button())
			{
				// 左键按下
				if (role != -1)
				{
					emit signalClicked(modelIndex, role);
				}
			}
		}
	}
	break;
	default:
		break;
	}

	return result;
}

} // namespace Contact