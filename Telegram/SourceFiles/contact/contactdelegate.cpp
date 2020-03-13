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

namespace Contact {
	const QRect GroupArrorIconRect{ 5,12,10,10 }; // 分组折叠箭头区域
	const int ArrorRectWidth = 20;

	ContactDelegate::ContactDelegate(QObject* parent)
		: QItemDelegate(parent)
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
			ContactInfo* pCI = (ContactInfo*)index.data(Qt::DisplayRole).value<void*>();
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

			if (pCI->parentId == DEFAULT_VALUE_ZERO) {
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
		//////////绘制折叠箭头区域//////////
		if (pCI->userTotalCount > 0)
		{
			// 箭头图标区域
			QRect arrowIconRect(option.rect.left() + GroupArrorIconRect.x(), option.rect.top() + GroupArrorIconRect.y(),
				GroupArrorIconRect.width(), GroupArrorIconRect.height());
			QString arrorPath{ ":/gui/art/ic_arrow_open.png" };
			if (!index.data(static_cast<int>(CustomRole::IsExpandedRole)).toBool())
			{
				arrorPath = ":/gui/art/ic_arrow_close.png";
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

	void ContactDelegate::paintUser(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, ContactInfo* pCI) const
	{
		DelegateHelper delegateHelper;
		QString name = pCI->firstName + " " + pCI->lastName;
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
		QRect nameRect = cellRect;
		int halfHeight = cellRect.height() / 2;
		QRect recName = delegateHelper.calTextRect(painter, m_si.fontSize, name);
		int showNameHeight = recName.height();
		nameRect.setLeft(cellRect.left() + avatarRect.width() + ArrorRectWidth);
		nameRect.setTop(cellRect.top() + (halfHeight - showNameHeight) / 2 + heightRevise);
		delegateHelper.paintText(painter, Qt::AlignLeft, m_si.fontColor, nameRect, m_si.fontSize, name);

		//画最近登录时间
		QString lastTime = pCI->lastLoginTime;
		QRect recentLoginRect = cellRect;
		QRect recRecLogin = delegateHelper.calTextRect(painter, m_si.fontSize, lastTime);
		showNameHeight = recRecLogin.height();
		recentLoginRect.setLeft(cellRect.left() + avatarRect.width() + ArrorRectWidth);
		recentLoginRect.setTop(cellRect.top() + halfHeight + (halfHeight - showNameHeight) / 2 - heightRevise);
		delegateHelper.paintText(painter, Qt::AlignLeft, m_si.userCountFontColor, recentLoginRect, m_si.fontSize, lastTime);
	}

	QSize ContactDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		Q_ASSERT(index.isValid());

		ContactInfo* pCI = (ContactInfo*)index.data(Qt::DisplayRole).value<void*>();
		QSize size = QItemDelegate::sizeHint(option, index);
		if (nullptr == pCI)
		{
			return size;
		}
		if (pCI->parentId == DEFAULT_VALUE_ZERO) {
			size.setHeight(34);
		}
		else {
			size.setHeight(62);
		}
		return size;
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