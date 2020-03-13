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


#include "mysortfilterproxymodel.h"
#include "datadefine.h"
#include "contact/treemodel.h"
namespace Contact {

    //! [0]
    MySortFilterProxyModel::MySortFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
        //setRecursiveFilteringEnabled(true);
    }
    //! [0]
    void MySortFilterProxyModel::setSearchKey(const QString& searchKey)
    {
        m_searchKey = searchKey;
        invalidateFilter();
    }

	bool MySortFilterProxyModel::setExtDataExpanded(const QModelIndex& index, bool value)
	{
        QModelIndex sourceIndex = QSortFilterProxyModel::mapToSource(index);
        //qDebug() << "MySortFilterProxyModel::setExtDataExpanded" << index << value << sourceIndex;
        return (dynamic_cast<TreeModel*>(sourceModel()))->setExtDataExpanded(sourceIndex, value);
	}
    


	QModelIndex MySortFilterProxyModel::mapToSource(const QModelIndex& proxyIndex) const
	{
        QModelIndex index = QSortFilterProxyModel::mapToSource(proxyIndex);
        //qDebug() << " proxyIndex" << proxyIndex << "index" << index;
        return QSortFilterProxyModel::mapToSource(proxyIndex);
	}

	//! [2]

    //! [3]
    bool MySortFilterProxyModel::filterAcceptsRow(int sourceRow,
        const QModelIndex& sourceParent) const
    {
        QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);
        if (m_searchKey.isEmpty() || m_searchKey.isNull()) {
            return true;
        }
        ContactInfo* pCI = static_cast<ContactInfo*>(index0.data(Qt::DisplayRole).value<void*>());
        if (pCI != nullptr) {
            return (pCI->firstName.contains(m_searchKey) || pCI->lastName.contains(m_searchKey));
        }
        return false;
    }

    bool MySortFilterProxyModel::lessThan(const QModelIndex& left,
        const QModelIndex& right) const
    {
        //QVariant leftData = sourceModel()->data(left);
        //QVariant rightData = sourceModel()->data(right);
        //MailFolderInfo* pCILeft = (MailFolderInfo*)left.data(Qt::DisplayRole).value<void*>();
        //MailFolderInfo* pCIRight = (MailFolderInfo*)right.data(Qt::DisplayRole).value<void*>();
        //QString leftString = left.data(Qt::DisplayRole).toString();
        //QString rightString = right.data(Qt::DisplayRole).toString();
        //return QString::localeAwareCompare(leftString, rightString) < 0;
        return QSortFilterProxyModel::lessThan(left, right);
    }

}