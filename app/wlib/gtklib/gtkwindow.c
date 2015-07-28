/** \file gtkwindow.c
 * Basic window handling stuff.
 *
 * $Header: /home/dmarkle/xtrkcad-fork-cvs/xtrkcad/app/wlib/gtklib/gtkwindow.c,v 1.12 2010-04-28 04:04:38 dspagnol Exp $
 */

/*  XTrkCad - Model Railroad CAD
 *  Copyright (C) 2005 Dave Bullis
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

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include "gtkint.h"

wWin_p gtkMainW;

#define FOUR	(4)
#define MENUH	(24)

#define MIN_WIN_WIDTH (50)
#define MIN_WIN_HEIGHT (50)

#define SECTIONWINDOWSIZE  "gtklib window size"
#define SECTIONWINDOWPOS   "gtklib window pos"

extern wBool_t listHelpStrings;
		
static wControl_p firstWin = NULL, lastWin;
static int keyState;
static wBool_t gtkBlockEnabled = TRUE;

/*
 *****************************************************************************
 *
 * Window Utilities
 *
 *****************************************************************************
 */

/**
 * Get the window size from the resource (.rc) file.  The size is saved under the key 
 * SECTIONWINDOWSIZE.window name
 *
 * \param win IN window
 * \param nameStr IN window name
 */

static void getWinSize( wWin_p win, const char * nameStr )
{
	int w, h;
	const char *cp;
    char *cp1, *cp2;

	if ( (win->option&F_RESIZE) &&
		 (win->option&F_RECALLPOS) &&
		 (cp = wPrefGetString( SECTIONWINDOWSIZE, nameStr)) &&
		 (w = strtod( cp, &cp1 ), cp != cp1) &&
		 (h = strtod( cp1, &cp2 ), cp1 != cp2) ) {
		if (w < 10)
			w = 10;
		if (h < 10)
			h = 10;
		win->w = win->origX = w;
		win->h = win->origY = h;
		win->option &= ~F_AUTOSIZE;
	}
}

/**
 * Save the window size to the resource (.rc) file.  The size is saved under the key 
 * SECTIONWINDOWSIZE.window name
 *
 * \param win IN window
 */

static void saveSize( wWin_p win )
{
	char pos_s[20];
	if ( (win->option&F_RESIZE) &&
		 (win->option&F_RECALLPOS) &&
		 GTK_WIDGET_VISIBLE( GTK_WIDGET(win->gtkwin) ) ) {
		sprintf( pos_s, "%d %d", win->w, win->h-(FOUR + ((win->option&F_MENUBAR)?MENUH:0) ) );
		wPrefSetString( SECTIONWINDOWSIZE, win->nameStr, pos_s );
	}
}

/**
 * Get the window position from the resource (.rc) file.  The position is saved under the key 
 * SECTIONWINDOWPOS.window name
 *
 * \param win IN window
 */

static void getPos( wWin_p win )
{
	int x, y;
	const char *cp;
    char *cp1, *cp2;
	wPos_t gtkDisplayWidth = gdk_screen_width();
	wPos_t gtkDisplayHeight = gdk_screen_height();

	if ( (win->option&F_RECALLPOS) &&
			(!win->shown) ) {
		if ((cp = wPrefGetString( SECTIONWINDOWPOS, win->nameStr))) {
			x = strtod( cp, &cp1 );
			if (cp == cp1)
				return;
			y = strtod( cp1, &cp2 );
			if (cp2 == cp1)
				return;
			if ( y > gtkDisplayHeight-win->h )
				y = gtkDisplayHeight-win->h;
			if ( x > gtkDisplayWidth-win->w )
				x = gtkDisplayWidth-win->w;
			if ( x <= 0 )
				x = 1;
			if ( y <= 0 )
				y = 1;
			gtk_window_move( GTK_WINDOW(win->gtkwin), x, y );
			gtk_window_resize( GTK_WINDOW(win->gtkwin), win->w, win->h );
		}
	}
}

/**
 * Save the window position to the resource (.rc) file.  The position is saved under the key 
 * SECTIONWINDOWPOS.window name
 *
 * \param win IN window
 */

static void savePos( wWin_p win )
{
	int x, y;
	char pos_s[20];
	if ( (win->option&F_RECALLPOS) ) {
		gdk_window_get_position( GTK_WIDGET(win->gtkwin)->window, &x, &y );
		x -= 5;
		y -= 25;
		sprintf( pos_s, "%d %d", x, y );
		wPrefSetString( SECTIONWINDOWPOS, win->nameStr, pos_s );
	}
}

/**
 * Returns the dimensions of <win>.
 *
 * \param win IN window handle
 * \param width OUT width of window
 * \param height OUT height of window minus menu bar size
 */

EXPORT void wWinGetSize(
		wWin_p win,		/* Window */
		wPos_t * width,		/* Returned window width */
		wPos_t * height )	/* Returned window height */
{
	GtkRequisition requisition;
	wPos_t w, h;
	gtk_widget_size_request( win->gtkwin, &requisition );
	w = win->w;
	h = win->h;
	if ( win->option&F_AUTOSIZE ) {
		if ( win->realX > w )
			w = win->realX;
		if ( win->realY > h )
			h = win->realY;
	}
	*width = w;
	*height = h - FOUR - ((win->option&F_MENUBAR)?MENUH:0);
}

/**
 * Sets the dimensions of <w> to <widht> and <height>.
 *
 * \param win IN window
 * \param width IN new width
 * \param height IN new height
 */

EXPORT void wWinSetSize(
		wWin_p win,		/* Window */
		wPos_t width,		/* Window width */
		wPos_t height )		/* Window height */
{
	win->busy = TRUE;
	win->w = width;
	win->h = height + FOUR + ((win->option&F_MENUBAR)?MENUH:0);
	gtk_widget_set_size_request( win->gtkwin, win->w, win->h );
	gtk_widget_set_size_request( win->widget, win->w, win->h ); 
	win->busy = FALSE;

}

/**
 * Shows or hides window <win>. If <win> is created with 'F_BLOCK' option then the applications other
 * windows are disabled and 'wWinShow' doesnot return until the window <win> is closed by calling 
 * 'wWinShow(<win>,FALSE)'.
 *
 * \param win IN window
 * \param show IN visibility state
 */

EXPORT void wWinShow(
		wWin_p win,		/* Window */
		wBool_t show )		/* Command */
{
	GtkRequisition requisition;
	if (debugWindow >= 2) printf("Set Show %s\n", win->labelStr?win->labelStr:"No label" );
	if (win->widget == 0) abort();
	if (show) {
		keyState = 0;
		getPos( win );
		if ( win->option & F_AUTOSIZE ) {
			gtk_widget_size_request( win->gtkwin, &requisition );
			if ( requisition.width != win->w || requisition.height != win->h ) {
				gtk_widget_set_size_request( win->gtkwin, win->w, win->h );
				gtk_widget_set_size_request( win->widget, win->w, win->h );
				if (win->option&F_MENUBAR) {
					gtk_widget_set_size_request( win->menubar, win->w, MENUH );
				}
			}
		}
		if ( !win->shown ) {
			gtk_widget_show( win->gtkwin );
			gtk_widget_show( win->widget );
		}
		gdk_window_raise( win->gtkwin->window );
		if ( win->shown && win->modalLevel > 0 )
			gtk_widget_set_sensitive( GTK_WIDGET(win->gtkwin), TRUE );
		win->shown = show;
		win->modalLevel = 0;

		if ( (!gtkBlockEnabled) || (win->option & F_BLOCK) == 0) {
			wFlush();
		} else {
			gtkDoModal( win, TRUE );
		}
	} else {
		wFlush();
		saveSize( win );
		savePos( win );
		win->shown = show;
		if ( gtkBlockEnabled && (win->option & F_BLOCK) != 0) {
			gtkDoModal( win, FALSE );
		}
		gtk_widget_hide( win->gtkwin );
		gtk_widget_hide( win->widget );
	}
}

/**
 * Block windows against user interactions. Done during demo mode etc.
 *
 * \param enabled IN blocked if TRUE
 */

EXPORT void wWinBlockEnable(
		wBool_t enabled )
{
	gtkBlockEnabled = enabled;
}

/**
 * Returns whether the window is visible.
 *
 * \param win IN window
 * \return    TRUE if visible, FALSE otherwise
 */

EXPORT wBool_t wWinIsVisible(
		wWin_p win )
{
	return win->shown;
}

/**
 * Sets the title of <win> to <title>.
 *
 * \param varname1 IN window
 * \param varname2 IN new title
 */

EXPORT void wWinSetTitle(
		wWin_p win,		/* Window */
		const char * title )		/* New title */
{
	gtk_window_set_title( GTK_WINDOW(win->gtkwin), title );
}

/**
 * Sets the window <win> to busy or not busy. Sets the cursor accordingly
 *
 * \param varname1 IN window
 * \param varname2 IN TRUE if busy, FALSE otherwise
 */

EXPORT void wWinSetBusy(
		wWin_p win,		/* Window */
		wBool_t busy )		/* Command */
{
	GdkCursor * cursor;
	if (win->gtkwin == 0) abort();
	if ( busy )
		cursor = gdk_cursor_new ( GDK_WATCH );
	else
		cursor = NULL;
	gdk_window_set_cursor (win->gtkwin->window, cursor);
	if ( cursor )
		gdk_cursor_destroy (cursor);   
	gtk_widget_set_sensitive( GTK_WIDGET(win->gtkwin), busy==0 );
}


EXPORT void gtkDoModal(
		wWin_p win0,
		wBool_t modal )
{
	wWin_p win;
	for ( win=(wWin_p)firstWin; win; win=(wWin_p)win->next ) {
		if ( win->shown && win != win0 ) {
			if ( modal ) {
				if ( win->modalLevel == 0 )
					gtk_widget_set_sensitive( GTK_WIDGET(win->gtkwin), FALSE );
				win->modalLevel++;
			} else {
				if ( win->modalLevel > 0 ) {
					win->modalLevel--;
					if ( win->modalLevel == 0 )
						gtk_widget_set_sensitive( GTK_WIDGET(win->gtkwin), TRUE );
				}
			}
			if ( win->modalLevel < 0 ) {
				fprintf( stderr, "DoModal: %s modalLevel < 0", win->nameStr?win->nameStr:"<NULL>" );
				abort();
			}
		}
	}
	if ( modal ) {
		gtk_main();
	} else {
		gtk_main_quit();
	}
}

/**
 * Returns the Title of <win>.
 *
 * \param win IN window
 * \return    pointer to window title
 */

EXPORT const char * wWinGetTitle(
	 wWin_p win )			/* Window */
{
	return win->labelStr;
}


EXPORT void wWinClear(
		wWin_p win,
		wPos_t x,
		wPos_t y,
		wPos_t width,
		wPos_t height )
{
}


EXPORT void wWinDoCancel(
		wWin_p win )
{
	wControl_p b;
	for (b=win->first; b; b=b->next) {
		if ((b->type == B_BUTTON) && (b->option & BB_CANCEL) ) {
			gtkButtonDoAction( (wButton_p)b );
		}
	}
}

/*
 ******************************************************************************
 *
 * Call Backs
 *
 ******************************************************************************
 */

static gint window_delete_event(
		GtkWidget *widget,
		GdkEvent *event,
		wWin_p win )
{
	wControl_p b;
	/* if you return FALSE in the "delete_event" signal handler,
	 * GTK will emit the "destroy" signal.  Returning TRUE means
	 * you don't want the window to be destroyed.
	 * This is useful for popping up 'are you sure you want to quit ?'
	 * type dialogs. */

	/* Change TRUE to FALSE and the main window will be destroyed with
	 * a "delete_event". */

	for ( b = win->first; b; b=b->next )
		if (b->doneProc)
			b->doneProc( b );
	if (win->winProc) {
		win->winProc( win, wClose_e, win->data );
		if (win != gtkMainW)
			wWinShow( win, FALSE );
	}
	return (TRUE);
}

static int window_redraw(
		wWin_p win,
		wBool_t doWinProc )
{
	wControl_p b;

	if (win==NULL)
		return FALSE;

	for (b=win->first; b != NULL; b = b->next) {
		if (b->repaintProc)
			b->repaintProc( b );
	}

	return FALSE;
}

static int fixed_expose_event(
		GtkWidget * widget,
		GdkEventExpose * event,
		wWin_p win )
{
	if (event->count==0)
		return window_redraw( win, TRUE );
	else
		return FALSE;
}

static int window_configure_event(
		GtkWidget * widget,
		GdkEventConfigure * event,
		wWin_p win )
{
	wPos_t h;

	if (win==NULL)
		return FALSE;

	h = event->height - FOUR;
	if (win->option&F_MENUBAR)
		h -= MENUH;
	if (win->option&F_RESIZE) {
		if ( event->width < 10 || event->height < 10 )
			return TRUE;
		if (win->w != event->width || win->h != event->height) {
			win->w = event->width;
			win->h = event->height;
			if ( win->w < MIN_WIN_WIDTH )
				win->w = MIN_WIN_WIDTH;
			if ( win->h < MIN_WIN_HEIGHT )
				win->h = MIN_WIN_HEIGHT;
			if (win->option&F_MENUBAR)
				gtk_widget_set_size_request( win->menubar, win->w, MENUH ); 

			if (win->busy==FALSE && win->winProc) {
				win->winProc( win, wResize_e, win->data );
			}
		}
	} 
	return FALSE;
}

/**
 * Get current state of shift, ctrl or alt keys.
 *
 * \return    or'ed value of WKEY_SHIFT, WKEY_CTRL and WKEY_ALT depending on state
 */

int wGetKeyState( void )
{
	return keyState;
}

wBool_t catch_shift_ctrl_alt_keys(
		GtkWidget * widget,
		GdkEventKey *event,
		void * data )
{
	int state;

	state = 0;
	switch (event->keyval) {
	case GDK_Shift_L:
	case GDK_Shift_R:
		state |= WKEY_SHIFT;
		break;
	case GDK_Control_L:
	case GDK_Control_R:
		state |= WKEY_CTRL;
		break;
	case GDK_Alt_L:
	case GDK_Alt_R:
		state |= WKEY_ALT;
		break;
	}
	if ( state != 0 ) {
		if (event->type == GDK_KEY_PRESS)
			keyState |= state;
		else
			keyState &= ~state;
		return TRUE;
	}
	return FALSE;
}
		
static gint window_char_event(
		GtkWidget * widget,
		GdkEventKey *event,
		wWin_p win )
{
	wControl_p bb;
	if ( catch_shift_ctrl_alt_keys( widget, event, win ) )
		return FALSE;
	if (event->type == GDK_KEY_RELEASE)
		return FALSE;

	if ( event->state == 0 ) {
		if ( event->keyval == GDK_Escape ) {
			for ( bb=win->first; bb; bb=bb->next ) {
				if ( bb->type == B_BUTTON && (bb->option&BB_CANCEL) ) {
					gtkButtonDoAction( (wButton_p)bb );
					return TRUE;
				}
			}
		}
	}
	if ( gtkHandleAccelKey( event ) ) {
		return TRUE;
	} else {
		return FALSE;
	}
}


/*
 *******************************************************************************
 *
 * Main and Popup Windows
 *
 *******************************************************************************
 */



static wWin_p wWinCommonCreate(
		wWin_p parent,
		int winType,
		wPos_t x,
		wPos_t y,
		const char * labelStr,
		const char * nameStr,
		long option,
		wWinCallBack_p winProc,
		void * data )
{
	wWin_p w;
	int h;

	w = gtkAlloc( NULL, winType, x, y, labelStr, sizeof *w, data );
	w->busy = TRUE;
	w->option = option;
	getWinSize( w, nameStr );

	h = FOUR;
	if (w->option&F_MENUBAR)
		h += MENUH;

	if ( winType == W_MAIN ) {
		w->gtkwin = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	} else {
		w->gtkwin = gtk_window_new( GTK_WINDOW_TOPLEVEL );
		if ( gtkMainW )
			gtk_window_set_transient_for( GTK_WINDOW(w->gtkwin), GTK_WINDOW(gtkMainW->gtkwin) );
	}
	
	if( option & F_HIDE )
		gtk_widget_hide( w->gtkwin );

	/* center window on top of parent window */
	if( option & F_CENTER )
		gtk_window_set_position(GTK_WINDOW(w->gtkwin), GTK_WIN_POS_CENTER_ON_PARENT );
				
	w->widget = gtk_fixed_new();
	if (w->widget == 0) abort();

	if (w->option&F_MENUBAR) {
		w->menubar = gtk_menu_bar_new();
		gtk_container_add( GTK_CONTAINER(w->widget), w->menubar );
		gtk_widget_show( w->menubar );
		gtk_widget_set_size_request( w->menubar, -1, MENUH );
	}

	gtk_container_add( GTK_CONTAINER(w->gtkwin), w->widget );

	if (w->option&F_AUTOSIZE) {
		w->realX = 0;
		w->w = 0;
		w->realY = h;
		w->h = 0;
	} else {
		w->w = w->realX = w->origX;
		w->h = w->realY = w->origY+h;
		gtk_widget_set_size_request( w->gtkwin, w->w, w->h );
		gtk_widget_set_size_request( w->widget, w->w, w->h );
		if (w->option&F_MENUBAR) {
			gtk_widget_set_size_request( w->menubar, w->w, MENUH );
		}
	}

	w->first = w->last = NULL;

	w->winProc = winProc;

	gtk_signal_connect (GTK_OBJECT (w->gtkwin), "delete_event",
						GTK_SIGNAL_FUNC (window_delete_event), w);
	gtk_signal_connect (GTK_OBJECT (w->widget), "expose_event",
						GTK_SIGNAL_FUNC (fixed_expose_event), w);
	gtk_signal_connect (GTK_OBJECT (w->gtkwin), "configure_event",
						GTK_SIGNAL_FUNC (window_configure_event), w); 
	gtk_signal_connect (GTK_OBJECT (w->gtkwin), "key_press_event",
						GTK_SIGNAL_FUNC (window_char_event), w);
	gtk_signal_connect (GTK_OBJECT (w->gtkwin), "key_release_event",
						GTK_SIGNAL_FUNC (window_char_event), w);
	gtk_widget_set_events (w->widget, GDK_EXPOSURE_MASK );
	gtk_widget_set_events ( GTK_WIDGET(w->gtkwin), GDK_EXPOSURE_MASK|GDK_KEY_PRESS_MASK|GDK_KEY_RELEASE_MASK );

	/**
     * \todo { set_policy is deprecated and should be replaced by set_resizable. In order to do that
     * the library has to be cleared up from calls to set_size_request as these set the minimum widget size 
     * to the current size preventing the user from re-sizing the window to a smaller size. At least this
     * is my current assumption ;-) }
     */
	if (w->option & F_RESIZE) {
		gtk_window_set_policy( GTK_WINDOW(w->gtkwin), TRUE, TRUE, TRUE );
//		gtk_window_set_resizable( GTK_WINDOW(w->gtkwin), TRUE );
	} else {
//		gtk_window_set_resizable( GTK_WINDOW(w->gtkwin), FALSE );
		gtk_window_set_policy( GTK_WINDOW(w->gtkwin), FALSE, FALSE, TRUE );
	}
	
	w->lastX = 0;
	w->lastY = h;
	w->shown = FALSE;
	w->nameStr = nameStr?strdup( nameStr ):NULL;
	if (labelStr)
		gtk_window_set_title( GTK_WINDOW(w->gtkwin), labelStr );
	if (listHelpStrings)
		printf( "WINDOW - %s\n", nameStr?nameStr:"<NULL>" );

	if (firstWin) {
		lastWin->next = (wControl_p)w;
	} else {
		firstWin = (wControl_p)w;
	}
	lastWin = (wControl_p)w;
	gtk_widget_show( w->widget );
	gtk_widget_realize( w->gtkwin );

	w->busy = FALSE;
	return w;
}

/**
 * \todo { investigate and implement this function for setting the correct icon as necessary.
 * It looks like these functions are never called at the moment. }
 */

EXPORT void wWinSetBigIcon(
		wWin_p win,		/* Main window */
		wIcon_p ip )		/* The icon */
/*
Create an Icon from a X-bitmap.
*/
{
#ifdef LATER
	GdkPixmap * pixmap;
	GdkBitmap * mask;
	pixmap = gtkMakeIcon( win->gtkwin, ip, &mask );
	gdk_window_set_icon( win->gtkwin->window, NULL, pixmap, mask );
	gdk_pixmap_unref( pixmap );
	gdk_bitmap_unref( mask );
#endif
}


/**
 * \todo { investigate and implement this function for setting the correct icon as necessary.
 * It looks like these functions are never called at the moment. }
 */


EXPORT void wWinSetSmallIcon(
		wWin_p win,		/* Main window */
		wIcon_p ip )		/* The icon */
/*
Create an Icon from a X-bitmap.
*/
{
	GdkBitmap * mask;
	if ( ip->gtkIconType == gtkIcon_bitmap ) {
		mask = gdk_bitmap_create_from_data( win->gtkwin->window, ip->bits, ip->w, ip->h );
		gdk_window_set_icon( win->gtkwin->window, NULL, mask, mask );
		/*gdk_bitmap_unref( mask );*/
	}
}

/**
 * Initialize the application's main window. This function does the necessary initialization 
 * of the application including creation of the main window.
 *
 * \param name IN internal name of the application. Used for filenames etc. 
 * \param x    IN Initial window width
 * \param y    IN Initial window height
 * \param helpStr IN Help topic string
 * \param labelStr IN window title
 * \param nameStr IN Window name
 * \param option IN options for window creation
 * \param winProc IN pointer to main window procedure
 * \param data IN User context
 * \return    window handle or NULL on error
 */

EXPORT wWin_p wWinMainCreate(
		const char * name,		/* Application name */
		wPos_t x,				/* Initial window width */
		wPos_t y,				/* Initial window height */
		const char * helpStr,	/* Help topic string */
		const char * labelStr,	/* Window title */
		const char * nameStr,	/* Window name */
		long option,			/* Options */
		wWinCallBack_p winProc,	/* Call back function */
		void * data )			/* User context */
{
	char *pos;

	if( pos = strchr( name, ';' )) {
		/* if found, split application name and configuration name */
		strcpy( wConfigName, pos + 1 );
	} else {
		/* if not found, application name and configuration name are same */
		strcpy( wConfigName, name );
	}

	gtkMainW = wWinCommonCreate( NULL, W_MAIN, x, y, labelStr, nameStr, option, winProc, data );

	wDrawColorWhite = wDrawFindColor( 0xFFFFFF );
	wDrawColorBlack = wDrawFindColor( 0x000000 );

	gdk_window_set_group( gtkMainW->gtkwin->window, gtkMainW->gtkwin->window );
	return gtkMainW;
}

/**
 * Create a new popup window.
 *
 * \param parent IN Parent window (may be NULL)
 * \param x IN Initial window width
 * \param y IN Initial window height
 * \param helpStr IN Help topic string
 * \param labelStr IN Window title
 * \param nameStr IN Window name
 * \param option IN Options
 * \param winProc IN call back function
 * \param data IN User context information
 * \return    handle for new window
 */

EXPORT wWin_p wWinPopupCreate(
		wWin_p parent,		
		wPos_t x,		
		wPos_t y,		
		const char * helpStr,
		const char * labelStr,
		const char * nameStr,
		long option,	
		wWinCallBack_p winProc,
		void * data )		
{
	wWin_p win;

	if (parent == NULL) {
		if (gtkMainW == NULL)
			abort();
		parent = gtkMainW;
	}
	win = wWinCommonCreate( parent, W_POPUP, x, y, labelStr, nameStr, option, winProc, data );
	gdk_window_set_group( win->gtkwin->window, gtkMainW->gtkwin->window );

	return win;
}


/**
 * Terminates the applicaton with code <rc>. Before closing the main window
 * call back is called with wQuit_e. The program is terminated without exiting
 * the main message loop.
 *
 * \param rc IN exit code
 * \return    never returns
 */


EXPORT void wExit(
		int rc )		/* Application return code */
{
	wWin_p win;
	for ( win = (wWin_p)firstWin; win; win = (wWin_p)win->next ) {
		if ( GTK_WIDGET_VISIBLE( GTK_WIDGET(win->gtkwin) ) ) {
			saveSize( win );
			savePos( win );
		}
	}
	wPrefFlush();
	if (gtkMainW && gtkMainW->winProc != NULL)
		gtkMainW->winProc( gtkMainW, wQuit_e, gtkMainW->data );
	
	exit( rc );
}
