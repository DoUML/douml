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





#include "UmlCanvas.h"
#include "UmlGlobal.h"
#include "DiagramView.h"
#include "CodDirsCanvas.h"
#include "ArrowCanvas.h"
#include "ArrowPointCanvas.h"
#include "BrowserDiagram.h"

static QList<UmlCanvas *> All;

static const struct {
    int w;
    int h;
} formatSize[] = {
    {841 * 4, 1189 * 4},	// IsoA0
    {594 * 4, 841 * 4},	// IsoA1
    {420 * 4, 594 * 4},	// IsoA2
    {297 * 4, 420 * 4},	// IsoA3
    {210 * 4, 297 * 4},	// IsoA4
    {148 * 4, 210 * 4},	// IsoA5
    {216 * 4, 279 * 4},	// UsA
    {279 * 4, 432 * 4},	// UsB
    {432 * 4, 559 * 4},	// UsC
    {559 * 4, 864 * 4},	// UsD
    {864 * 4, 1118 * 4},	// UsE
    {216 * 4, 279 * 4},	// Us letter
    {216 * 4, 356 * 4},	// Us legal
    {279 * 4, 432 * 4},	// Us tabloid

    {1189 * 4, 841 * 4},	// landscape IsoA0
    {841 * 4, 594 * 4},	// landscape IsoA1
    {594 * 4, 420 * 4},	// landscape IsoA2
    {420 * 4, 297 * 4},	// landscape IsoA3
    {297 * 4, 210 * 4},	// landscape IsoA4
    {210 * 4, 148 * 4},	// landscape IsoA5
    {279 * 4, 216 * 4},	// landscape UsA
    {432 * 4, 279 * 4},	// landscape UsB
    {559 * 4, 432 * 4},	// landscape UsC
    {864 * 4, 559 * 4},	// landscape UsD
    {1118 * 4, 864 * 4},	// landscape UsE
    {279 * 4, 216 * 4},	// landscape Us letter
    {356 * 4, 216 * 4},	// landscape Us legal
    {432 * 4, 279 * 4},	// Us ledger = lanscape Us tabloid
};

UmlCanvas::UmlCanvas(CanvasFormat f, BrowserDiagram * br_diag)
    : Q3Canvas(formatSize[f].w, formatSize[f].h),
      br_diagram(br_diag), view(0), all_items((const char *) 0)
{
    width100percent = width();
    height100percent = height();
    do_paste = FALSE;
    hlimit = 0;
    vlimit = 0;
    set_zoom(1);	// set do_scale
    do_scale = FALSE;
    br_diag->update_drawing_settings();
    show_shadow = br_diag->get_shadow();
    draw_all_relations = br_diag->get_draw_all_relations();

    All.append(this);
}

UmlCanvas::~UmlCanvas()
{
    All.remove(this);
}

void UmlCanvas::set_view(DiagramView * v)
{
    view = v;

    // add limits
    vlimit = new Q3CanvasLine(this);
    hlimit = new Q3CanvasLine(this);
    vlimit->setZ(TOP_Z);	// alien
    hlimit->setZ(TOP_Z);	// alien
    vlimit->setPen(::Qt::DotLine);
    hlimit->setPen(::Qt::DotLine);
    update_limits();
    vlimit->show();
    hlimit->show();
}

void UmlCanvas::update_limits()
{
    if (vlimit) {
        vlimit->setPoints(width() - 1, 0, width() - 1, height() - 1);
        hlimit->setPoints(0, height() - 1, width() - 1, height() - 1);
    }
}

void UmlCanvas::resize(CanvasFormat f)
{
    Q3Canvas::resize((int)(formatSize[f].w * zoom_value),
                     (int)(formatSize[f].h * zoom_value));
    width100percent = formatSize[f].w;
    height100percent = formatSize[f].h;
    update_limits();

    // force redisplay else phantoms may appears
    setBackgroundColor(::Qt::lightGray);
    update();
    setBackgroundColor(::Qt::white);
    update();
}

// to know if ths element is covered by arrow bounding
static bool small_element(const QRect & r)
{
    int l = ARROW_LENGTH * 3;

    return (r.width() < l) && (r.height() < l);
}

Q3CanvasItem * UmlCanvas::collision(const QPoint & p) const
{
    Q3CanvasItemList l = collisions(p);
    Q3CanvasItemList::ConstIterator it;
    Q3CanvasItemList::ConstIterator end = l.end();
    ArrowCanvas * arrow = 0;

    for (it = l.begin(); it != end; ++it)
        if (((*it)->visible()) && // at least not deleted
            !isa_alien(*it) &&
            !isa_col_msg_dirs(*it)) {
            switch ((*it)->rtti()) {
            case RTTI_ARROW:
                if (arrow == 0)
                    arrow = (ArrowCanvas *) *it;

                break;

            case RTTI_LABEL:
                return (arrow == 0) ? *it : arrow;

            default:
                // isa DiagramCanvas
                return ((arrow == 0) ||
                        (small_element(((DiagramCanvas *) *it)->rect()) &&
                         ((DiagramCanvas *) *it)->attached_to(arrow)))
                       ? *it : arrow;
            }
        }

    return arrow;
}

Q3CanvasItem * UmlCanvas::collision(const QPoint & p, int except) const
{
    Q3CanvasItemList l = collisions(p);
    Q3CanvasItemList::ConstIterator it;
    Q3CanvasItemList::ConstIterator end = l.end();
    ArrowCanvas * arrow = 0;

    for (it = l.begin(); it != end; ++it)
        if (((*it)->visible()) && // at least not deleted
            !isa_alien(*it) &&
            !isa_col_msg_dirs(*it)) {
            int k = (*it)->rtti();

            if (k != except) {
                switch (k) {
                case RTTI_ARROW:
                    if (arrow == 0)
                        arrow = (ArrowCanvas *) *it;

                    break;

                case RTTI_LABEL:
                    return (arrow == 0) ? *it : arrow;

                default:
                    // isa DiagramCanvas
                    return ((arrow == 0) ||
                            (small_element(((DiagramCanvas *) *it)->rect()) &&
                             ((DiagramCanvas *) *it)->attached_to(arrow)))
                           ? *it : arrow;
                }
            }
        }

    return arrow;
}

void UmlCanvas::del(Q3CanvasItem * i)
{
    // do not delete, just hide because of a Qt's bug (?)
    i->hide();
    selected.remove(i);

    DiagramItem * it = QCanvasItemToDiagramItem(i);

    if (it != 0)
        all_items.remove(it->get_ident());
}

void UmlCanvas::select(Q3CanvasItem * i)
{
    i->setSelected(TRUE);
    selected.append(i);
}

void UmlCanvas::unselect(Q3CanvasItem * i)
{
    i->setSelected(FALSE);
    selected.remove(i);
}

void UmlCanvas::unselect_all()
{
    Q3CanvasItemList::Iterator it;

    for (it = selected.begin(); it != selected.end(); ++it)
        (*it)->setSelected(FALSE);

    selected.clear();
}

void UmlCanvas::set_zoom(double zm)
{
    do_scale = TRUE;
    zoom_value = zm;

    float ps = (float)(NormalFont.pointSize() * zm);
    float ps_small = 2 * ps / 3;
    float ps_large = 2 * ps;

    the_fonts[UmlNormalFont] = NormalFont;
    the_fonts[UmlNormalFont].setPointSizeFloat(ps);
    the_fonts[UmlNormalBoldFont] = BoldFont;
    the_fonts[UmlNormalBoldFont].setPointSizeFloat(ps);
    the_fonts[UmlNormalItalicFont] = ItalicFont;
    the_fonts[UmlNormalItalicFont].setPointSizeFloat(ps);
    the_fonts[UmlNormalBoldItalicFont] = BoldItalicFont;
    the_fonts[UmlNormalBoldItalicFont].setPointSizeFloat(ps);
    the_fonts[UmlNormalUnderlinedFont] = UnderlineFont;
    the_fonts[UmlNormalUnderlinedFont].setPointSizeFloat(ps);
    the_fonts[UmlNormalStrikeOutFont] = StrikeOutFont;
    the_fonts[UmlNormalStrikeOutFont].setPointSizeFloat(ps);

    the_fonts[UmlSmallFont] = NormalFont;
    the_fonts[UmlSmallFont].setPointSizeFloat(ps_small);
    the_fonts[UmlSmallBoldFont] = BoldFont;
    the_fonts[UmlSmallBoldFont].setPointSizeFloat(ps_small);
    the_fonts[UmlSmallItalicFont] = ItalicFont;
    the_fonts[UmlSmallItalicFont].setPointSizeFloat(ps_small);
    the_fonts[UmlSmallBoldItalicFont] = BoldItalicFont;
    the_fonts[UmlSmallBoldItalicFont].setPointSizeFloat(ps_small);

    the_fonts[UmlLargeFont] = NormalFont;
    the_fonts[UmlLargeFont].setPointSizeFloat(ps_large);
    the_fonts[UmlLargeBoldFont] = BoldFont;
    the_fonts[UmlLargeBoldFont].setPointSizeFloat(ps_large);
    the_fonts[UmlLargeItalicFont] = ItalicFont;
    the_fonts[UmlLargeItalicFont].setPointSizeFloat(ps_large);
    the_fonts[UmlLargeBoldItalicFont] = BoldItalicFont;
    the_fonts[UmlLargeBoldItalicFont].setPointSizeFloat(ps_large);

    Q3Canvas::resize((int)(width100percent * zm),
                     (int)(height100percent * zm));

    update_limits();
}

void UmlCanvas::show_limits(bool y)
{
    vlimit->setVisible(y);
    hlimit->setVisible(y);
}

void UmlCanvas::update_global_settings()
{
    foreach (UmlCanvas *c, All) {
        c->br_diagram->update_drawing_settings();
        c->show_shadow = c->br_diagram->get_shadow();
        c->draw_all_relations = c->br_diagram->get_draw_all_relations();
    }
}

int UmlCanvas::shadow() const
{
    if (! show_shadow)
        return 0;

    int w = (int)(4 * zoom_value);

    if (w < 3)
        return 3;
    else if (w > 6)
        return 6;
    else
        return w;
}

void UmlCanvas::update()
{
    if (view != 0)
        view->update_history();

    Q3Canvas::update();
}

bool UmlCanvas::already_drawn(BrowserNode * bn)
{
    Q3CanvasItemList all = allItems();
    Q3CanvasItemList::Iterator cit;
    UmlCode k = bn->get_type();

    for (cit = all.begin(); cit != all.end(); ++cit) {
        if ((*cit)->visible()) {
            DiagramItem * di = QCanvasItemToDiagramItem(*cit);

            if ((di != 0) && (di->type() == k) && (di->get_bn() == bn))
                return TRUE;
        }
    }

    return FALSE;
}


void UmlCanvas::dont_draw_all_relations()
{
    draw_all_relations = FALSE;
    browser_diagram()->dont_draw_all_relations();
}
