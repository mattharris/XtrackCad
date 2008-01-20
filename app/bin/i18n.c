/*  XTrkCad - Model Railroad CAD
 *  Copyright (C) 2007 Mikko Nissinen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "i18n.h"

#include <locale.h>
#include <stdio.h>

/*
 * Initialize gettext environment.
 */
void InitGettext( void )
{
#ifdef XTRKCAD_USE_GETTEXT
	setlocale(LC_ALL, "");
	bindtextdomain(XTRKCAD_PACKAGE, XTRKCAD_LOCALE_DIR);
	bind_textdomain_codeset(XTRKCAD_PACKAGE, "UTF-8");
	textdomain(XTRKCAD_PACKAGE);

#ifdef VERBOSE
	printf(_("Gettext initialized (PACKAGE=%s, LOCALEDIR=%s, LC_ALL=%s).\n"),
			XTRKCAD_PACKAGE, XTRKCAD_LOCALE_DIR, setlocale(LC_ALL, NULL));
#endif

#endif /* XTRKCAD_USE_GETTEXT */
}