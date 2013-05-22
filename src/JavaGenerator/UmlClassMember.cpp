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

#include <QTextStream>
//Added by qt3to4:
#include "misc/mystr.h"
#include <QTextStream>

#include "UmlClassMember.h"
#include "UmlCom.h"
#include "UmlTypeSpec.h"
//#include "JavaRefType.h"
#include "UmlSettings.h"
#include "JavaSettings.h"

void UmlClassMember::remove_comments(WrapperStr & s)
{
    int index1 = 0;

    if ((index1 = s.find("${comment}")) != -1)
        s.remove((unsigned) index1, 10);
    else if ((index1 = s.find("${description}")) != -1)
        s.remove((unsigned) index1, 14);

    while ((index1 = s.find('/', index1)) != -1) {
        int index2;

        switch (((const char *) s)[index1 + 1]) {
        case '/':
            if ((index2 = s.find('\n', index1 + 2)) != -1)
                s.remove(index1, index2 - index1 + 1);
            else
                s.truncate(index1);

            break;

        case '*':
            if ((index2 = s.find("*/", index1 + 2)) != -1)
                s.replace(index1, index2 - index1 + 1, " ");
            else
                s.truncate(index1);

            break;

        default:
            index1 += 1;
        }
    }
}

void UmlClassMember::remove_arrays(WrapperStr & s)
{
    int index1 = 0;

    while ((index1 = s.find('[', index1)) != -1) {
        int index2 = index1 = s.find(']', index1 + 1);

        if (index2 == -1) {
            s.truncate(index1);
            return;
        }
        else
            s.replace(index1, index2 - index1 + 1, " ");
    }
}

void UmlClassMember::generate_visibility(QTextStream & f, const char * parent_st)
{
    switch (visibility()) {
    case PublicVisibility:
        if ((parent()->kind() != aClass) ||
            (parent_st == 0) ||
            ((*parent_st != 'i') && (*parent_st != '@')))
            // not public by default
            f << "public ";

        break;

    case ProtectedVisibility:
        f << "protected ";
        break;

    case PrivateVisibility:
        f << "private ";
        break;

    default:	// package
        break;
    }
}

/*
bool UmlClassMember::compute_dependency(QList<JavaRefType *> & dependencies,
					WrapperStr decl, const UmlTypeSpec & t)
{
  remove_comments(decl);
  remove_arrays(decl);

  bool have_type = FALSE;
  const char * p = decl;
  const char * dontsubstituteuntil = 0;

  for (;;) {
    UmlTypeSpec ts;
    char c;
    bool dontsearchend = FALSE;

    // search word beginning
    while ((c = *p) != 0) {
      if ((c == '_') ||
	  ((c >= 'a') && (c <= 'z')) ||
	  ((c >= 'A') && (c <= 'Z')))
	break;
      else if (dontsubstituteuntil != 0) {
	if (p >= dontsubstituteuntil)
	  dontsubstituteuntil = 0;
	p += 1;
      }
      else if (c == '=')
	// init, all is done
	return have_type;
      else if (!strncmp(p, "${type}", 7)) {
	p += 7;
	ts = t;
	if (ts.type != 0) {
	  dontsearchend = TRUE;
	  break;
	}
	else {
	  decl = ts.explicit_type + p;
	  p = decl;
	}
      }
      else
	p += 1;
    }

    if (c == 0)
      return have_type;

    if (!dontsearchend) {
      // search word end
      const char * p2 = p;

      ts.explicit_type = p2;
      p += 1;

      while ((c = *p) != 0) {
	if ((c == '_') ||
	    (c == ':') ||
	    ((c >= 'a') && (c <= 'z')) ||
	    ((c >= 'A') && (c <= 'Z')) ||
	    ((c >= '0') && (c <= '9')))
	  p += 1;
	else {
	  ts.explicit_type.truncate(p - p2);
	  break;
	}
      }

      if ((p2 = strrchr(ts.explicit_type, ':')) != 0)
	// remove package name !!!
	ts.explicit_type = p2 + 1;

      if (dontsubstituteuntil == 0) {
	WrapperStr subst = JavaSettings::type(ts.explicit_type);

	if (subst != ts.explicit_type) {
	  decl = subst + ' ' + p;
	  p = decl;
	  dontsubstituteuntil = p + subst.length();
	  continue;
	}
      }
    }

    // check manually added keyword
    if ((ts.explicit_type == "const") ||
	(ts.explicit_type == "static"))
      continue;

    if (JavaRefType::add(ts, dependencies))
      have_type = TRUE;
  }

  return have_type;
}
*/
