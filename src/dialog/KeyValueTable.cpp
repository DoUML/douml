// *************************************************************************
//
// Copyright 2004-2010 Bruno PAGES  .
// Copyright 2012-2013 Nikolai Marchenko.
// Copyright 2012-2013 Leonardo Guilherme.
//
// This file is part of the DOUML Uml Toolkit.
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
// home   : http://sourceforge.net/projects/douml
//
// *************************************************************************
#include <stdio.h>
#include <q3popupmenu.h>
#include <QEvent>
#include <QKeyEvent>

#include "KeyValueTable.h"
#include "HaveKeyValueData.h"
#include "KeyValueData.h"
#include "MLinesItem.h"
#include "ComboItem.h"
#include "ProfiledStereotypes.h"
#include "strutil.h"
#include "DialogUtil.h"
#include "translate.h"

KeyValuesTable::KeyValuesTable(HaveKeyValueData * kvData, QWidget * parent, bool isReadOnly)
    : StringTable(((kvData == 0) ? 0 : kvData->get_n_keys()) + ((isReadOnly) ? 0 : 1),
                  (isReadOnly) ? 2 : 3, parent, isReadOnly)
{
    colNames.insert(0, TR("Key"));
    colNames.insert(1, TR("Value"));
    colNames.insert(2, TR("Do"));
    Init(kvData, isReadOnly);
}

void KeyValuesTable::init_row(int index)
{
    setText(index, 0, QString());
    setItem(index, 1, new MLinesItem(this, QString()));
    setText(index, 2, QString());
    setRowStretchable(index, TRUE);
}

bool KeyValuesTable::eventFilter(QObject *o, QEvent * e)
{
    if(e->type() == QEvent::KeyPress)
    {
        return false;
    }
   return  Q3Table::eventFilter(o,e);
}

bool KeyValuesTable::check_unique()
{
    forceUpdateCells();

    unsigned n = numRows();

    if (n != 0) {
        unsigned index;

        if (text(n - 1, 0).isEmpty())
            n -= 1;

        QStringList l;

        for (index = 0; index != n; index += 1) {
            const QString & s = text(index, 0);

            if (l.findIndex(s) != -1) {
                msg_critical(TR("Error"), TR("key '%1' used several times", s));
                return FALSE;
            }
            else
                l.append(s);
        }
    }

    return TRUE;
}

void KeyValuesTable::updateNodeFromThis(HaveKeyValueData * oper)
{
    forceUpdateCells();

    unsigned n = numRows();
    unsigned index;

    if (text(n - 1, 0).isEmpty())
        n -= 1;

    oper->set_n_keys(n);

    for (index = 0; index != n; index += 1) {
        oper->set_key(index, fromUnicode(text(index, 0)));
        oper->set_value(index, fromUnicode(text(index, 1)));
    }
}

void KeyValuesTable::updateThisFromNode(HaveKeyValueData *kvData, bool isReadOnly)
{
    Init(kvData, isReadOnly);
}

bool KeyValuesTable::get_value(const char * key, QString & value)
{
    QString ks = toUnicode(key);
    int index;

    for (index = 0; index != numRows(); index += 1) {
        if (text(index, 0) == ks) {
            value = text(index, 1);
            return TRUE;
        }
    }

    return FALSE;
}

void KeyValuesTable::remove(const char * key)
{
    QString ks = toUnicode(key);
    int index;

    for (index = 0; index != numRows(); index += 1) {
        if (text(index, 0) == ks) {
            delete_row(index);
            break;
        }
    }
}

void KeyValuesTable::Init(HaveKeyValueData *kvData, bool isReadOnly)
{
    AdjustColumnCount(isReadOnly);
    AdjustRowCount(kvData, isReadOnly);
    SetupTableText(kvData, isReadOnly);
    AdjustEditability(isReadOnly);
}

void KeyValuesTable::SetupTableText(HaveKeyValueData *kvData,  bool isReadOnly)
{
    QStringList items;
    unsigned index = 0;
    for (index = 0; index < kvData->get_n_keys(); index++)
    {
        QString k = toUnicode(kvData->get_key(index));
        QString v = toUnicode(kvData->get_value(index));

        TableItem * tiKey = nullptr;
        tiKey =  new MLinesItem(this, k, !isReadOnly);

        setItem(index, 0, tiKey);

        if (props.contains(k))
            setItem(index, 1, new ComboItem(this, v, props[k], FALSE));

        if ((k.contains(':') == 2) &&
                ProfiledStereotypes::enumerated(k, items))
        {
            props.insert(k, items);
            setItem(index, 1, new ComboItem(this, v, props[k], FALSE));
        }
        else
            setItem(index, 1, new MLinesItem(this, v, !isReadOnly));

        setItem(index, 2, new TableItem(this, Q3TableItem::Never,""));
        setRowStretchable(index, true);

        int crCount = v.count('\n');
        if (crCount != 0)
            setRowHeight(index, rowHeight(index) * (crCount + 1));


    }

    setItem(index, 0, new MLinesItem(this, QString()));
    setItem(index, 1, new MLinesItem(this, QString()));
    if(!isReadOnly)
        setItem(index, 2, new TableItem(this, Q3TableItem::Never,""));
    setRowStretchable(index, TRUE);
}

void KeyValuesTable::AdjustColumnCount(bool isReadOnly)
{
    int columnCount = isReadOnly ? 2 : 3;
    int currentCount = numCols();

    for(int current = currentCount - 1; current  > -1; current--)
        removeColumn(0);

    for(int current(0); current < columnCount ; current++)
    {
        insertColumns(current);
        horizontalHeader()->setLabel(current, colNames[current]);
    }
    setColumnStretchable(1, true);
    if(!isReadOnly)
    {
        setColumnStretchable(2, false);
        adjustColumn(2);
    }
    horizontalHeader()->setResizeEnabled(true, 0);
    horizontalHeader()->setResizeEnabled(true, 1);

}

void KeyValuesTable::AdjustRowCount(HaveKeyValueData * kvData, bool)
{
    int rowCount = numRows();
    int index ;
    for (index = rowCount - 1; index > -1; index --)
        removeRow(0);
    for(unsigned i(0); i < kvData->get_n_keys() + 1; i++)
    {
        insertRows(i);
        setRowStretchable(i, true);
    }
}

void KeyValuesTable::AdjustEditability(bool isReadOnly)
{
    int rowCount = numRows();
    for (int i(0); i < rowCount; i++)
    {
        item(i, 0)->setReplaceable(!isReadOnly);
        item(i, 1)->setReplaceable(!isReadOnly);
        if(!isReadOnly)
            item(i, 2)->setReplaceable(false);
        setRowReadOnly(i, isReadOnly);

    }
}
