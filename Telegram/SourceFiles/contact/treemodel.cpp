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
    treemodel.cpp

    Provides a simple tree model to show how to create and use hierarchical
    models.
*/


#include "treemodel.h"

#include <QStringList>
#include <QDebug>

namespace Contact {

    //! [0]
    TreeModel::TreeModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
       /* QList<QVariant> rootData;
        rootData << "col1" << "col2";*/
        pCIHead = new ContactInfo();
        pCIHead->firstName = "header";
        headItem = new TreeItem(pCIHead);
        //qDebug() << "---" << pCIHead << pCIHead->firstName << " :" << pCIHead->expanded;
    }
    //! [0]

    //! [1]
    TreeModel::~TreeModel()
    {
        if (nullptr != pCIHead) 
        {
            delete pCIHead;
        }
        if (nullptr != headItem)
        {
            delete headItem;
        }
        
        
    }
    //! [1]

    //! [2]
    int TreeModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
            return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
        else
            return headItem->columnCount();
    }

    bool TreeModel::setExtDataExpanded(const QModelIndex& index, bool value)
    {
        //qDebug() << "TreeModel::setExtDataExpanded" << index ;
        bool result = false;
        TreeItem* item = getItem(index);
        result = item->setExtDataExpanded(index.column(), value);
        return result;
    }


  /*  QVariant TreeModel::extData(const QModelIndex& index, int column)
    {
        if (!index.isValid())
            return QVariant();
        TreeItem* item = getItem(index);
        return item->data(column);
    }*/


    QVariant TreeModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        if (role != Qt::DisplayRole && CustomRole::IsExpandedRole != role
            && CustomRole::IsGroupRole != role 
            && CustomRole::PeerRole != role)
            return QVariant();

        TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
        if (CustomRole::IsExpandedRole == role)
        {
			//ContactInfo* pCI = (ContactInfo*)item->data(index.column()).value<void*>();
            ContactInfo* pCI = item->data();
            //qDebug() <<"paint:" << pCI << pCI->expanded << pCI->firstName;
            return item->data()->expanded;
        }
        else if (CustomRole::IsGroupRole == role)
        {
            //ContactInfo* pCI = (ContactInfo*)item->data(index.column()).value<void*>();
            ContactInfo* pCI = item->data();
            return pCI->isGroup;
        }
        else if (CustomRole::PeerRole == role)
        {
			//ContactInfo* pCI = (ContactInfo*)item->data(index.column()).value<void*>();
            ContactInfo* pCI = item->data();
			return QVariant::fromValue((void*)pCI->peerData);
        }
        ContactInfo* pCI = item->data();

        return QVariant::fromValue((void*)pCI);
    }
    //! [3]

    //! [4]
    Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
            return 0;

        return QAbstractItemModel::flags(index);
    }
    //! [4]

    //! [5]
    QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
        int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
            return QVariant::fromValue((void*)headItem->data()); 

        return QVariant();
    }
    //! [5]

    //! [6]
    QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent)
        const
    {
        if (!hasIndex(row, column, parent))
            return QModelIndex();

        TreeItem* parentItem;

        if (!parent.isValid())
            parentItem = headItem;
        else
            parentItem = static_cast<TreeItem*>(parent.internalPointer());

        TreeItem* childItem = parentItem->child(row);
        if (childItem)
            return createIndex(row, column, childItem);
        else
            return QModelIndex();
    }
    //! [6]

    //! [7]
    QModelIndex TreeModel::parent(const QModelIndex& index) const
    {
        if (!index.isValid())
            return QModelIndex();

        TreeItem* childItem = static_cast<TreeItem*>(index.internalPointer());
        TreeItem* parentItem = childItem->parentItem();

        if (parentItem == headItem)
            return QModelIndex();

        return createIndex(parentItem->row(), 0, parentItem);
    }
    //! [7]

    //! [8]
    int TreeModel::rowCount(const QModelIndex& parent) const
    {
        TreeItem* parentItem;
        if (parent.column() > 0)
            return 0;

        if (!parent.isValid())
            parentItem = headItem;
        else
            parentItem = static_cast<TreeItem*>(parent.internalPointer());

        return parentItem->childCount();
    }
    //! [8]

    void TreeModel::setupModelData(const QVector<ContactInfo*>& vecData)
    {
        removeRows(0, rowCount());
        QList<TreeItem*> parents;
		if (nullptr != headItem)
		{
			delete headItem;
            headItem = new TreeItem(pCIHead);
		}
        parents << headItem; //第一项放标题头，其下的都为树项数据
        //添加第一级目录数据
        QList<TreeItem*> top1List;
        for (int i = 0; i < vecData.size(); ++i) {
            //ContactInfo* pMfi = static_cast<ContactInfo*>(vecData.at(i));
            ContactInfo* pMfi = vecData.at(i);
            if (pMfi->parentId == DEFAULT_VALUE_ZERO) {
                QList<QVariant> columnData;
                //只显示2两列，第1列firstName 第2列为是否展开
                //columnData << QVariant::fromValue((void*)pMfi) << 0;
                //qDebug() << mfi.firstName << mfi.id;
                TreeItem* _item = new TreeItem(pMfi, parents.last());
                parents.last()->appendChild(_item);
                top1List << _item;
            }
        }
        //添加第二级目录数据
        for (int i = 0; i < top1List.size(); ++i) {
            //ContactInfo* pCI = (ContactInfo*)top1List.at(i)->data(0).value<void*>();
            ContactInfo* pCI = top1List.at(i)->data();
            uint32_t curId = pCI->id;
            //qDebug() << top1List.at(i)->data(0).toString() << top1List.at(i)->data(1).toString() << curId;
            for (int j = 0; j < vecData.size(); ++j) {
                ContactInfo* mfi = vecData.at(j);
                if (mfi->parentId == curId) {
                    //QList<QVariant> columnData;
                    //只显示2两列，第1列firstName 第2列为是否展开
                    //columnData << QVariant::fromValue((void*)mfi) << 0;
                    TreeItem* _item = new TreeItem(mfi, top1List.at(i));
                    top1List[i]->appendChild(_item);
                }

            }
        }
        PrintNodeData(headItem);
    }

    TreeItem* TreeModel::getItem(const QModelIndex& index) const
    {
        if (index.isValid()) {
            TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
            if (item)
            {
                //qDebug() << "TreeModel::getItem[" << item << item->m_pCI << "]";
                return item;
            }
        }
        return headItem;
    }


    void TreeModel::PrintNodeData(TreeItem* item)
    {
        Q_ASSERT(item);
        ContactInfo* pCIHeader = item->data();
        //qDebug() << "***["<< item << item->m_pCI <<"]" << pCIHeader << pCIHeader->firstName << " :" << pCIHeader->expanded;
        if (item->childCount() > 0)
        {
            for (int i = 0; i < item->childCount(); i++)
            {
                TreeItem* childitem = item->child(i);
                PrintNodeData(childitem);
            }
        }
    }
}