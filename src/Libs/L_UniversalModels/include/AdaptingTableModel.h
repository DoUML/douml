// *************************************************************************
//
// Copyright 2012-2013 Nikolai Marchenko.
//
// This file is part of the Douml Uml Toolkit.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License Version 3.0 as published by
// the Free Software Foundation and appearing in the file LICENSE.GPL included in the
//  packaging of this file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License Version 3.0 for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// e-mail : doumleditor@gmail.com
//
// *************************************************************************

#ifndef _ADAPTINGTABLEMODEL_H
#define _ADAPTINGTABLEMODEL_H


#include <QObject>
#include <QVariant>
#include <QModelIndex>
#include <QSharedPointer>

#include "l_tree_controller_global.h"

#include <QAbstractTableModel>

class TableDataInterface;
class AdaptingTableModelPrivate;


class L_TREE_CONTROLLER_EXPORT AdaptingTableModel  : public QAbstractTableModel
{

Q_OBJECT

public:
    AdaptingTableModel(QObject * parent = 0);

    virtual ~AdaptingTableModel();

    virtual QVariant data(const QModelIndex & index, int role) const override;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role) override;
    virtual int rowCount(const QModelIndex & index) const override;
    virtual int columnCount(const QModelIndex & index) const override;
    void SetInterface(QSharedPointer<TableDataInterface> _interface);
    virtual QModelIndex index(int row, int column, const QModelIndex & parent) const override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    int RowForValue(void* value);

    using QAbstractTableModel::sort;
    void sort();

    void RemoveRow(const QModelIndex & index);

public slots:
    void OnReloadDataFromInterface();

private:
Q_DECLARE_PRIVATE(AdaptingTableModel)

protected:
     AdaptingTableModelPrivate* const d_ptr;
};

#endif
