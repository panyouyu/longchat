/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
    contactdelegate.cpp

    A delegate that allows the user to change integer values from the model
    using a spin box widget.
*/

#include "contactdelegate.h"
#include "delegatehelper.h"
#include "ui/image/image.h"
#include "mainwidget.h"
#include "styles/style_contact.h"

namespace Contact {
	const QRect GroupArrorIconRect{ 5,12,10,10 }; // 分组折叠箭头区域
	const QRect GroupCheckBoxRect{ 23,8,22,22 }; // 分组复选区域
	const QRect SwitchBtnRect{ 307,17,54,28 }; // 转接矩形区域
	const QRect SwitchBtnTxtRect{ 322,22,24,17 }; // 转接文字矩形区域
	const int ArrorRectWidth = 20;

	ContactDelegate::ContactDelegate(CreatingTreeType ctt, QObject* parent)
		: QItemDelegate(parent)
		, m_ctt(ctt)
	{
		m_si.fontColor = QColor("#373737");
		m_si.userCountFontColor = QColor(186, 186, 186);
		m_si.avatarHeight = 48;
		m_si.avatarWidth = 48;
		m_si.fontSize = 14;

	}

	ContactDelegate::~ContactDelegate()
	{
	}

	void ContactDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		Q_ASSERT(index.isValid());
		if (0 == index.column())
		{
			ContactInfo* pCI = dynamic_cast<ContactInfo*>(index.data(Qt::DisplayRole).value<ContactInfo*>());
			if (pCI == nullptr)
			{
				return;
			}
			//bool isSelected = (option.state & QStyle::State_Selected) == QStyle::State_Selected ? true : false;
			bool isHovered = (option.state & QStyle::State_MouseOver) == QStyle::State_MouseOver ? true : false;
			QColor color;
			color = (isHovered) ? QColor("#f0f0f0") : QColor("#ffffff"); // hover时变灰
			// 背景色
			painter->setPen(Qt::NoPen);
			painter->setBrush(color);
			painter->drawRect(option.rect);

			if (pCI->isGroup) {
				paintGroup(painter, option, index, pCI);
			}
			else
			{
				paintUser(painter, option, index, pCI);
			}
		}
		else {
			QString text = index.data(Qt::DisplayRole).toString();
			if (!text.isEmpty())
			{
				QRect cellRect = option.rect;
				painter->drawText(cellRect, Qt::AlignLeft, text);
			}
		}
	}

	void ContactDelegate::paintGroup(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, ContactInfo* pCI) const
	{
		DelegateHelper delegateHelper;
		if (m_ctt == CTT_USERINGROUP)
		{
			//画复选框
			bool checked = pCI->userInGroup;

			// 箭头图标区域
			QRect ceckBoxRect = calGroupCheckBoxRect(option);

			QPixmap indicatorPixmap;
			QStyleOptionButton check_box_style_option;
			if (checked)
			{
				check_box_style_option.state |= QStyle::State_On;
				indicatorPixmap = QPixmap(":/gui/images/checkbox_checked.png");
			}
			else
			{
				check_box_style_option.state |= QStyle::State_Off;
				indicatorPixmap = QPixmap(":/gui/images/checkbox_unchecked.png");
			}
			QApplication::style()->drawItemPixmap(painter, ceckBoxRect, 0, indicatorPixmap);

			

			/////////画组名/////
			QString groupName = pCI->firstName;
			int groupFontSize = 15;
			QRect grouNameRect = option.rect;
			int left = GroupCheckBoxRect.x() + GroupCheckBoxRect.width() + ArrorRectWidth;
			grouNameRect.setLeft(option.rect.left() + left);
			QRect groupTextRec = delegateHelper.calTextRect(painter, groupFontSize, groupName);
			grouNameRect.setTop(option.rect.top() + (option.rect.height() - groupTextRec.height()) / 2);
			delegateHelper.paintText(painter, Qt::AlignLeft, m_si.fontColor, grouNameRect, groupFontSize, groupName);
		}
		else {
			//////////绘制折叠箭头区域//////////
			if (pCI->userTotalCount > 0)
			{
				// 箭头图标区域
				QRect arrowIconRect(option.rect.left() + GroupArrorIconRect.x(), option.rect.top() + GroupArrorIconRect.y(),
					GroupArrorIconRect.width(), GroupArrorIconRect.height());
				QString arrorPath{ ":/gui/images/ic_arrow_open.png" };
				if (!index.data(static_cast<int>(CustomRole::IsExpandedRole)).toBool())
				{
					arrorPath = ":/gui/images/ic_arrow_close.png";
				}
				painter->drawPixmap(arrowIconRect, QPixmap(arrorPath));
			}

			/////////画组名/////
			QString groupName = pCI->firstName;
			int groupFontSize = 15;
			QRect grouNameRect = option.rect;
			grouNameRect.setLeft(option.rect.left() + ArrorRectWidth);
			QRect groupTextRec = delegateHelper.calTextRect(painter, groupFontSize, groupName);
			grouNameRect.setTop(option.rect.top() + (option.rect.height() - groupTextRec.height()) / 2);
			delegateHelper.paintText(painter, Qt::AlignLeft, m_si.fontColor, grouNameRect, groupFontSize, groupName);
			//////////////画组成员数量////////////////////////////////////
			int groupUserInfoFontSize = 12;
			QString userCountInfo = pCI->showUserCount;
			QRect groupUserInfoTextRec = delegateHelper.calTextRect(painter, groupUserInfoFontSize, userCountInfo);
			//字符串所占的像素宽度,高度
			int textWidth = groupUserInfoTextRec.width();
			int textHeight = groupUserInfoTextRec.height();
			QRect grouUserInfoRect = option.rect;
			grouUserInfoRect.setTop(option.rect.top() + (option.rect.height() - textHeight) / 2);
			grouUserInfoRect.setLeft(option.rect.width() - textWidth);
			delegateHelper.paintText(painter, Qt::AlignLeft, m_si.userCountFontColor, grouUserInfoRect, groupUserInfoFontSize, userCountInfo);
		}
		

	}

	void ContactDelegate::paintUser(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, ContactInfo* pCI) const
	{
		DelegateHelper delegateHelper;
		QString name = pCI->firstName + " " + pCI->lastName;
		if (m_ctt == CTT_SWITCH) {
			name = pCI->firstName;
		}
		QRect cellRect = option.rect;
		cellRect.setLeft(10);
		QRect avatarRect = cellRect;
		avatarRect.setTop(avatarRect.top() + (avatarRect.height() - m_si.avatarHeight) / 2);
		avatarRect.setWidth(m_si.avatarWidth);
		avatarRect.setHeight(m_si.avatarHeight);
		if (pCI->hasAvatar)
		{
			//QPixmap pic = rixmapToRound(QPixmap(":/gui/art/sunrise.jpg"), 24);
			//QPixmap pic(pCI->pAvatarImage->pixCircled(userpicOrigin(), m_si.avatarWidth, m_si.avatarHeight));
			painter->drawPixmap(avatarRect, pCI->peerData->genUserpic(m_si.avatarWidth));//.scaled(48, 48)
		}
		else
		{
			QColor randomColor(255, 160, 90);
			delegateHelper.paintEllipse(painter, randomColor, avatarRect);
			QString twoChar = pCI->firstName.left(2);
			delegateHelper.paintText(painter, Qt::AlignCenter, QColor(255, 255, 255), avatarRect, 12, twoChar);
		}
		///////////画姓名///////////
		int heightRevise = 5;
		int pointSize = 6;
		QRect nameRect = cellRect;
		int halfHeight = cellRect.height() / 2;
		QRect recName = delegateHelper.calTextRect(painter, m_si.fontSize, name);
		int showNameHeight = recName.height();
		nameRect.setLeft(cellRect.left() + avatarRect.width() + ArrorRectWidth);
		nameRect.setTop(cellRect.top() + (halfHeight - showNameHeight) / 2 + heightRevise);
		delegateHelper.paintText(painter, Qt::AlignLeft, m_si.fontColor, nameRect, m_si.fontSize, name);

		
		QString midbutStr = pCI->lastLoginTime;
		if (m_ctt == CTT_SWITCH)
		{
			midbutStr = pCI->serverCount + " " + pCI->queueCount;

			// 画绿点红点
			QRect pointIconRect(cellRect.left() + avatarRect.width() + ArrorRectWidth, cellRect.top() + halfHeight + (halfHeight - pointSize) / 2 - heightRevise,
				pointSize , pointSize);
			QString arrorPath{ ":/gui/art/ic_point_greed.png" };
			if (pCI->queueNum > 0) //排队人数大于0 显示红色
			{
				arrorPath = ":/gui/art/ic_point_red.png";
			}
			painter->drawPixmap(pointIconRect, QPixmap(arrorPath));
			//画服务人数
			QRect recentLoginRect = cellRect;
			QRect recRecLogin = delegateHelper.calTextRect(painter, m_si.fontSize, midbutStr);
			showNameHeight = recRecLogin.height();
			recentLoginRect.setLeft(cellRect.left() + avatarRect.width() + ArrorRectWidth + pointIconRect.width() + heightRevise);
			recentLoginRect.setTop(cellRect.top() + halfHeight + (halfHeight - showNameHeight) / 2 - heightRevise);
			delegateHelper.paintText(painter, Qt::AlignLeft, m_si.userCountFontColor, recentLoginRect, m_si.fontSize, midbutStr);
			
		}
		else  //画最近登录时间
		{
			QRect recentLoginRect = cellRect;
			QRect recRecLogin = delegateHelper.calTextRect(painter, m_si.fontSize, midbutStr);
			showNameHeight = recRecLogin.height();
			recentLoginRect.setLeft(cellRect.left() + avatarRect.width() + ArrorRectWidth);
			recentLoginRect.setTop(cellRect.top() + halfHeight + (halfHeight - showNameHeight) / 2 - heightRevise);
			delegateHelper.paintText(painter, Qt::AlignLeft, m_si.userCountFontColor, recentLoginRect, m_si.fontSize, midbutStr);
		}
		

		if (m_ctt == CTT_SWITCH)
		{
			//////////////画转接按钮////////////////////////////////////
			QString userSwitchInfo = pCI->lastName;

			QRect switchUserInfoBackRect = calSwitchUserInfoBackRect(option);
			QRect switchUserInfoRect = option.rect; ;
			switchUserInfoRect.setTop(option.rect.top() + SwitchBtnTxtRect.y());
			switchUserInfoRect.setLeft(SwitchBtnTxtRect.x());
			switchUserInfoRect.setWidth(SwitchBtnTxtRect.width());
			switchUserInfoRect.setHeight(SwitchBtnTxtRect.height());
			delegateHelper.paintRect(painter, QColor(st::switchButton.bgcolor->c), switchUserInfoBackRect);
			delegateHelper.paintText(painter, Qt::AlignLeft, QColor(st::switchButton.fontcolor->c), switchUserInfoRect, st::switchButton.fontsize, userSwitchInfo);

		}
		
	}

	QRect ContactDelegate::calSwitchUserInfoBackRect(const QStyleOptionViewItem& option/*, int textWidth, int textHeight, int marginRight*/) const {
		QRect switchUserInfoBackRect = option.rect;
		switchUserInfoBackRect.setTop(option.rect.top() + SwitchBtnRect.y() );
		switchUserInfoBackRect.setLeft(SwitchBtnRect.x());
		switchUserInfoBackRect.setWidth(SwitchBtnRect.width());
		switchUserInfoBackRect.setHeight(SwitchBtnRect.height());
		//qDebug() << "----" << option.rect.top() << SwitchBtnRect.y();
		return switchUserInfoBackRect;
	}

	QRect ContactDelegate::calGroupCheckBoxRect(const QStyleOptionViewItem& option) const
	{
		return QRect(option.rect.left() + GroupCheckBoxRect.x(), option.rect.top() + GroupCheckBoxRect.y(),
			GroupCheckBoxRect.width(), GroupCheckBoxRect.height());
	}

	QSize ContactDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		Q_ASSERT(index.isValid());

		ContactInfo* pCI = dynamic_cast<ContactInfo*>(index.data(Qt::DisplayRole).value<ContactInfo*>());
		QSize size = QItemDelegate::sizeHint(option, index);
		if (nullptr == pCI)
		{
			return size;
		}
		if (m_ctt == CTT_USERINGROUP)
		{
			size.setHeight(38);
		}
		else {
			if (pCI->isGroup) {
				size.setHeight(34);
			}
			else {
				size.setHeight(62);
			}
		}
		
		return size;
	}

	int ContactDelegate::getMouseEventRole(const QPoint& pos, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		//转接所在区域
		QRect switchRect = calSwitchUserInfoBackRect(option/*, 24, 17, 10*/);
		if (switchRect.contains(pos))
		{
			return static_cast<int>(CustomRole::SwitchRole);
		}

		return -1;
	}

	bool ContactDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
	{
		if (m_ctt == CTT_USERINGROUP) {
			//复选框区域
			QRect ceckBoxRect = calGroupCheckBoxRect(option);

			QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
			if (event->type() == QEvent::MouseButtonPress && ceckBoxRect.contains(mouseEvent->pos()))
			{
				bool data = model->data(index, CustomRole::GroupCheckRole).toBool();
				QMutexLocker lock(&App::main()->getUserGroupMutex());
				model->setData(index, !data, CustomRole::GroupCheckRole);
			}
		}
		
		return QItemDelegate::editorEvent(event, model, option, index);
	}

	Q_INVOKABLE int ContactDelegate::mouseEvent(QMouseEvent* mouseEvent, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& modelIndex)
	{
		if (nullptr == mouseEvent || nullptr == view || !modelIndex.isValid())
		{
			return -1;
		}

		auto curModel = view->model();
		if (nullptr == curModel)
		{
			return -1;
		}

		auto pos = mouseEvent->pos();

		//curModel->setData(modelIndex, pos, ItemRole::PosRole);

		//auto eventType = mouseEvent->type();
		//switch (eventType)
		//{
		//case QMouseEvent::MouseMove:
		//	curModel->setData(modelIndex, true, ItemRole::MouseHoverRole);
		//	break;
		//case QMouseEvent::MouseButtonPress:
		//	curModel->setData(modelIndex, true, ItemRole::MousePressRole);
		//	break;
		//case QMouseEvent::MouseButtonRelease:
		//	curModel->setData(modelIndex, true, ItemRole::MousePressRole);
		//	break;
		//default:
		//	break;
		//}

		//view->update(modelIndex);

		if (option.rect.contains(pos))
		{
			// 点到的区域
			return getMouseEventRole(pos, option, modelIndex);
		}

		return -1;
	}

	QPixmap ContactDelegate::rixmapToRound(const QPixmap& src, int radius) const
	{
		if (src.isNull()) {
			return QPixmap();
		}

		QSize size(2 * radius, 2 * radius);
		QBitmap mask(size);
		QPainter painter(&mask);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		painter.fillRect(0, 0, size.width(), size.height(), Qt::white);
		painter.setBrush(QColor(0, 0, 0));
		painter.drawRoundedRect(0, 0, size.width(), size.height(), 99, 99);

		QPixmap image = src.scaled(size);
		image.setMask(mask);
		return image;
	}
}