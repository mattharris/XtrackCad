/** \file compound.c
 * Compound tracks: Turnouts and Structures
 *
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

#include <ctype.h>
#include "track.h"
#include "compound.h"
#include "shrtpath.h"
#include "cjoin.h"
#include "i18n.h"

#if _MSC_VER >=1400
#define strdup _strdup
#endif

/*****************************************************************************
 *
 * Misc
 *
 */

BOOL_T WriteCompoundPathsEndPtsSegs(
		FILE * f,
		PATHPTR_T paths,
		wIndex_t segCnt,
		trkSeg_p segs,
		EPINX_T endPtCnt,
		trkEndPt_t * endPts )
{
	int i;
	PATHPTR_T pp;
	BOOL_T rc = TRUE;
	for ( pp=paths; *pp; pp+=2 ) {
		rc &= fprintf( f, "\tP \"%s\"", pp )>0;
		for ( pp+=strlen((char *)pp)+1; pp[0]!=0||pp[1]!=0; pp++ )
			rc &= fprintf( f, " %d", *pp )>0;
		rc &= fprintf( f, "\n" )>0;
	}
	for ( i=0; i<endPtCnt; i++ )
		rc &= fprintf( f, "\tE %0.6f %0.6f %0.6f\n",
				endPts[i].pos.x, endPts[i].pos.y, endPts[i].angle )>0;
	rc &= WriteSegs( f, segCnt, segs )>0;
	return rc;
}


EXPORT void ParseCompoundTitle(
		char * title,
		char * * manufP,
		int * manufL,
		char * * nameP,
		int * nameL,
		char * * partnoP,
		int * partnoL )
{
	char * cp1, *cp2;
	int len;
	*manufP = *nameP = *partnoP = NULL;
	*manufL = *nameL = *partnoL = 0;
	len = strlen( title );
	cp1 = strchr( title, '\t' );
	if ( cp1 ) {
		cp2 = strchr( cp1+1, '\t' );
		if ( cp2 ) {
			cp2++;
			*partnoP = cp2;
			*partnoL = title+len-cp2;
			len = cp2-title-1;
		}
		cp1++;
		*nameP = cp1;
		*nameL = title+len-cp1;
		*manufP = title;
		*manufL = cp1-title-1;
	} else {
		*nameP = title;
		*nameL = len;
	}
}


void FormatCompoundTitle(
		long format,
		char * title )
{
	char *cp1, *cp2=NULL, *cq;
	int len;
	FLOAT_T price;
	BOOL_T needSep;
	cq = message;
	if (format&LABEL_COST) {
		FormatCompoundTitle( LABEL_MANUF|LABEL_DESCR|LABEL_PARTNO, title );
		wPrefGetFloat( "price list", message, &price, 0.0 );
		if (price > 0.00) {
			sprintf( cq, "%7.2f\t", price );
		} else {
			strcpy( cq, "\t" );
		}
		cq += strlen(cq);
	}
	cp1 = strchr( title, '\t' );
	if ( cp1 != NULL )
		cp2 = strchr( cp1+1, '\t' );
	if (cp2 == NULL) {
		if ( (format&LABEL_TABBED) ) {
			*cq++ = '\t';
			*cq++ = '\t';
		}
		strcpy( cq, title );
	} else {
		len = 0;
		needSep = FALSE;
		if ((format&LABEL_MANUF) && cp1-title>1) {
			len = cp1-title;
			memcpy( cq, title, len );
			cq += len;
			needSep = TRUE;
		}
		if ( (format&LABEL_TABBED) ) {
			*cq++ = '\t';
			needSep = FALSE;
		}
		if ((format&LABEL_PARTNO) && *(cp2+1)) {
			if ( needSep ) {
				*cq++ = ' ';
				needSep = FALSE;
			}
			strcpy( cq, cp2+1 );
			cq += strlen( cq );
			needSep = TRUE;
		}
		if ( (format&LABEL_TABBED) ) {
			*cq++ = '\t';
			needSep = FALSE;
		}
		if ((format&LABEL_DESCR) || !(format&LABEL_PARTNO)) {
			if ( needSep ) {
				*cq++ = ' ';
				needSep = FALSE;
			}
			if ( (format&LABEL_FLIPPED) ) {
				memcpy( cq, "Flipped ", 8 );
				cq += 8;
			}
			if ( (format&LABEL_UNGROUPED) ) {
				memcpy( cq, "Ungrouped ", 10 );
				cq += 10;
			}
			if ( (format&LABEL_SPLIT) ) {
				memcpy( cq, "Split ", 6 );
				cq += 6;
			}
			memcpy( cq, cp1+1, cp2-cp1-1 );
			cq += cp2-cp1-1;
			needSep = TRUE;
		}
		*cq = '\0';
	}
}



void ComputeCompoundBoundingBox(
		track_p trk )
{
	struct extraData *xx;
	coOrd hi, lo;

	xx = GetTrkExtraData(trk);

	GetSegBounds( xx->orig, xx->angle, xx->segCnt, xx->segs, &lo, &hi );
	hi.x += lo.x;
	hi.y += lo.y;
	SetBoundingBox( trk, hi, lo );
}


turnoutInfo_t * FindCompound( long type, char * scale, char * title )
{
	turnoutInfo_t * to;
	wIndex_t inx;
	SCALEINX_T scaleInx;

	if ( scale )
		scaleInx = LookupScale( scale );
	else
		scaleInx = -1;
	if ( type&FIND_TURNOUT )
	for (inx=0; inx<turnoutInfo_da.cnt; inx++) {
		to = turnoutInfo(inx);
		if ( IsParamValid(to->paramFileIndex) &&
			 to->segCnt > 0 &&
			 (scaleInx == -1 || to->scaleInx == scaleInx ) &&
			 to->segCnt != 0 &&
			 strcmp( to->title, title ) == 0 ) {
			return to;
		}
	}
	if ( type&FIND_STRUCT )
	for (inx=0; inx<structureInfo_da.cnt; inx++) {
		to = structureInfo(inx);
		if ( IsParamValid(to->paramFileIndex) &&
			 to->segCnt > 0 &&
			 (scaleInx == -1 || to->scaleInx == scaleInx ) &&
			 to->segCnt != 0 &&
			 strcmp( to->title, title ) == 0 ) {
			return to;
		}
	}
	return NULL;
}


char * CompoundGetTitle( turnoutInfo_t * to )
{
	return to->title;
}


EXPORT void CompoundClearDemoDefns( void )
{
	turnoutInfo_t * to;
	wIndex_t inx;

	for (inx=0; inx<turnoutInfo_da.cnt; inx++) {
		to = turnoutInfo(inx);
		if ( to->paramFileIndex == PARAM_CUSTOM && strcasecmp( GetScaleName(to->scaleInx), "DEMO" ) == 0 )
			to->segCnt = 0;
	}
	for (inx=0; inx<structureInfo_da.cnt; inx++) {
		to = structureInfo(inx);
		if ( to->paramFileIndex == PARAM_CUSTOM	 && strcasecmp( GetScaleName(to->scaleInx), "DEMO" ) == 0 )
			to->segCnt = 0;
	}
}

/*****************************************************************************
 *
 * Descriptions
 *
 */

void SetDescriptionOrig(
		track_p trk )
{
	struct extraData *xx = GetTrkExtraData(trk);
	int i, j;
	coOrd p0, p1;

	for (i=0,j=-1;i<xx->segCnt;i++) {
		if ( IsSegTrack( &xx->segs[i] ) ) {
			if (j == -1) {
				j = i;
			} else {
				j = -1;
				break;
			}
		}
	}
	if (j != -1 && xx->segs[j].type == SEG_CRVTRK) {
		REORIGIN( p0, xx->segs[j].u.c.center, xx->angle, xx->orig )
		Translate( &p0, p0,
				xx->segs[j].u.c.a0 + xx->segs[j].u.c.a1/2.0 + xx->angle,
				fabs(xx->segs[j].u.c.radius) );

	} else {
		GetBoundingBox( trk, (&p0), (&p1) );
		p0.x = (p0.x+p1.x)/2.0;
		p0.y = (p0.y+p1.y)/2.0;
	}
	Rotate( &p0, xx->orig, -xx->angle );
	xx->descriptionOrig.x = p0.x - xx->orig.x;
	xx->descriptionOrig.y = p0.y - xx->orig.y;
}


void DrawCompoundDescription(
		track_p trk,
		drawCmd_p d,
		wDrawColor color )
{
	wFont_p fp;
	coOrd p1;
	struct extraData *xx = GetTrkExtraData(trk);
	char * desc;
	long layoutLabelsOption = layoutLabels;

	if (layoutLabels == 0)
		return;
	if ((labelEnable&LABELENABLE_TRKDESC)==0)
		return;
	if ( (d->options&DC_GROUP) )
		return;
		if ( xx->special == TOpier ) {
			desc = xx->u.pier.name;
		} else {
			if ( xx->flipped )
				layoutLabelsOption |= LABEL_FLIPPED;
			if ( xx->ungrouped )
				layoutLabelsOption |= LABEL_UNGROUPED;
			if ( xx->split )
				layoutLabelsOption |= LABEL_SPLIT;
			FormatCompoundTitle( layoutLabelsOption, xtitle(xx) );
			desc = message;
		}
		p1 = xx->descriptionOrig;
		Rotate( &p1, zero, xx->angle );
		p1.x += xx->orig.x + xx->descriptionOff.x;
		p1.y += xx->orig.y + xx->descriptionOff.y;
#ifdef LATER
		maxInx = -1;
		for ( inx=0,a=0.0; a<360.0; inx++,a+=45 ) {
			Translate( &p1, p0, a, trackGauge*3 );
			dists[inx].p = p1;
			if ((trk1 = dists[inx].trk = OnTrack( &p1, FALSE, TRUE )) == NULL ||
				trk1 == trk ) {
				p1 = dists[inx].p;
				dists[inx].d = DistanceSegs( xx->orig, xx->angle, xx->segCnt, xx->segs, &p1, NULL );
			} else if ( GetTrkType(trk1) == T_TURNOUT ) {
				struct extraData *yy = GetTrkExtraData(trk1);
				dists[inx].d = DistanceSegs( yy->orig, yy->angle, yy->segCnt, yy->segs, &p1, NULL );
			} else {
				dists[inx].d = FindDistance( p0, p1 );
			}
		}
		maxD = 0; maxInx = -1;
		for ( inx=0,a=0.0; a<360.0; inx++,a+=45 ) {
			if (dists[inx].trk == NULL || dists[inx].trk == trk) {
				if (dists[inx].d > maxD) {
					maxD = dists[inx].d;
					maxInx = inx;
				}
			}
		}
		if (maxInx == -1) {
				if (dists[inx].d > maxD) {
					maxD = dists[inx].d;
					maxInx = inx;
				}
		}
		if (maxInx != -1) {
			p0 = dists[maxInx].p;
		}
#endif
	fp = wStandardFont( F_TIMES, FALSE, FALSE );
	DrawBoxedString( (xx->special==TOpier)?BOX_INVERT:BOX_NONE, d, p1, desc, fp, (wFontSize_t)descriptionFontSize, color, 0.0 );
}


DIST_T CompoundDescriptionDistance(
		coOrd pos,
		track_p trk )
{
	struct extraData *xx = GetTrkExtraData(trk);
	coOrd p1;
	if (GetTrkType(trk) != T_TURNOUT && GetTrkType(trk) != T_STRUCTURE)
		return 100000;
	p1 = xx->descriptionOrig;
	Rotate( &p1, zero, xx->angle );
	p1.x += xx->orig.x + xx->descriptionOff.x;
	p1.y += xx->orig.y + xx->descriptionOff.y;
	return FindDistance( p1, pos );
}


STATUS_T CompoundDescriptionMove(
		track_p trk,
		wAction_t action,
		coOrd pos )
{
	struct extraData *xx = GetTrkExtraData(trk);
	static coOrd p0, p1;
	wDrawColor color;

	switch (action) {
	case C_DOWN:
		REORIGIN( p0, xx->descriptionOrig, xx->angle, xx->orig )

	case C_MOVE:
	case C_UP:
		if (action != C_DOWN)
			DrawLine( &tempD, p0, p1, 0, wDrawColorBlack );
		color = GetTrkColor( trk, &mainD );
		DrawCompoundDescription( trk, &tempD, color );
		xx->descriptionOff.x = (pos.x-p0.x);
		xx->descriptionOff.y = (pos.y-p0.y);
		p1 = xx->descriptionOrig;
		Rotate( &p1, zero, xx->angle );
		p1.x += xx->orig.x + xx->descriptionOff.x;
		p1.y += xx->orig.y + xx->descriptionOff.y;
		DrawCompoundDescription( trk, &tempD, color );
		if (action != C_UP)
			DrawLine( &tempD, p0, p1, 0, wDrawColorBlack );
        MainRedraw();
		return action==C_UP?C_TERMINATE:C_CONTINUE;
	}
	return C_CONTINUE;
}



/*****************************************************************************
 *
 * Generics
 *
 */


EXPORT void GetSegInxEP(
		signed char segChar,
		int * segInx,
		EPINX_T * segEP )
{
	int inx;
	inx = segChar;
	if (inx > 0 ) {
		*segInx = (inx)-1;
		*segEP = 0;
	} else {
		*segInx = (-inx)-1;
		*segEP = 1;
	}
}


DIST_T DistanceCompound(
		track_p t,
		coOrd * p )
{
	struct extraData *xx = GetTrkExtraData(t);
	EPINX_T ep;
	DIST_T d0, d1;
	coOrd p0, p2;
	PATHPTR_T path;
	int segInx;
	EPINX_T segEP;
	segProcData_t segProcData;

	if ( onTrackInSplit && GetTrkEndPtCnt(t) > 0 ) {
		d0 = DistanceSegs( xx->orig, xx->angle, xx->segCnt, xx->segs, p, NULL );
	} else if ( programMode != MODE_TRAIN || GetTrkEndPtCnt(t) <= 0 ) {
		d0 = DistanceSegs( xx->orig, xx->angle, xx->segCnt, xx->segs, p, NULL );
		if (programMode != MODE_TRAIN && GetTrkEndPtCnt(t) > 0 && d0 < 10000.0) {
			ep = PickEndPoint( *p, t );
			*p = GetTrkEndPos(t,ep);
		}
	} else {
		p0 = *p;
		Rotate( &p0, xx->orig, -xx->angle );
		p0.x -= xx->orig.x;
		p0.y -= xx->orig.y;
		d0 = 1000000.0;
		path = xx->pathCurr;
		for ( path=xx->pathCurr+strlen((char *)xx->pathCurr)+1; path[0] || path[1]; path++ ) {
			if ( path[0] != 0 ) {
				d1 = 1000000.0;
				GetSegInxEP( *path, &segInx, &segEP );
				segProcData.distance.pos1 = p0;
				SegProc( SEGPROC_DISTANCE, &xx->segs[segInx], &segProcData );
				if ( segProcData.distance.dd < d0 ) {
					d0 = segProcData.distance.dd;
					p2 = segProcData.distance.pos1;
				}
			}
		}
		if ( d0 < 1000000.0 ) {
			p2.x += xx->orig.x;
			p2.y += xx->orig.y;
			Rotate( &p2, xx->orig, xx->angle );
			*p = p2;
		}
	}
	return d0;
}


static struct {
		coOrd endPt[2];
		FLOAT_T elev[2];
		coOrd orig;
		ANGLE_T angle;
		char manuf[STR_SIZE];
		char name[STR_SIZE];
		char partno[STR_SIZE];
		long epCnt;
		long segCnt;
		FLOAT_T grade;
		DIST_T length;
		LAYER_T layerNumber;
		} compoundData;
typedef enum { E0, Z0, E1, Z1, GR, OR, AN, MN, NM, PN, EC, SC, LY } compoundDesc_e;
static descData_t compoundDesc[] = {
/*E0*/	{ DESC_POS, N_("End Pt 1: X"), &compoundData.endPt[0] },
/*Z0*/	{ DESC_DIM, N_("Z"), &compoundData.elev[0] },
/*E1*/	{ DESC_POS, N_("End Pt 2: X"), &compoundData.endPt[1] },
/*Z1*/	{ DESC_DIM, N_("Z"), &compoundData.elev[1] },
/*GR*/	{ DESC_FLOAT, N_("Grade"), &compoundData.grade },
/*OR*/	{ DESC_POS, N_("Origin: X"), &compoundData.orig },
/*AN*/	{ DESC_ANGLE, N_("Angle"), &compoundData.angle },
/*MN*/	{ DESC_STRING, N_("Manufacturer"), &compoundData.manuf },
/*NM*/	{ DESC_STRING, N_("Name"), &compoundData.name },
/*PN*/	{ DESC_STRING, N_("Part No"), &compoundData.partno },
/*EC*/	{ DESC_LONG, N_("# End Pt"), &compoundData.epCnt },
/*SC*/	{ DESC_LONG, N_("# Segments"), &compoundData.segCnt },
/*LY*/	{ DESC_LAYER, N_("Layer"), &compoundData.layerNumber },
		{ DESC_NULL } };



static void UpdateCompound( track_p trk, int inx, descData_p descUpd, BOOL_T needUndoStart )
{
	struct extraData *xx = GetTrkExtraData(trk);
	const char * manufS, * nameS, * partnoS;
	char * mP, *nP, *pP;
	int mL, nL, pL;
	coOrd hi, lo;
	coOrd pos;
	EPINX_T ep;
	BOOL_T titleChanged, flipped, ungrouped, split;
	char * newTitle;

	if ( inx == -1 ) {
		titleChanged = FALSE;
		ParseCompoundTitle( xtitle(xx), &mP, &mL, &nP, &nL, &pP, &pL );
		if (mP == NULL) mP = "";
		if (nP == NULL) nP = "";
		if (pP == NULL) pP = "";
		manufS = wStringGetValue( (wString_p)compoundDesc[MN].control0 );
		strcpy( message, manufS );
		if ( strncmp( manufS, mP, mL ) != 0 || manufS[mL] != '\0' ) {
			titleChanged = TRUE;
		}
		flipped = xx->flipped;
		ungrouped = xx->ungrouped;
		split = xx->split;
		nameS = wStringGetValue( (wString_p)compoundDesc[NM].control0 );
		if ( strncmp( nameS, "Flipped ", 8 ) == 0 ) {
			nameS += 8;
			flipped = TRUE;
		} else {
			flipped = FALSE;
		}
		if ( strncmp( nameS, "Ungrouped ", 10 ) == 0 ) {
			nameS += 10;
			ungrouped = TRUE;
		} else {
			ungrouped = FALSE;
		}
		if ( strncmp( nameS, "Split ", 6 ) == 0 ) {
			nameS += 6;
			split = TRUE;
		} else {
			split = FALSE;
		}
		if ( strncmp( nameS, nP, nL ) != 0 || nameS[nL] != '\0' ||
			 xx->flipped != flipped ||
			 xx->ungrouped != ungrouped ||
			 xx->split != split ) {
			titleChanged = TRUE;
		}
		strcat( message, "\t" );
		strcat( message, nameS );
		partnoS = wStringGetValue( (wString_p)compoundDesc[PN].control0 );
		strcat( message, "\t" );
		strcat( message, partnoS );
		newTitle = MyStrdup( message );
		if ( strncmp( partnoS, pP, pL ) != 0 || partnoS[pL] != '\0' ) {
			titleChanged = TRUE;
		}
		if ( ! titleChanged )
			return;
		if ( needUndoStart )
		   UndoStart( _("Change Track"), "Change Track" );
		UndoModify( trk );
		GetBoundingBox( trk, &hi, &lo );
		if ( labelScale >= mainD.scale &&
			 !OFF_MAIND( lo, hi ) ) {
			DrawCompoundDescription( trk, &tempD, GetTrkColor(trk,&tempD) );
		}
		/*sprintf( message, "%s\t%s\t%s", manufS, nameS, partnoS );*/
		xx->title = newTitle;
		xx->flipped = flipped;
		xx->ungrouped = ungrouped;
		xx->split = split;
		if ( labelScale >= mainD.scale &&
			 !OFF_MAIND( lo, hi ) ) {
			DrawCompoundDescription( trk, &tempD, GetTrkColor(trk,&tempD) );
		}
		return;
	}

	UndrawNewTrack( trk );
	switch ( inx ) {
	case OR:
		pos.x = compoundData.orig.x - xx->orig.x;
		pos.y = compoundData.orig.y - xx->orig.y;
		MoveTrack( trk, pos );
		ComputeCompoundBoundingBox( trk );
		break;
	case AN:
		RotateTrack( trk, xx->orig, NormalizeAngle( compoundData.angle-xx->angle ) );
		ComputeCompoundBoundingBox( trk );
		break;
	case E0:
	case E1:
		ep = (inx==E0?0:1);
		pos = GetTrkEndPos(trk,ep);
		pos.x = compoundData.endPt[ep].x - pos.x;
		pos.y = compoundData.endPt[ep].y - pos.y;
		MoveTrack( trk, pos );
		ComputeCompoundBoundingBox( trk );
		if ( compoundData.epCnt >= 2 ) {
			compoundData.endPt[1-ep] = GetTrkEndPos(trk,1-ep);
			compoundDesc[inx==E0?E1:E0].mode |= DESC_CHANGE;
		}
		break;
	case Z0:
	case Z1:
		ep = (inx==Z0?0:1);
		UpdateTrkEndElev( trk, ep, GetTrkEndElevUnmaskedMode(trk,ep), compoundData.elev[ep], NULL );
		if ( GetTrkEndPtCnt(trk) == 1 )
			 break;
		ComputeElev( trk, 1-ep, FALSE, &compoundData.elev[1-ep], NULL );
		if ( compoundData.length > minLength )
			compoundData.grade = fabs( (compoundData.elev[0]-compoundData.elev[1])/compoundData.length )*100.0;
		else
			compoundData.grade = 0.0;
		compoundDesc[GR].mode |= DESC_CHANGE;
		compoundDesc[inx==Z0?Z1:Z0].mode |= DESC_CHANGE;
		break;
	case LY:
		SetTrkLayer( trk, compoundData.layerNumber);
		break;
	default:
		break;
	}
	DrawNewTrack( trk );

}


void DescribeCompound(
		track_p trk,
		char * str,
		CSIZE_T len )
{
	struct extraData *xx = GetTrkExtraData(trk);
	int fix;
	EPINX_T ep, epCnt;
	char * mP, *nP, *pP, *cnP;
	int mL, nL, pL;
	long mode;
	long listLabelsOption = listLabels;

	if ( xx->flipped )
		listLabelsOption |= LABEL_FLIPPED;
	if ( xx->ungrouped )
		listLabelsOption |= LABEL_UNGROUPED;
	if ( xx->split )
		listLabelsOption |= LABEL_SPLIT;
	FormatCompoundTitle( listLabelsOption, xtitle(xx) );
	if (message[0] == '\0')
		FormatCompoundTitle( listLabelsOption|LABEL_DESCR, xtitle(xx) );
	strcpy( str, _(GetTrkTypeName( trk )) );
	str++;
	while (*str) {
		*str = tolower((unsigned char)*str);
		str++;
	}
	sprintf( str, _("(%d): Layer=%d %s"),
		GetTrkIndex(trk), GetTrkLayer(trk)+1, message );

	epCnt = GetTrkEndPtCnt(trk);
	fix = 0;
	for ( ep=0; ep<epCnt; ep++ ) {
		if (GetTrkEndTrk(trk,ep)) {
			fix = 1;
			break;
		}
	}
	compoundData.orig = xx->orig;
	compoundData.angle = xx->angle;
	ParseCompoundTitle( xtitle(xx), &mP, &mL, &nP, &nL, &pP, &pL );
	if (mP) {
		memcpy( compoundData.manuf, mP, mL );
		compoundData.manuf[mL] = 0;
	} else {
		compoundData.manuf[0] = 0;
	}
	if (nP) {
		cnP = compoundData.name;
		if ( xx->flipped ) {
			memcpy( cnP, "Flipped ", 8 );
			cnP += 8;
		}
		if ( xx->ungrouped ) {
			memcpy( cnP, "Ungrouped ", 10 );
			cnP += 10;
		}
		if ( xx->split ) {
			memcpy( cnP, "Split ", 6 );
			cnP += 6;
		}
		memcpy( cnP, nP, nL );
		cnP[nL] = 0;
	} else {
		compoundData.name[0] = 0;
	}
	if (pP) {
		memcpy( compoundData.partno, pP, pL );
		compoundData.partno[pL] = 0;
	} else {
		compoundData.partno[0] = 0;
	}
	compoundData.epCnt = GetTrkEndPtCnt(trk);
	compoundData.segCnt = xx->segCnt;
	compoundData.length = 0;
	compoundData.layerNumber = GetTrkLayer( trk );
	compoundDesc[E0].mode =
	compoundDesc[Z0].mode =
	compoundDesc[E1].mode =
	compoundDesc[Z1].mode =
	compoundDesc[GR].mode = DESC_IGNORE;
	compoundDesc[OR].mode =
	compoundDesc[AN].mode = fix?DESC_RO:0;
	compoundDesc[MN].mode =
	compoundDesc[NM].mode =
	compoundDesc[PN].mode = 0 /*DESC_NOREDRAW*/;
	compoundDesc[EC].mode =
	compoundDesc[SC].mode =
	compoundDesc[LY].mode = DESC_NOREDRAW;
	if ( compoundData.epCnt ) {
		if ( compoundData.epCnt <=2 ) {
			if ( GetTrkEndTrk(trk,0) || (compoundData.epCnt==2 && GetTrkEndTrk(trk,1)) )
				mode = DESC_RO;
			else
				mode = 0;
			compoundDesc[OR].mode = DESC_IGNORE;
			compoundDesc[AN].mode = DESC_IGNORE;
			compoundDesc[EC].mode = DESC_IGNORE;
			compoundData.endPt[0] = GetTrkEndPos(trk,0);
			ComputeElev( trk, 0, FALSE, &compoundData.elev[0], NULL );
			compoundDesc[E0].mode = (int)mode;
			compoundDesc[Z0].mode = (EndPtIsDefinedElev(trk,0)?0:DESC_RO)|DESC_NOREDRAW;
			if ( compoundData.epCnt == 2 ) {
				compoundData.length = GetTrkLength( trk, 0, 1 );
				compoundData.endPt[1] = GetTrkEndPos(trk,1);
				ComputeElev( trk, 1, FALSE, &compoundData.elev[1], NULL );
				compoundDesc[E1].mode = (int)mode;
				compoundDesc[Z1].mode = (EndPtIsDefinedElev(trk,1)?0:DESC_RO)|DESC_NOREDRAW;
				compoundDesc[GR].mode = DESC_RO;
				if ( compoundData.length > minLength )
					compoundData.grade = fabs( (compoundData.elev[0]-compoundData.elev[1])/compoundData.length )*100.0;
				else
					compoundData.grade = 0.0;
			}
		}
		DoDescribe( compoundData.epCnt>2?_("Turnout"):_("Sectional Track"), trk, compoundDesc, UpdateCompound );
	} else {
		compoundDesc[EC].mode |= DESC_IGNORE;
		DoDescribe( _("Structure"), trk, compoundDesc, UpdateCompound );
	}
}


void DeleteCompound(
		track_p t )
{
}


BOOL_T WriteCompound(
		track_p t,
		FILE * f )
{
	struct extraData *xx = GetTrkExtraData(t);
	EPINX_T ep, epCnt;
	long options;
	long position = 0;
	PATHPTR_T path;
	BOOL_T rc = TRUE;

	options = (long)GetTrkWidth(t);
	if (xx->handlaid)
		options |= 0x08;
	if (xx->flipped)
		options |= 0x10;
	if (xx->ungrouped)
		options |= 0x20;
	if (xx->split)
		options |= 0x40;
	if ( ( GetTrkBits( t ) & TB_HIDEDESC ) != 0 )
		options |= 0x80;
	epCnt = GetTrkEndPtCnt(t);
	if ( epCnt > -0 ) {
		path = xx->paths;
		while ( path != xx->pathCurr ) {
			path += strlen((char*)path)+1;
			while ( path[0] || path[1] )
				path++;
			path += 2;
			if ( *path == 0 )
				break;
			position++;
		}
	}
	rc &= fprintf(f, "%s %d %d %ld %ld 0 %s %d %0.6f %0.6f 0 %0.6f \"%s\"\n",
				GetTrkTypeName(t),
				GetTrkIndex(t), GetTrkLayer(t), options, position,
				GetTrkScaleName(t), GetTrkVisible(t),
				xx->orig.x, xx->orig.y, xx->angle,
				PutTitle(xtitle(xx)) )>0;
	for (ep=0; ep<epCnt; ep++ )
		WriteEndPt( f, t, ep );
	switch ( xx->special ) {
	case TOadjustable:
		rc &= fprintf( f, "\tX %s %0.3f %0.3f\n", ADJUSTABLE,
				xx->u.adjustable.minD, xx->u.adjustable.maxD )>0;
		break;
	case TOpier:
		rc &= fprintf( f, "\tX %s %0.6f \"%s\"\n", PIER, xx->u.pier.height, xx->u.pier.name )>0;
	default:
		;
	}
	rc &= fprintf( f, "\tD %0.6f %0.6f\n", xx->descriptionOff.x, xx->descriptionOff.y )>0;
	rc &= WriteCompoundPathsEndPtsSegs( f, xpaths(xx), xx->segCnt, xx->segs, 0, NULL );
	return rc;
}




/*****************************************************************************
 *
 * Generic Functions
 *
 */


EXPORT track_p NewCompound(
		TRKTYP_T trkType,
		TRKINX_T index,
		coOrd pos,
		ANGLE_T angle,
		char * title,
		EPINX_T epCnt,
		trkEndPt_t * epp,
		int pathLen,
		char * paths,
		wIndex_t segCnt,
		trkSeg_p segs )
{
	track_p trk;
	struct extraData * xx;
	EPINX_T ep;

	trk = NewTrack( index, trkType, epCnt, sizeof (*xx) + 1 );
	xx = GetTrkExtraData(trk);
	xx->orig = pos;
	xx->angle = angle;
	xx->handlaid = FALSE;
	xx->flipped = FALSE;
	xx->ungrouped = FALSE;
	xx->split = FALSE;
	xx->descriptionOff = zero;
	xx->descriptionSize = zero;
	xx->title = MyStrdup( title );
	xx->customInfo = NULL;
	xx->special = TOnormal;
	if ( pathLen > 0 )
		xx->paths = memdup( paths, pathLen );
	else
		xx->paths = (PATHPTR_T)"";
	xx->pathLen = pathLen;
	xx->pathCurr = xx->paths;
	xx->segCnt = segCnt;
	xx->segs = memdup( segs, segCnt * sizeof *segs );
	ComputeCompoundBoundingBox( trk );
	SetDescriptionOrig( trk );
	for ( ep=0; ep<epCnt; ep++ )
		SetTrkEndPoint( trk, ep, epp[ep].pos, epp[ep].angle );
	return trk;
}


void ReadCompound(
		char * line,
		TRKTYP_T trkType )
{
	track_p trk;
	struct extraData *xx;
	TRKINX_T index;
	BOOL_T visible;
	coOrd orig;
	DIST_T elev;
	ANGLE_T angle;
	char scale[10];
	char *title;
	wIndex_t layer;
	char *cp;
	long options = 0;
	long position = 0;
	PATHPTR_T path=NULL;

	if (paramVersion<3) {
		if ( !GetArgs( line, "dXsdpfq",
			&index, &layer, scale, &visible, &orig, &angle, &title ) )
			return;
	} else if (paramVersion <= 5 && trkType == T_STRUCTURE) {
		if ( !GetArgs( line, "dL00sdpfq",
			&index, &layer, scale, &visible, &orig, &angle, &title ) )
			return;
	} else {
		if ( !GetArgs( line, paramVersion<9?"dLll0sdpYfq":"dLll0sdpffq",
			&index, &layer, &options, &position, scale, &visible, &orig, &elev, &angle, &title ) )
			return;
	}
	if (paramVersion >=3 && paramVersion <= 5 && trkType == T_STRUCTURE)
		strcpy( scale, curScaleName );
	DYNARR_RESET( trkEndPt_t, tempEndPts_da );
	pathCnt = 0;
	ReadSegs();
	path = pathPtr;
	if ( tempEndPts_da.cnt > 0 && pathCnt <= 1 ) {
		pathCnt = 10;
		path = (PATHPTR_T)"Normal\01\0\0";
	}
	if (paramVersion<6 && strlen( title ) > 2) {
		cp = strchr( title, '\t' );
		if (cp != NULL) {
			cp = strchr( cp, '\t' );
		}
		if (cp == NULL) {
			UpdateTitleMark( title, LookupScale(scale) );
		}
	}
	trk = NewCompound( trkType, index, orig, angle, title, 0, NULL, pathCnt, (char *)path, tempSegs_da.cnt, &tempSegs(0) );
	SetEndPts( trk, 0 );
	SetTrkVisible(trk, visible);
	SetTrkScale(trk, LookupScale( scale ));
	SetTrkLayer(trk, layer);
	SetTrkWidth(trk, (int)(options&3));
	xx = GetTrkExtraData(trk);
	xx->handlaid = (int)((options&0x08)!=0);
	xx->flipped = (int)((options&0x10)!=0);
	xx->ungrouped = (int)((options&0x20)!=0);
	xx->split = (int)((options&0x40)!=0);
	xx->descriptionOff = descriptionOff;
	if ( ( options & 0x80 ) != 0 )
		SetTrkBits( trk, TB_HIDEDESC );
#ifdef LATER
	trk = NewTrack( index, trkType, 0, sizeof (*xx) + 1 );
	SetEndPts( trk, 0 );
	xx = GetTrkExtraData(trk);
	SetTrkVisible(trk, visible);
	SetTrkScale(trk, LookupScale( scale ));
	SetTrkLayer(trk, layer);
	SetTrkWidth(trk, (int)(options&3));
	xx->orig = orig;
	xx->angle = angle;
	xx->customInfo = NULL;
	xx->handlaid = (int)((options>>3)&0x01);
	xx->flipped = (int)((options>>4)&0x01);
	xx->segCnt = tempSegs_da.cnt;
	xx->segs = MyMalloc( (tempSegs_da.cnt)*sizeof xx->segs[0] );
	if (paramVersion<6 && strlen( title ) > 2) {
		cp = strchr( title, '\t' );
		if (cp != NULL) {
			cp = strchr( cp, '\t' );
		}
		if (cp == NULL) {
			UpdateTitleMark(title, GetTrkScale(trk));
		}
	}
	xx->title = title;
	if ( GetTrkEndPtCnt(trk) > 0 && pathCnt <= 1 ) {
		xx->pathLen = 10;
		xx->paths = xx->pathCurr = (PATHPTR_T)Malloc( xx->pathLen );
		memcpy( xx->paths, "Normal\01\0\0", xx->pathLen );
	} else {
		xx->pathLen = pathCnt;
		if (pathCnt > 0) {
			xx->paths = xx->pathCurr = (PATHPTR_T)Malloc( pathCnt );
			memcpy( xpaths(xx), pathPtr, pathCnt );
		} else {
			xx->paths = xx->pathCurr = NULL;
		}
	}
	xx->segCnt = tempSegs_da.cnt;
	memcpy( xx->segs, tempSegs_da.ptr, tempSegs_da.cnt * sizeof *xx->segs );

	ComputeCompoundBoundingBox( trk );
	SetDescriptionOrig( trk );
	xx->descriptionOff = descriptionOff;
#endif

	if (tempSpecial[0] != '\0') {
		if (strncmp( tempSpecial, ADJUSTABLE, strlen(ADJUSTABLE) ) == 0) {
			xx->special = TOadjustable;
			GetArgs( tempSpecial+strlen(ADJUSTABLE), "ff",
						&xx->u.adjustable.minD, &xx->u.adjustable.maxD );

		} else if (strncmp( tempSpecial, PIER, strlen(PIER) ) == 0) {
			xx->special = TOpier;
			GetArgs( tempSpecial+strlen(PIER), "fq",
						&xx->u.pier.height, &xx->u.pier.name );

		} else {
			InputError("Unknown special case", TRUE);
		}
	}
	if (pathCnt > 0) {
		path = xx->pathCurr;
		while ( position-- ) {
			path += strlen((char *)path)+1;
			while ( path[0] || path[1] )
				path++;
			path += 2;
		if ( *path == 0 )
			path = xx->paths;
		}
	}
	xx->pathCurr = path;

}

void MoveCompound(
		track_p trk,
		coOrd orig )
{
	struct extraData *xx = GetTrkExtraData(trk);
	xx->orig.x += orig.x;
	xx->orig.y += orig.y;
	ComputeCompoundBoundingBox( trk );
}


void RotateCompound(
		track_p trk,
		coOrd orig,
		ANGLE_T angle )
{
	struct extraData *xx = GetTrkExtraData(trk);
	Rotate( &xx->orig, orig, angle );
	xx->angle = NormalizeAngle( xx->angle + angle );
	Rotate( &xx->descriptionOff, zero, angle );
	ComputeCompoundBoundingBox( trk );
}


void RescaleCompound(
		track_p trk,
		FLOAT_T ratio )
{
	struct extraData *xx = GetTrkExtraData(trk);
	xx->orig.x *= ratio;
	xx->orig.y *= ratio;
	xx->descriptionOff.x *= ratio;
	xx->descriptionOff.y *= ratio;
	xx->segs = (trkSeg_p)memdup( xx->segs, xx->segCnt * sizeof xx->segs[0] );
	CloneFilledDraw( xx->segCnt, xx->segs, TRUE );
	RescaleSegs( xx->segCnt, xx->segs, ratio, ratio, ratio );
}


void FlipCompound(
		track_p trk,
		coOrd orig,
		ANGLE_T angle )
{
	struct extraData *xx = GetTrkExtraData(trk);
	EPINX_T ep, epCnt;
	char * mP, *nP, *pP;
	int mL, nL, pL;
	char *type, *mfg, *descL, *partL, *descR, *partR, *cp;
	wIndex_t inx;
	turnoutInfo_t *to, *toBest;
	coOrd endPos[4];
	ANGLE_T endAngle[4];
	DIST_T d2, d1, d0;
	ANGLE_T a2, a1;
#define SMALLVALUE (0.001)

	FlipPoint( &xx->orig, orig, angle );
	xx->angle = NormalizeAngle( 2*angle - xx->angle + 180.0 );
	xx->segs = memdup( xx->segs, xx->segCnt * sizeof xx->segs[0] );
	FlipSegs( xx->segCnt, xx->segs, zero, angle );
	xx->descriptionOrig.y = - xx->descriptionOrig.y;
	ComputeCompoundBoundingBox( trk );
	epCnt = GetTrkEndPtCnt( trk );
	if ( epCnt >= 1 && epCnt <= 2 )
		return;
	ParseCompoundTitle( xtitle(xx), &mP, &mL, &nP, &nL, &pP, &pL );
	to = FindCompound( epCnt==0?FIND_STRUCT:FIND_TURNOUT, GetScaleName(GetTrkScale(trk)), xx->title );
	if ( epCnt!=0 && to && to->customInfo ) {
		if ( GetArgs( to->customInfo, "qc", &type, &cp ) ) {
			if ( strcmp( type, "Regular Turnout" ) == 0 ||
				 strcmp( type, "Curved Turnout" ) == 0 ) {
				if ( GetArgs( cp, "qqqqq", &mfg, &descL, &partL, &descR, &partR ) &&
					 mP && strcmp( mP, mfg ) == 0 && nP && pP ) {
					if ( strcmp( nP, descL ) == 0 && strcmp( pP, partL ) == 0 ) {
						sprintf( message, "%s\t%s\t%s", mfg, descR, partR );
						xx->title = strdup( message );
						return;
					}
					if ( strcmp( nP, descR ) == 0 && strcmp( pP, partR ) == 0 ) {
						sprintf( message, "%s\t%s\t%s", mfg, descL, partL );
						xx->title = strdup( message );
						return;
					}
				}
		   }
		}
	}
	if ( epCnt == 3 || epCnt == 4 ) {
		for ( ep=0; ep<epCnt; ep++ ) {
			endPos[ep] = GetTrkEndPos( trk, ep );
			endAngle[ep] = NormalizeAngle( GetTrkEndAngle( trk, ep ) - xx->angle );
			Rotate( &endPos[ep], xx->orig, -xx->angle );
			endPos[ep].x -= xx->orig.x;
			endPos[ep].y -= xx->orig.y;
		}
		if ( epCnt == 3 ) {
			/* Wye? */
			if ( fabs(endPos[1].x-endPos[2].x) < SMALLVALUE &&
				 fabs(endPos[1].y+endPos[2].y) < SMALLVALUE )
				return;
		} else {
			/* Crossing */
			if ( fabs( (endPos[1].x-endPos[3].x) - (endPos[2].x-endPos[0].x ) ) < SMALLVALUE &&
				 fabs( (endPos[2].y+endPos[3].y) ) < SMALLVALUE &&
				 fabs( (endPos[0].y-endPos[1].y) ) < SMALLVALUE &&
				 NormalizeAngle( (endAngle[2]-endAngle[3]-180+0.05) ) < 0.10 )
				return;
			/* 3 way */
			if ( fabs( (endPos[1].x-endPos[2].x) ) < SMALLVALUE &&
				 fabs( (endPos[1].y+endPos[2].y) ) < SMALLVALUE &&
				 fabs( (endPos[0].y-endPos[3].y) ) < SMALLVALUE &&
				 NormalizeAngle( (endAngle[1]+endAngle[2]-180+0.05) ) < 0.10 )
				return;
		}
		toBest = NULL;
		d0 = 0.0;
		for (inx=0; inx<turnoutInfo_da.cnt; inx++) {
			to = turnoutInfo(inx);
			if ( IsParamValid(to->paramFileIndex) &&
				 to->segCnt > 0 &&
				 to->scaleInx == GetTrkScale(trk) &&
				 to->segCnt != 0 &&
				 to->endCnt == epCnt ) {
				d1 = 0;
				a1 = 0;
				for ( ep=0; ep<epCnt; ep++ ) {
					d2 = FindDistance( endPos[ep], to->endPt[ep].pos );
					if ( d2 > SMALLVALUE )
						break;
					if ( d2 > d1 )
						d1 = d2;
					a2 = NormalizeAngle( endAngle[ep] - to->endPt[ep].angle + 0.05 );
					if ( a2 > 0.1 )
						break;
					if ( a2 > a1 )
						a1 = a2;
				}
				if ( ep<epCnt )
					continue;
				if ( toBest == NULL || d1 < d0 )
					toBest = to;
			}
		}
		if ( toBest ) {
			if ( strcmp( xx->title, toBest->title ) != 0 )
				xx->title = MyStrdup( toBest->title );
			return;
		}
	}
	xx->flipped = !xx->flipped;
}


typedef struct {
		long count;
		char * type;
		char * name;
		FLOAT_T price;
		} enumCompound_t;
static dynArr_t enumCompound_da;
#define EnumCompound(N) DYNARR_N( enumCompound_t,enumCompound_da,N)

BOOL_T EnumerateCompound( track_p trk )
{
	struct extraData *xx;
	INT_T inx, inx2;
	int cmp;
	long listLabelsOption = listLabels;

	if ( trk != NULL ) {
		xx = GetTrkExtraData(trk);
		if ( xx->flipped )
			listLabelsOption |= LABEL_FLIPPED;
#ifdef LATER
		if ( xx->ungrouped )
			listLabelsOption |= LABEL_UNGROUPED;
		if ( xx->split )
			listLabelsOption |= LABEL_SPLIT;
#endif
		FormatCompoundTitle( listLabelsOption, xtitle(xx) );
		if (message[0] == '\0')
			return TRUE;
		for (inx = 0; inx < enumCompound_da.cnt; inx++ ) {
			cmp =  strcmp( EnumCompound(inx).name, message );
			if ( cmp == 0 ) {
				EnumCompound(inx).count++;
				return TRUE;
			} else if ( cmp > 0 ) {
				break;
			}
		}
		DYNARR_APPEND( enumCompound_t, enumCompound_da, 10 );
		for ( inx2 = enumCompound_da.cnt-1; inx2 > inx; inx2-- )
			EnumCompound(inx2) = EnumCompound(inx2-1);
		EnumCompound(inx).name = MyStrdup( message );
		if (strlen(message) > (size_t)enumerateMaxDescLen)
			enumerateMaxDescLen = strlen(message);
		EnumCompound(inx).type = GetTrkTypeName( trk );
		EnumCompound(inx).count = 1;
		FormatCompoundTitle( LABEL_MANUF|LABEL_DESCR|LABEL_PARTNO, xtitle(xx) );
		wPrefGetFloat( "price list", message, &(EnumCompound(inx).price), 0.0 );
	} else {
		char * type;
		for ( type="TS"; *type; type++ ) {
			for (inx = 0; inx < enumCompound_da.cnt; inx++ ) {
				if (EnumCompound(inx).type[0] == *type) {
					EnumerateList( EnumCompound(inx).count,
						EnumCompound(inx).price,
						EnumCompound(inx).name );
				}
			}
		}
		DYNARR_RESET( enumCompound_t, enumCompound_da );
	}
	return TRUE;
}

