/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
    treeitem.cpp

    A container for items of data supplied by the simple tree model.
*/

#include <QStringList>

#include "treeitem.h"
#include "datadefine.h"

namespace Contact {

    //! [0]
    TreeItem::TreeItem(ContactInfo* pCI, TreeItem* parent)
    {
        m_parentItem = parent;
        m_pCI = pCI;
    }
    //! [0]

    //! [1]
    TreeItem::~TreeItem()
    {
        qDeleteAll(m_childItems);
        m_childItems.clear();
        m_pCI = nullptr;
    }
    //! [1]

    //! [2]
    void TreeItem::appendChild(TreeItem* item)
    {
        m_childItems.append(item);
    }
    //! [2]

    //! [3]
    TreeItem* TreeItem::child(int row)
    {
        return m_childItems.value(row);
    }
    //! [3]

    //! [4]
    int TreeItem::childCount() const
    {
        if (m_pCI)
        {
            return m_childItems.count();
        }
        return 0;
    }
    //! [4]

    //! [5]
    int TreeItem::columnCount() const
    {
        //return m_itemData.count();
        return 1;
    }
    //! [5]

    //! [6]
    //QVariant TreeItem::data(int column) const
    //{
    //    //return m_itemData.value(column);
    //    return QVariant::fromValue((void*)m_pCI);
    //    
    //}
    ContactInfo* TreeItem::data() const
    {
        return m_pCI;
    }
    //! [6]

    //! [7]
    TreeItem* TreeItem::parentItem()
    {
        return m_parentItem;
    }

    bool TreeItem::setExtDataExpanded(int column, bool value)
    {
        //ContactInfo* pCI = (ContactInfo*)m_itemData[column].value<void*>();
        //if (!value) {
        //    int xx = 0;
        //}
        m_pCI->expanded = value;
        //qDebug() << "TreeItem::setExtDataExpanded[" << this << this->m_pCI << "]" << m_pCI << m_pCI->expanded; // << m_pCI->firstName;
        return true;
    }


	bool TreeItem::removeChildren(int position, int count)
	{
		if (position < 0 || position + count > m_childItems.size())
			return false;

		for (int row = 0; row < count; ++row)
			delete m_childItems.takeAt(position);

		return true;
	}

	//! [7]

    //! [8]
    int TreeItem::row() const
    {
        if (m_parentItem)
            return m_parentItem->m_childItems.indexOf(const_cast<TreeItem*>(this));

        return 0;
    }
    //! [8]
}