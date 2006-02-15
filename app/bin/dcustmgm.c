/*
 * $Header: /home/dmarkle/xtrkcad-fork-cvs/xtrkcad/app/bin/dcustmgm.c,v 1.1 2005-12-07 15:46:56 rc-flyer Exp $
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

#include "track.h"
#include <errno.h>
#ifdef WINDOWS
#include <io.h>
#define F_OK	(0)
#define W_OK	(2)
#define access	_access
#endif

/*****************************************************************************
 *
 * Custom List Management
 *
 */

static void CustomEdit( void * action );
static void CustomDelete( void * action );
static void CustomExport( void * action );
static void CustomDone( void * action );
static wPos_t customListWidths[] = { 18, 100, 30, 80, 220 };
static const char * customListTitles[] = { "", "Manufacturer", "Scale", "Part No", "Description" };
static paramListData_t customListData = { 10, 400, 5, customListWidths, customListTitles };
static paramData_t customPLs[] = {
#define I_CUSTOMLIST	(0)
#define customSelL		((wList_p)customPLs[I_CUSTOMLIST].control)
	{	PD_LIST, NULL, "inx", PDO_DLGRESETMARGIN|PDO_DLGRESIZE, &customListData, NULL, BL_MANY },
#define I_CUSTOMEDIT	(1)
	{	PD_BUTTON, CustomEdit, "edit", PDO_DLGCMDBUTTON, NULL, "Edit" },
#define I_CUSTOMDEL		(2)
	{	PD_BUTTON, CustomDelete, "delete", 0, NULL, "Delete" },
#define I_CUSTOMCOPYTO	(3)
	{	PD_BUTTON, CustomExport, "export", 0, NULL, "Move To" },
#define I_CUSTOMNEW		(4)
	{	PD_MENU, NULL, "new", PDO_DLGWIDE, NULL, "New" },
	{	PD_MENUITEM, CarDlgAddDesc, "new-part-mi", 0, NULL, "Car Part" },
	{	PD_MENUITEM, CarDlgAddProto, "new-proto-mi", 0, NULL, "Car Prototype" }
  } ;
static paramGroup_t customPG = { "custmgm", 0, customPLs, sizeof customPLs/sizeof customPLs[0] };


typedef struct {
		custMgmCallBack_p proc;
		void * data;
		wIcon_p icon;
		} custMgmContext_t, *custMgmContext_p;


static void CustomDlgUpdate(
		paramGroup_p pg,
		int inx,
		void *valueP )
{
	custMgmContext_p context = NULL;
	wIndex_t selcnt = wListGetSelectedCount( (wList_p)customPLs[0].control );
	wIndex_t linx, lcnt;

	if ( inx != I_CUSTOMLIST ) return;
	if ( selcnt == 1 ) {
		lcnt = wListGetCount( (wList_p)pg->paramPtr[inx].control );
		for ( linx=0;
			  linx<lcnt && wListGetItemSelected( (wList_p)customPLs[0].control, linx ) != TRUE;
			  linx++ );
		if ( linx < lcnt ) {
			context = (custMgmContext_p)wListGetItemContext( (wList_p)pg->paramPtr[inx].control, linx );
			wButtonSetLabel( (wButton_p)customPLs[I_CUSTOMEDIT].control, context->proc( CUSTMGM_CAN_EDIT, context->data )?"Edit":"Rename" );
			ParamControlActive( &customPG, I_CUSTOMEDIT, TRUE );
		} else {
			ParamControlActive( &customPG, I_CUSTOMEDIT, FALSE );
		}
	} else {
		ParamControlActive( &customPG, I_CUSTOMEDIT, FALSE );
	}
	ParamControlActive( &customPG, I_CUSTOMDEL, selcnt>0 );
	ParamControlActive( &customPG, I_CUSTOMCOPYTO, selcnt>0 );
}


static void CustomEdit( void * action )
{
	custMgmContext_p context = NULL;
	wIndex_t selcnt = wListGetSelectedCount( (wList_p)customPLs[0].control );
	wIndex_t inx, cnt;

	if ( selcnt != 1 )
		return;
	cnt = wListGetCount( (wList_p)customPLs[0].control );
	for ( inx=0;
		  inx<cnt && wListGetItemSelected( (wList_p)customPLs[0].control, inx ) != TRUE;
		  inx++ );
	if ( inx >= cnt )
		return;
	context = (custMgmContext_p)wListGetItemContext( customSelL, inx );
	if ( context == NULL )
		return;
	context->proc( CUSTMGM_DO_EDIT, context->data );
#ifdef OBSOLETE
	context->proc( CUSTMGM_GET_TITLE, context->data );
	wListSetValues( customSelL, inx, message, context->icon, context );
#endif
}


static void CustomDelete( void * action )
{
	wIndex_t selcnt = wListGetSelectedCount( (wList_p)customPLs[0].control );
	wIndex_t inx, cnt;
	custMgmContext_p context = NULL;

	if ( selcnt <= 0 )
		return;
	if ( (!NoticeMessage2( 1, MSG_CUSTMGM_DELETE_CONFIRM, "Yes", "No", selcnt ) ) )
		return;
	cnt = wListGetCount( (wList_p)customPLs[0].control );
	for ( inx=0; inx<cnt; inx++ ) {
		if ( !wListGetItemSelected( (wList_p)customPLs[0].control, inx ) )
			continue;
		context = (custMgmContext_p)wListGetItemContext( customSelL, inx );
		context->proc( CUSTMGM_DO_DELETE, context->data );
		MyFree( context );
		wListDelete( customSelL, inx );
		inx--;
		cnt--;
	}
	DoChangeNotification( CHANGE_PARAMS );
}

static struct wFilSel_t * customMgmExport_fs;
EXPORT FILE * customMgmF;
static char custMgmContentsStr[STR_SIZE];
static BOOL_T custMgmProceed;
static paramData_t custMgmContentsPLs[] = {
	{ PD_STRING, custMgmContentsStr, "label", 0, (void*)400, "Label" } };
static paramGroup_t custMgmContentsPG = { "contents", 0, custMgmContentsPLs, sizeof custMgmContentsPLs/sizeof custMgmContentsPLs[0] };

static void CustMgmContentsOk( void * junk )
{
	custMgmProceed = TRUE;
	wHide( custMgmContentsPG.win );
}


static int CustomDoExport(
		const char * pathName,
		const char * fileName,
		void * data )
{
	int rc;
	wIndex_t selcnt = wListGetSelectedCount( (wList_p)customPLs[0].control );
	wIndex_t inx, cnt;
	custMgmContext_p context = NULL;

	if ( selcnt <= 0 )
		return FALSE;
	
	SetCurDir( pathName, fileName );
	rc = access( pathName, F_OK );
	if ( rc != -1 ) {
		rc = access( pathName, W_OK );
		if ( rc == -1 ) {
			NoticeMessage( MSG_CUSTMGM_CANT_WRITE, "Ok", NULL, pathName );
			return FALSE;
		}
		custMgmProceed = TRUE;
	} else {
		if ( custMgmContentsPG.win == NULL ) {
			ParamCreateDialog( &custMgmContentsPG, MakeWindowTitle("Contents Label"), "Ok", CustMgmContentsOk, wHide, TRUE, NULL, F_BLOCK, NULL );
		}
		custMgmProceed = FALSE;
		wShow( custMgmContentsPG.win );
	}
	if ( !custMgmProceed )
		return FALSE;
	customMgmF = fopen( pathName, "a" );
	if ( customMgmF == NULL ) {
		NoticeMessage( MSG_CUSTMGM_CANT_WRITE, "Ok", NULL, pathName );
		return FALSE;
	}
	if ( rc == -1 )
		fprintf( customMgmF, "CONTENTS %s\n", custMgmContentsStr );

	cnt = wListGetCount( (wList_p)customPLs[0].control );
	for ( inx=0; inx<cnt; inx++ ) {
		if ( !wListGetItemSelected( (wList_p)customPLs[0].control, inx ) )
			continue;
		context = (custMgmContext_p)wListGetItemContext( customSelL, inx );
		if ( context == NULL ) continue;
		if (!context->proc( CUSTMGM_DO_COPYTO, context->data )) {
			NoticeMessage( MSG_WRITE_FAILURE, "Ok", NULL, strerror(errno), pathName );
			fclose( customMgmF );
			return FALSE;
		}
		context->proc( CUSTMGM_DO_DELETE, context->data );
		MyFree( context );
		wListDelete( customSelL, inx );
		inx--;
		cnt--;
	}
	fclose( customMgmF );
	LoadParamFile( pathName, fileName, NULL );
	DoChangeNotification( CHANGE_PARAMS );
	return TRUE;
}


static void CustomExport( void * junk )
{
	if ( customMgmExport_fs == NULL )
		customMgmExport_fs = wFilSelCreate( mainW, FS_UPDATE, 0, "Move To XTP",
				"Parameter File|*.xtp", CustomDoExport, NULL );
	wFilSelect( customMgmExport_fs, curDirName );
}


static void CustomDone( void * action )
{
	FILE * f = OpenCustom( "w" );

	if (f == NULL) {
		wHide( customPG.win );
		return;
	}
	CompoundCustomSave(f);
	CarCustomSave(f);
	fclose(f);
	wHide( customPG.win );
}


EXPORT void CustMgmLoad(
		wIcon_p icon,
		custMgmCallBack_p proc,
		void * data )
{
	custMgmContext_p context;
	context = MyMalloc( sizeof *context );
	context->proc = proc;
	context->data = data;
	context->icon = icon;
	context->proc( CUSTMGM_GET_TITLE, context->data );
	wListAddValue( customSelL, message, icon, context );
}


static void LoadCustomMgmList( void )
{
	wIndex_t curInx, cnt=0;
	long tempL;
	custMgmContext_p context;
	custMgmContext_t curContext;

	curInx = wListGetIndex( customSelL );
	curContext.proc = NULL;
	curContext.data = NULL;
	curContext.icon = NULL;
	if ( curInx >= 0 ) {
		context = (custMgmContext_p)wListGetItemContext( customSelL, curInx );
		if ( context != NULL )
			curContext = *context;
	}
	cnt = wListGetCount( customSelL );
	for ( curInx=0; curInx<cnt; curInx++ ) {
		context = (custMgmContext_p)wListGetItemContext( customSelL, curInx );
		if ( context )
			MyFree( context );
	}
	curInx = wListGetIndex( customSelL );
	wControlShow( (wControl_p)customSelL, FALSE );
	wListClear( customSelL );

	CompoundCustMgmLoad();
	CarCustMgmLoad();

#ifdef LATER
	curInx = 0;
	cnt = wListGetCount( customSelL );
	if ( curContext.proc != NULL ) {
		for ( curInx=0; curInx<cnt; curInx++ ) {
			context = (custMgmContext_p)wListGetItemContext( customSelL, curInx );
			if ( context &&
				 context->proc == curContext.proc &&
				 context->data == curContext.data )
				break;
		}
	}
	if ( curInx >= cnt )
		curInx = (cnt>0?0:-1);

	wListSetIndex( customSelL, curInx );
	tempL = curInx;
#endif
	tempL = -1;
	CustomDlgUpdate( &customPG, I_CUSTOMLIST, &tempL );
	wControlShow( (wControl_p)customSelL, TRUE );
}


static void CustMgmChange( long changes )
{
	if (changes) {
		if (changed) {
			changed = 1;
			checkPtMark = 1;
		}
	}
	if ((changes&CHANGE_PARAMS) == 0 ||
		customPG.win == NULL || !wWinIsVisible(customPG.win) )
		return;

	LoadCustomMgmList();
}


static void DoCustomMgr( void * junk )
{
	if (customPG.win == NULL) {
		ParamCreateDialog( &customPG, MakeWindowTitle("Custom Update"), "Done", CustomDone, NULL, TRUE, NULL, F_RESIZE|F_RECALLSIZE|F_BLOCK, CustomDlgUpdate );
	} else {
		wListClear( customSelL );
	}

	/*ParamLoadControls( &customPG );*/
	/*ParamGroupRecord( &customPG );*/
	LoadCustomMgmList();
	wShow( customPG.win );
}


EXPORT addButtonCallBack_t CustomMgrInit( void )
{
	ParamRegister( &customPG );
	ParamRegister( &custMgmContentsPG );
	RegisterChangeNotification( CustMgmChange );
	return &DoCustomMgr;
}