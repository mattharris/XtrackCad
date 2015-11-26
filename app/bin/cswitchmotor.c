/** \file cswitchmotor.c
 * Switch Motors
 * Created by Robert Heller on Sat Mar 14 10:39:56 2009
 * ------------------------------------------------------------------
 * Modification History: $Log: not supported by cvs2svn $
 * Modification History: Revision 1.5  2009/11/23 19:46:16  rheller
 * Modification History: Block and Switchmotor updates
 * Modification History:
 * Modification History: Revision 1.4  2009/09/16 18:32:24  m_fischer
 * Modification History: Remove unused locals
 * Modification History:
 * Modification History: Revision 1.3  2009/09/05 16:40:53  m_fischer
 * Modification History: Make layout control commands a build-time choice
 * Modification History:
 * Modification History: Revision 1.2  2009/07/08 19:13:58  m_fischer
 * Modification History: Make compile under MSVC
 * Modification History:
 * Modification History: Revision 1.1  2009/07/08 18:40:27  m_fischer
 * Modification History: Add switchmotor and block for layout control
 * Modification History:
 * Modification History: Revision 1.1  2002/07/28 14:03:50  heller
 * Modification History: Add it copyright notice headers
 * Modification History:
 * ------------------------------------------------------------------
 * Contents:
 * ------------------------------------------------------------------
 *
 *     Generic Project
 *     Copyright (C) 2005  Robert Heller D/B/A Deepwoods Software
 * 			51 Locke Hill Road
 * 			Wendell, MA 01379-9728
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software
 *     Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */

#include <ctype.h>
#include "track.h"
#include "compound.h"
#include "i18n.h"

EXPORT TRKTYP_T T_SWITCHMOTOR = -1;

#define SWITCHMOTORCMD

static int log_switchmotor = 0;

#ifdef SWITCHMOTORCMD
static drawCmd_t switchmotorD = {
	NULL,
	&screenDrawFuncs,
	0,
	1.0,
	0.0,
	{0.0,0.0}, {0.0,0.0},
	Pix2CoOrd, CoOrd2Pix };

static char switchmotorName[STR_SHORT_SIZE];
static char switchmotorNormal[STR_LONG_SIZE];
static char switchmotorReverse[STR_LONG_SIZE];
static char switchmotorPointSense[STR_LONG_SIZE];
static track_p switchmotorTurnout;

static paramData_t switchmotorPLs[] = {
/*0*/ { PD_STRING, switchmotorName, "name", PDO_NOPREF, (void*)200, N_("Name") },
/*1*/ { PD_STRING, switchmotorNormal, "normal", PDO_NOPREF, (void*)350, N_("Normal") },
/*2*/ { PD_STRING, switchmotorReverse, "reverse", PDO_NOPREF, (void*)350, N_("Reverse") },
/*3*/ { PD_STRING, switchmotorPointSense, "pointSense", PDO_NOPREF, (void*)350, N_("Point Sense") }
};

static paramGroup_t switchmotorPG = { "switchmotor", 0, switchmotorPLs, sizeof switchmotorPLs/sizeof switchmotorPLs[0] };
/*
static dynArr_t switchmotorTrk_da;
#define switchmotorTrk(N) DYNARR_N( track_p , switchmotorTrk_da, N )
*/
static wWin_p switchmotorW;
#endif

typedef struct switchmotorData_t {
	char * name;
	char * normal;
	char * reverse;
	char * pointsense;
	track_p turnout;
} switchmotorData_t, *switchmotorData_p;

static switchmotorData_p GetswitchmotorData ( track_p trk )
{
	return (switchmotorData_p) GetTrkExtraData(trk);
}

#include "bitmaps/switchmotormark.xbm"
static wDrawBitMap_p switchmotormark_bm = NULL;

static void DrawSwitchMotor (track_p t, drawCmd_p d, wDrawColor color )
{
	coOrd p;
	switchmotorData_p data_p = GetswitchmotorData(t);
	struct extraData *xx = GetTrkExtraData(data_p->turnout);
	coOrd orig = xx->orig;
	ANGLE_T angle = xx->angle;

	if (switchmotormark_bm == NULL) {
		switchmotormark_bm =
			wDrawBitMapCreate( mainD.d,
					   switchmotormark_width,
					   switchmotormark_height, 16, 16,
					   switchmotormark_bits);
	}
	Translate (&p, orig, -angle , 2 );
	Translate (&p, p, 90-angle, 2);
	DrawBitMap(d, p, switchmotormark_bm, color);
}

static struct {
	char name[STR_SHORT_SIZE];
	char normal[STR_LONG_SIZE];
	char reverse[STR_LONG_SIZE];
	char pointsense[STR_LONG_SIZE];
	long turnout;
} switchmotorData;

typedef enum { NM, NOR, REV, PS, TO } switchmotorDesc_e;
static descData_t switchmotorDesc[] = {
/*NM */  { DESC_STRING, N_("Name"), &switchmotorData.name },
/*NOR*/  { DESC_STRING, N_("Normal"), &switchmotorData.normal },
/*REV*/  { DESC_STRING, N_("Reverse"), &switchmotorData.reverse },
/*PS */  { DESC_STRING, N_("Point Sense"), &switchmotorData.pointsense },
/*TO */  { DESC_LONG, N_("Turnout"), &switchmotorData.turnout },
	 { DESC_NULL } };

static void UpdateSwitchMotor (track_p trk, int inx, descData_p descUpd, BOOL_T needUndoStart )
{
	switchmotorData_p xx = GetswitchmotorData(trk);
	const char * thename, *thenormal, *thereverse, *thepointsense;
	char *newName, *newNormal, *newReverse, *newPointSense;
	BOOL_T changed, nChanged, norChanged, revChanged, psChanged;

	LOG( log_switchmotor, 1, ("*** UpdateSwitchMotor(): needUndoStart = %d\n",needUndoStart))
	if ( inx == -1 ) {
		nChanged = norChanged = revChanged = psChanged = changed = FALSE;
		thename = wStringGetValue( (wString_p)switchmotorDesc[NM].control0 );
		if ( strcmp( thename, xx->name ) != 0 ) {
			nChanged = changed = TRUE;
			newName = MyStrdup(thename);
		}
		thenormal = wStringGetValue( (wString_p)switchmotorDesc[NOR].control0 );
		if ( strcmp( thenormal, xx->normal ) != 0 ) {
			norChanged = changed = TRUE;
			newNormal = MyStrdup(thenormal);
		}
		thereverse = wStringGetValue( (wString_p)switchmotorDesc[REV].control0 );
		if ( strcmp( thereverse, xx->reverse ) != 0 ) {
			revChanged = changed = TRUE;
			newReverse = MyStrdup(thereverse);
		}
		thepointsense = wStringGetValue( (wString_p)switchmotorDesc[PS].control0 );
		if ( strcmp( thepointsense, xx->pointsense ) != 0 ) {
			psChanged = changed = TRUE;
			newPointSense = MyStrdup(thepointsense);
		}
		if ( ! changed ) return;
		if ( needUndoStart )
			UndoStart( _("Change Switch Motor"), "Change Switch Motor" );
		UndoModify( trk );
		if (nChanged) {
			MyFree(xx->name);
			xx->name = newName;
		}
		if (norChanged) {
			MyFree(xx->normal);
			xx->normal = newNormal;
		}
		if (revChanged) {
			MyFree(xx->reverse);
			xx->reverse = newReverse;
		}
		if (psChanged) {
			MyFree(xx->pointsense);
			xx->pointsense = newPointSense;
		}
		return;
	}
}

static DIST_T DistanceSwitchMotor (track_p t, coOrd * p )
{
	switchmotorData_p xx = GetswitchmotorData(t);
	return GetTrkDistance(xx->turnout,*p);
}

static void DescribeSwitchMotor (track_p trk, char * str, CSIZE_T len )
{
	switchmotorData_p xx = GetswitchmotorData(trk);
	long listLabelsOption = listLabels;

	LOG( log_switchmotor, 1, ("*** DescribeSwitchMotor(): trk is T%d\n",GetTrkIndex(trk)))
	FormatCompoundTitle( listLabelsOption, xx->name );
	if (message[0] == '\0')
		FormatCompoundTitle( listLabelsOption|LABEL_DESCR, xx->name );
	strcpy( str, _(GetTrkTypeName( trk )) );
	str++;
	while (*str) {
		*str = tolower((unsigned char)*str);
		str++;
	}
	sprintf( str, _("(%d): Layer=%d %s"),
		GetTrkIndex(trk), GetTrkLayer(trk)+1, message );
	strncpy(switchmotorData.name,xx->name,STR_SHORT_SIZE-1);
	switchmotorData.name[STR_SHORT_SIZE-1] = '\0';
	strncpy(switchmotorData.normal,xx->normal,STR_LONG_SIZE-1);
	switchmotorData.normal[STR_LONG_SIZE-1] = '\0';
	strncpy(switchmotorData.reverse,xx->reverse,STR_LONG_SIZE-1);
	switchmotorData.reverse[STR_LONG_SIZE-1] = '\0';
	strncpy(switchmotorData.pointsense,xx->pointsense,STR_LONG_SIZE-1);
	switchmotorData.pointsense[STR_LONG_SIZE-1] = '\0';
	switchmotorData.turnout = GetTrkIndex(xx->turnout);
	switchmotorDesc[TO].mode = DESC_RO;
	switchmotorDesc[NM].mode =
	switchmotorDesc[NOR].mode =
	switchmotorDesc[REV].mode =
	switchmotorDesc[PS].mode = DESC_NOREDRAW;
	DoDescribe(_("Switch motor"), trk, switchmotorDesc, UpdateSwitchMotor );
}

static switchmotorDebug (track_p trk)
{
	switchmotorData_p xx = GetswitchmotorData(trk);
	LOG( log_switchmotor, 1, ("*** switchmotorDebug(): trk = %08x\n",trk))
	LOG( log_switchmotor, 1, ("*** switchmotorDebug(): Index = %d\n",GetTrkIndex(trk)))
	LOG( log_switchmotor, 1, ("*** switchmotorDebug(): name = \"%s\"\n",xx->name))
	LOG( log_switchmotor, 1, ("*** switchmotorDebug(): normal = \"%s\"\n",xx->normal))
	LOG( log_switchmotor, 1, ("*** switchmotorDebug(): reverse = \"%s\"\n",xx->reverse))
	LOG( log_switchmotor, 1, ("*** switchmotorDebug(): pointsense = \"%s\"\n",xx->pointsense))
	LOG( log_switchmotor, 1, ("*** switchmotorDebug(): turnout = T%d, %s\n",
			GetTrkIndex(xx->turnout), GetTrkTypeName(xx->turnout)))
}

static void DeleteSwitchMotor ( track_p trk )
{
	switchmotorData_p xx = GetswitchmotorData(trk);
	MyFree(xx->name); xx->name = NULL;
	MyFree(xx->normal); xx->normal = NULL;
	MyFree(xx->reverse); xx->reverse = NULL;
	MyFree(xx->pointsense); xx->pointsense = NULL;
}

static BOOL_T WriteSwitchMotor ( track_p t, FILE * f )
{
	BOOL_T rc = TRUE;
	switchmotorData_p xx = GetswitchmotorData(t);

	rc &= fprintf(f, "SWITCHMOTOR %d %d \"%s\" \"%s\" \"%s\" \"%s\"\n",
		GetTrkIndex(t), GetTrkIndex(xx->turnout), xx->name,
		xx->normal, xx->reverse, xx->pointsense)>0;
	return rc;
}

static void ReadSwitchMotor ( char * line )
{
	TRKINX_T trkindex;
	wIndex_t index;
	track_p trk;
	switchmotorData_p xx;
	char *name, *normal, *reverse, *pointsense;

	LOG( log_switchmotor, 1, ("*** ReadSwitchMotor: line is '%s'\n",line))
	if (!GetArgs(line+12,"ddqqqq",&index,&trkindex,&name,&normal,&reverse,&pointsense)) {
		return;
	}
	trk = NewTrack(index, T_SWITCHMOTOR, 0, sizeof(switchmotorData_t)+1);
	xx = GetswitchmotorData( trk );
	xx->name = name;
	xx->normal = normal;
	xx->reverse = reverse;
	xx->pointsense = pointsense;
	xx->turnout = FindTrack(trkindex);
	switchmotorDebug(trk);
}

static void MoveSwitchMotor (track_p trk, coOrd orig ) {}
static void RotateSwitchMotor (track_p trk, coOrd orig, ANGLE_T angle ) {}
static void RescaleSwitchMotor (track_p trk, FLOAT_T ratio ) {}


static trackCmd_t switchmotorCmds = {
	"SWITCHMOTOR",
	DrawSwitchMotor,
	DistanceSwitchMotor,
	DescribeSwitchMotor,
	DeleteSwitchMotor,
	WriteSwitchMotor,
	ReadSwitchMotor,
	MoveSwitchMotor,
	RotateSwitchMotor,
	RescaleSwitchMotor,
	NULL, /* audit */
	NULL, /* getAngle */
	NULL, /* split */
	NULL, /* traverse */
	NULL, /* enumerate */
	NULL, /* redraw */
	NULL, /* trim */
	NULL, /* merge */
	NULL, /* modify */
	NULL, /* getLength */
	NULL, /* getTrkParams */
	NULL, /* moveEndPt */
	NULL, /* query */
	NULL, /* ungroup */
	NULL, /* flip */
	NULL, /* drawPositionIndicator */
	NULL, /* advancePositionIndicator */
	NULL, /* checkTraverse */
	NULL, /* makeParallel */
	NULL  /* drawDesc */
};

#ifdef SWITCHMOTORCMD
static track_p FindSwitchMotor (track_p trk)
{
	track_p a_trk;
	switchmotorData_p xx;

	for (a_trk = NULL; TrackIterate( &a_trk ) ;) {
		if (GetTrkType(a_trk) == T_SWITCHMOTOR) {
			xx =  GetswitchmotorData(a_trk);
			if (xx->turnout == trk) return a_trk;
		}
	}
	return NULL;
}

static void SwitchMotorOk ( void * junk )
{
	switchmotorData_p xx;
	track_p trk;

	LOG( log_switchmotor, 1, ("*** SwitchMotorOk()\n"))
	ParamUpdate (&switchmotorPG );
	if ( switchmotorName[0]==0 ) {
		NoticeMessage( 0, "Switch motor must have a name!", _("Ok"));
		return;
	}
	wDrawDelayUpdate( mainD.d, TRUE );
	UndoStart( _("Create Switch Motor"), "Create Switch Motor" );
	/* Create a switchmotor object */
	trk = NewTrack(0, T_SWITCHMOTOR, 0, sizeof(switchmotorData_t)+1);
	xx = GetswitchmotorData( trk );
	xx->name = MyStrdup(switchmotorName);
	xx->normal = MyStrdup(switchmotorNormal);
	xx->reverse = MyStrdup(switchmotorReverse);
	xx->pointsense = MyStrdup(switchmotorPointSense);
	xx->turnout = switchmotorTurnout;
	switchmotorDebug(trk);
	UndoEnd();
	wHide( switchmotorW );
}

static void NewSwitchMotorDialog(track_p trk)
{
	LOG( log_switchmotor, 1, ("*** NewSwitchMotorDialog()\n"))

	switchmotorTurnout = trk;
	if ( log_switchmotor < 0 ) log_switchmotor = LogFindIndex( "switchmotor" );
	if ( !switchmotorW ) {
		ParamRegister( &switchmotorPG );
		switchmotorW = ParamCreateDialog (&switchmotorPG, MakeWindowTitle(_("Create switch motor")), _("Ok"), SwitchMotorOk, wHide, TRUE, NULL, F_BLOCK, NULL );
		switchmotorD.dpi = mainD.dpi;
	}
	ParamLoadControls( &switchmotorPG );
	wShow( switchmotorW );
}

static STATUS_T CmdSwitchMotorCreate( wAction_t action, coOrd pos )
{
	track_p trk;

	LOG( log_switchmotor, 1, ("*** CmdSwitchMotorCreate(%08x,{%f,%f})\n",action,pos.x,pos.y))
	switch (action & 0xFF) {
	case C_START:
		InfoMessage( _("Select a turnout") );
		return C_CONTINUE;
	case C_DOWN:
		if ((trk = OnTrack(&pos, TRUE, TRUE )) == NULL) {
			return C_CONTINUE;
		}
		if (GetTrkType( trk ) != T_TURNOUT) {
			ErrorMessage( _("Not a turnout!") );
			return C_CONTINUE;
		}
		NewSwitchMotorDialog(trk);
		return C_CONTINUE;
	case C_REDRAW:
		return C_CONTINUE;
	case C_CANCEL:
		return C_TERMINATE;
	default:
		return C_CONTINUE;
	}
}

extern BOOL_T inDescribeCmd;

static STATUS_T CmdSwitchMotorEdit( wAction_t action, coOrd pos )
{
	track_p trk,btrk;
	char msg[STR_SIZE];

	switch (action) {
	case C_START:
		InfoMessage( _("Select a turnout") );
		inDescribeCmd = TRUE;
		return C_CONTINUE;
	case C_DOWN:
		if ((trk = OnTrack(&pos, TRUE, TRUE )) == NULL) {
			return C_CONTINUE;
		}
		btrk = FindSwitchMotor( trk );
		if ( !btrk ) {
			ErrorMessage( _("Not a switch motor!") );
			return C_CONTINUE;
		}
		DescribeTrack (btrk, msg, sizeof msg );
		InfoMessage( msg );
		return C_CONTINUE;
	case C_REDRAW:
		return C_CONTINUE;
	case C_CANCEL:
		inDescribeCmd = FALSE;
		return C_TERMINATE;
	default:
		return C_CONTINUE;
	}
}

static STATUS_T CmdSwitchMotorDelete( wAction_t action, coOrd pos )
{
	track_p trk,btrk;
	switchmotorData_p xx;

	switch (action) {
	case C_START:
		InfoMessage( _("Select a turnout") );
		return C_CONTINUE;
	case C_DOWN:
		if ((trk = OnTrack(&pos, TRUE, TRUE )) == NULL) {
			return C_CONTINUE;
		}
		btrk = FindSwitchMotor( trk );
		if ( !btrk ) {
			ErrorMessage( _("Not a switch motor!") );
			return C_CONTINUE;
		}
		/* Confirm Delete SwitchMotor */
		xx = GetswitchmotorData(btrk);
		if ( NoticeMessage( _("Really delete switch motor %s?"), _("Yes"), _("No"), xx->name) ) {
			UndoStart( _("Delete Switch Motor"), "delete" );
			DeleteTrack (btrk, FALSE);
			UndoEnd();
			return C_TERMINATE;
		}
		return C_CONTINUE;
	case C_REDRAW:
		return C_CONTINUE;
	case C_CANCEL:
		return C_TERMINATE;
	default:
		return C_CONTINUE;
	}
}



#define SWITCHMOTOR_CREATE 0
#define SWITCHMOTOR_EDIT   1
#define SWITCHMOTOR_DELETE 2

static STATUS_T CmdSwitchMotor (wAction_t action, coOrd pos )
{

	LOG( log_switchmotor, 1, ("*** CmdSwitchMotor(%08x,{%f,%f})\n",action,pos.x,pos.y))

	switch ((long)commandContext) {
	case SWITCHMOTOR_CREATE: return CmdSwitchMotorCreate(action,pos);
	case SWITCHMOTOR_EDIT:   return CmdSwitchMotorEdit(action,pos);
	case SWITCHMOTOR_DELETE: return CmdSwitchMotorDelete(action,pos);
	default: return C_TERMINATE;
	}
}

//#include "bitmaps/switchmotor.xpm"

#include "bitmaps/switchmnew.xpm"
#include "bitmaps/switchmedit.xpm"
#include "bitmaps/switchmdel.xpm"

EXPORT void InitCmdSwitchMotor( wMenu_p menu )
{
	switchmotorName[0] = '\0';
	switchmotorNormal[0] = '\0';
	switchmotorReverse[0] = '\0';
	switchmotorPointSense[0] = '\0';
	ButtonGroupBegin( _("SwitchMotor"), "cmdSwitchMotorSetCmd", _("Switch Motors") );
	AddMenuButton( menu, CmdSwitchMotor, "cmdSwitchMotorCreate", _("Create Switch Motor"), wIconCreatePixMap(switchmnew_xpm), LEVEL0_50, IC_CANCEL|IC_POPUP, ACCL_SWITCHMOTOR1, (void*)SWITCHMOTOR_CREATE );
	AddMenuButton( menu, CmdSwitchMotor, "cmdSwitchMotorEdit", _("Edit Switch Motor"), wIconCreatePixMap(switchmedit_xpm), LEVEL0_50, IC_CANCEL|IC_POPUP, ACCL_SWITCHMOTOR2, (void*)SWITCHMOTOR_EDIT );
	AddMenuButton( menu, CmdSwitchMotor, "cmdSwitchMotorDelete", _("Delete Switch Motor"), wIconCreatePixMap(switchmdel_xpm), LEVEL0_50, IC_CANCEL|IC_POPUP, ACCL_SWITCHMOTOR3, (void*)SWITCHMOTOR_DELETE );
	ButtonGroupEnd();
	ParamRegister( &switchmotorPG );
}
#endif


EXPORT void InitTrkSwitchMotor( void )
{
	T_SWITCHMOTOR = InitObject ( &switchmotorCmds );
	log_switchmotor = LogFindIndex ( "switchmotor" );
}


