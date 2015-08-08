/*
 * $Header: /home/dmarkle/xtrkcad-fork-cvs/xtrkcad/app/bin/cdraw.c,v 1.4 2008-03-06 19:35:04 m_fischer Exp $
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
#include "ccurve.h"
#include "drawgeom.h"
#include "i18n.h"

#include <stdint.h>

extern void wSetSelectedFontSize(int size);

static long fontSizeList[] = {
		4, 5, 6, 7, 8, 10, 12, 14, 16, 18, 20, 24, 28, 32, 36,
		40, 48, 56, 64, 72, 80, 90, 100, 120, 140, 160, 180,
		200, 250, 300, 350, 400, 450, 500 };

EXPORT void LoadFontSizeList(
		wList_p list,
		long curFontSize )
{
	wIndex_t curInx=0, inx1;
	int inx;
	wListClear( list );
	for ( inx=0; inx<sizeof fontSizeList/sizeof fontSizeList[0]; inx++ ) {
		if ( ( inx==0 || curFontSize > fontSizeList[inx-1] ) && 
			 ( curFontSize < fontSizeList[inx] ) ) {
			sprintf( message, "%ld", curFontSize );
			curInx = wListAddValue( list, message, NULL, (void*)curFontSize );
		}
		sprintf( message, "%ld", fontSizeList[inx] );
		inx1 = wListAddValue( list, message, NULL, (void*)fontSizeList[inx] );
		if ( curFontSize == fontSizeList[inx] )
			curInx = inx1;
	}
	if ( curFontSize > fontSizeList[(sizeof fontSizeList/sizeof fontSizeList[0])-1] ) {
		sprintf( message, "%ld", curFontSize );
		curInx = wListAddValue( list, message, NULL, (void*)curFontSize );
	}
	wListSetIndex( list, curInx );
	wFlush();
}


EXPORT void UpdateFontSizeList(
		long * fontSizeR,
		wList_p list,
		wIndex_t listInx )
{
	long fontSize;

	if ( listInx >= 0 ) {
		*fontSizeR = (long)wListGetItemContext( list, listInx );
	} else {
		wListGetValues( list, message, sizeof message, NULL, NULL );
		if ( message[0] != '\0' ) {
			fontSize = atol( message );
			if ( fontSize <= 0 ) {
				NoticeMessage( _("Font Size must be > 0"), _("Ok"), NULL );
				sprintf( message, "%ld", *fontSizeR );
				wListSetValue( list, message );
			} else {
				if ( fontSize <= 500 || NoticeMessage( MSG_LARGE_FONT, _("Yes"), _("No") ) > 0 ) {
				
					*fontSizeR = fontSize;
					/* inform gtkfont dialog from change */
					wSetSelectedFontSize((int)fontSize);
					/*LoadFontSizeList( list, *fontSizeR );*/
				} else {
					sprintf( message, "%ld", *fontSizeR );
					wListSetValue( list, message );
				}
			}
		}
	}
}

/*******************************************************************************
 *
 * DRAW
 *
 */

struct extraData {
		coOrd orig;
		ANGLE_T angle;
		wIndex_t segCnt;
		trkSeg_t segs[1];
		};

static TRKTYP_T T_DRAW = -1;
static track_p ignoredTableEdge;
static track_p ignoredDraw;


static void ComputeDrawBoundingBox( track_p t )
{
	struct extraData * xx = GetTrkExtraData(t);
	coOrd lo, hi;

	GetSegBounds( xx->orig, xx->angle, xx->segCnt, xx->segs, &lo, &hi );
	hi.x += lo.x;
	hi.y += lo.y;
	SetBoundingBox( t, hi, lo );
}


static track_p MakeDrawFromSeg1(
		wIndex_t index,
		coOrd pos,
		ANGLE_T angle,
		trkSeg_p sp )
{
	struct extraData * xx;
	track_p trk;
	if ( sp->type == ' ' )
		return NULL;
	trk = NewTrack( index, T_DRAW, 0, sizeof *xx );
	xx = GetTrkExtraData( trk );
	xx->orig = pos;
	xx->angle = angle;
	xx->segCnt = 1;
	memcpy( xx->segs, sp, sizeof *(trkSeg_p)0 );
	ComputeDrawBoundingBox( trk );
	return trk;
}

EXPORT track_p MakeDrawFromSeg(
		coOrd pos,
		ANGLE_T angle,
		trkSeg_p sp )
{
	return MakeDrawFromSeg1( 0, pos, angle, sp );
}




static DIST_T DistanceDraw( track_p t, coOrd * p )
{
	struct extraData * xx = GetTrkExtraData(t);
	if ( ignoredTableEdge == t && xx->segs[0].type == SEG_TBLEDGE )
		return 100000.0;
	if ( ignoredDraw == t )
		return 100000.0;
	return DistanceSegs( xx->orig, xx->angle, xx->segCnt, xx->segs, p, NULL );
}


static struct {
		coOrd endPt[4];
		FLOAT_T length;
		coOrd center;
		DIST_T radius;
        DIST_T min_radius;
		ANGLE_T angle0;
		ANGLE_T angle1;
		ANGLE_T angle;
		long pointCount;
		long lineWidth;
		wDrawColor color;
		wIndex_t benchChoice;
		wIndex_t benchOrient;
		wIndex_t dimenSize;
		descPivot_t pivot;
		wIndex_t fontSizeInx;
		char text[STR_SIZE];
		} drawData;
typedef enum { E0, E1, C1, C2, CE, RA, LN, AL, MR, A1, A2, VC, LW, CO, BE, OR, DS, TP, TA, TS, TX, PV, LY } drawDesc_e;
static descData_t drawDesc[] = {
/*E0*/	{ DESC_POS, N_("End Pt 1: X"), &drawData.endPt[0] },
/*E1*/	{ DESC_POS, N_("End Pt 2: X"), &drawData.endPt[1] },
/*C1*/  { DESC_POS, N_("Ctrl Pt 1: X"), &drawData.endPt[2] },
/*C2*/  { DESC_POS, N_("Ctrl Pt 2: X"), &drawData.endPt[3] },
/*CE*/	{ DESC_POS, N_("Center: X"), &drawData.center },
/*RA*/	{ DESC_DIM, N_("Radius"), &drawData.radius },
/*LN*/	{ DESC_DIM, N_("Length"), &drawData.length },
/*AL*/	{ DESC_FLOAT, N_("Angle"), &drawData.angle },
/*MR*/  { DESC_DIM, N_("Min Radius"), &drawData.min_radius },
/*A1*/	{ DESC_ANGLE, N_("CCW Angle"), &drawData.angle0 },
/*A2*/	{ DESC_ANGLE, N_("CW Angle"), &drawData.angle1 },
/*VC*/	{ DESC_LONG, N_("Point Count"), &drawData.pointCount },
/*LW*/	{ DESC_LONG, N_("Line Width"), &drawData.lineWidth },
/*CO*/	{ DESC_COLOR, N_("Color"), &drawData.color },
/*BE*/	{ DESC_LIST, N_("Lumber"), &drawData.benchChoice },
/*OR*/	{ DESC_LIST, N_("Orientation"), &drawData.benchOrient },
/*DS*/	{ DESC_LIST, N_("Size"), &drawData.dimenSize },
/*TP*/	{ DESC_POS, N_("Origin: X"), &drawData.endPt[0] },
/*TA*/	{ DESC_FLOAT, N_("Angle"), &drawData.angle },
/*TS*/	{ DESC_EDITABLELIST, N_("Font Size"), &drawData.fontSizeInx },
/*TX*/	{ DESC_STRING, N_("Text"), &drawData.text },
/*PV*/	{ DESC_PIVOT, N_("Pivot"), &drawData.pivot },
/*LY*/	{ DESC_LAYER, N_("Layer"), NULL },
		{ DESC_NULL } };
int drawSegInx;

#define UNREORIGIN( Q, P, A, O ) { \
		(Q) = (P); \
		(Q).x -= (O).x; \
		(Q).y -= (O).y; \
		if ( (A) != 0.0 ) \
			Rotate( &(Q), zero, -(A) ); \
		}

static void UpdateDraw( track_p trk, int inx, descData_p descUpd, BOOL_T final )
{
	struct extraData *xx = GetTrkExtraData(trk);
	trkSeg_p segPtr;
	coOrd mid;
	const char * text;
	long fontSize;

	if ( drawSegInx==-1 )
		return;
	if ( inx == -1 )
		return;
	segPtr = &xx->segs[drawSegInx];
    MainRedraw();
	//UndrawNewTrack( trk );
	switch ( inx ) {
	case LW:
		segPtr->width = drawData.lineWidth/mainD.dpi;
		break;
	case CO:
		segPtr->color = drawData.color;
		break;
	case E0:
	case E1:
    case C1:
    case C2:
		if ( inx == E0 ) {
			UNREORIGIN( segPtr->u.l.pos[0], drawData.endPt[0], xx->angle, xx->orig );
		} else if (inx == E1 ) {
			UNREORIGIN( segPtr->u.l.pos[1], drawData.endPt[1], xx->angle, xx->orig );
        } else if (inx ==C1 ) {
            UNREORIGIN( segPtr->u.b.pos[2], drawData.endPt[2], xx->angle, xx->orig );
        } else {
            UNREORIGIN( segPtr->u.b.pos[3], drawData.endPt[3], xx->angle, xx->orig );
        }
		drawData.length = FindDistance( drawData.endPt[0], drawData.endPt[1] );
		drawData.angle = FindAngle( drawData.endPt[0], drawData.endPt[1] );
		drawDesc[LN].mode |= DESC_CHANGE;
		drawDesc[AL].mode |= DESC_CHANGE;
		break;
    case MR:
            if (segPtr->type == SEG_BZRLIN || segPtr->type == SEG_BZRTRK) {
                drawData.min_radius = BezierMinRadius(segPtr->u.b.pos[0],segPtr->u.b.pos[1],segPtr->u.b.pos[2],segPtr->u.b.pos[3]);
            } else {
                drawData.min_radius = 0.0;
            }
            drawDesc[MR].mode |= DESC_CHANGE;
        break;
	case LN:
	case AL:
		if ( segPtr->type == SEG_CRVLIN && inx == AL ) {
			if ( drawData.angle <= 0.0 || drawData.angle >= 360.0 ) {
				ErrorMessage( MSG_CURVE_OUT_OF_RANGE );
				drawData.angle = segPtr->u.c.a1;
				drawDesc[AL].mode |= DESC_CHANGE;
				break;
			}
		} else {
			if ( drawData.length <= minLength ) {
				ErrorMessage( MSG_OBJECT_TOO_SHORT );
				if ( segPtr->type != SEG_CRVLIN ) {
					drawData.length = FindDistance( drawData.endPt[0], drawData.endPt[1] );
				} else {
					drawData.length = segPtr->u.c.radius*2*M_PI*segPtr->u.c.a1/360.0;
				}
				drawDesc[LN].mode |= DESC_CHANGE;
				break;
			}
		}
		if ( segPtr->type != SEG_CRVLIN ) {
			switch ( drawData.pivot ) {
			case DESC_PIVOT_FIRST:
				Translate( &drawData.endPt[1], drawData.endPt[0], drawData.angle, drawData.length );
				UNREORIGIN( segPtr->u.l.pos[1], drawData.endPt[1], xx->angle, xx->orig );
				drawDesc[E1].mode |= DESC_CHANGE;
				break;
			case DESC_PIVOT_SECOND:
				Translate( &drawData.endPt[0], drawData.endPt[1], drawData.angle+180.0, drawData.length );
				UNREORIGIN( segPtr->u.l.pos[0], drawData.endPt[0], xx->angle, xx->orig );
				drawDesc[E0].mode |= DESC_CHANGE;
				break;
			case DESC_PIVOT_MID:
				mid.x = (drawData.endPt[0].x+drawData.endPt[1].x)/2.0;
				mid.y = (drawData.endPt[0].y+drawData.endPt[1].y)/2.0;
				Translate( &drawData.endPt[0], mid, drawData.angle+180.0, drawData.length/2.0 );
				Translate( &drawData.endPt[1], mid, drawData.angle, drawData.length/2.0 );
				UNREORIGIN( segPtr->u.l.pos[0], drawData.endPt[0], xx->angle, xx->orig );
				UNREORIGIN( segPtr->u.l.pos[1], drawData.endPt[1], xx->angle, xx->orig );
				drawDesc[E0].mode |= DESC_CHANGE;
				drawDesc[E1].mode |= DESC_CHANGE;
				break;
			default:
				break;
			}
		} else {
			if ( drawData.angle < 0.0 || drawData.angle >= 360.0 ) {
				ErrorMessage( MSG_CURVE_OUT_OF_RANGE );
				drawData.angle = segPtr->u.c.a1;
				drawDesc[AL].mode |= DESC_CHANGE;
			} else {
				segPtr->u.c.a0 = NormalizeAngle( segPtr->u.c.a0+segPtr->u.c.a1/2.0-drawData.angle/2.0);
				segPtr->u.c.a1 = drawData.angle;
				drawData.angle0 = NormalizeAngle( segPtr->u.c.a0+xx->angle );
				drawData.angle1 = NormalizeAngle( drawData.angle0+segPtr->u.c.a1 );
				drawDesc[A1].mode |= DESC_CHANGE;
				drawDesc[A2].mode |= DESC_CHANGE;
			}
		}
		break;
	case CE:
		UNREORIGIN( segPtr->u.c.center, drawData.center, xx->angle, xx->orig );
		break;
	case RA:
		segPtr->u.c.radius = drawData.radius;
		break;
	case A1:
		segPtr->u.c.a0 = NormalizeAngle( drawData.angle0-xx->angle );
		drawData.angle1 = NormalizeAngle( drawData.angle0+drawData.angle );
		drawDesc[A2].mode |= DESC_CHANGE;
		break;
	case A2:
		segPtr->u.c.a0 = NormalizeAngle( drawData.angle1-segPtr->u.c.a1-xx->angle );
		drawData.angle0 = NormalizeAngle( segPtr->u.c.a0+xx->angle );
		drawDesc[A1].mode |= DESC_CHANGE;
		break;
	case BE:
		BenchUpdateOrientationList( (long)wListGetItemContext((wList_p)drawDesc[BE].control0, drawData.benchChoice ), (wList_p)drawDesc[OR].control0 );
		if ( drawData.benchOrient < wListGetCount( (wList_p)drawDesc[OR].control0 ) )
			wListSetIndex( (wList_p)drawDesc[OR].control0, drawData.benchOrient );
		else
			drawData.benchOrient = 0;
		segPtr->u.l.option = GetBenchData( (long)wListGetItemContext((wList_p)drawDesc[BE].control0, drawData.benchChoice ), drawData.benchOrient );
		break;
	case OR:
		segPtr->u.l.option = GetBenchData( (long)wListGetItemContext((wList_p)drawDesc[BE].control0, drawData.benchChoice ), drawData.benchOrient );
		break;
	case DS:
		segPtr->u.l.option = drawData.dimenSize;
		break;
	case TP:
		UNREORIGIN( segPtr->u.t.pos, drawData.endPt[0], xx->angle, xx->orig );
		break;
	case TA:
		//segPtr->u.t.angle = NormalizeAngle( drawData.angle );
		xx->angle = NormalizeAngle( drawData.angle );
		break;
	case TS:
		fontSize = (long)segPtr->u.t.fontSize;
		UpdateFontSizeList( &fontSize, (wList_p)drawDesc[TS].control0, drawData.fontSizeInx );
		segPtr->u.t.fontSize = fontSize;
		break;
	case TX:
		text = wStringGetValue( (wString_p)drawDesc[TX].control0 );
		if ( text && text[0] && strcmp( segPtr->u.t.string, text ) != 0 ) {
			MyFree( segPtr->u.t.string );
			segPtr->u.t.string = MyStrdup( text );
			/*(char*)drawDesc[TX].valueP = segPtr->u.t.string;*/
		}
		break;
	default:
		AbortProg( "bad op" );
	}
	ComputeDrawBoundingBox( trk );
	DrawNewTrack( trk );
}

static void DescribeDraw( track_p trk, char * str, CSIZE_T len )
{
	struct extraData *xx = GetTrkExtraData(trk);
	coOrd pos = oldMarker;
	trkSeg_p segPtr;
	int inx;
	char * title = NULL;


	DistanceSegs( xx->orig, xx->angle, xx->segCnt, xx->segs, &pos, &drawSegInx );
	if ( drawSegInx==-1 )
		return;
	segPtr = &xx->segs[drawSegInx];
	for ( inx=0; inx<sizeof drawDesc/sizeof drawDesc[0]; inx++ ) {
		drawDesc[inx].mode = DESC_IGNORE;
		drawDesc[inx].control0 = NULL;
	}
	drawData.color = segPtr->color;
	drawDesc[CO].mode = 0;
	drawData.lineWidth = (long)floor(segPtr->width*mainD.dpi+0.5);
	drawDesc[LW].mode = 0;
	drawDesc[LY].mode = DESC_RO;
	drawDesc[BE].mode =
	drawDesc[OR].mode =
	drawDesc[DS].mode = DESC_IGNORE;
	drawData.pivot = DESC_PIVOT_MID;
	switch ( segPtr->type ) {
	case SEG_STRLIN:
	case SEG_DIMLIN:
	case SEG_BENCH:
	case SEG_TBLEDGE:
		REORIGIN( drawData.endPt[0], segPtr->u.l.pos[0], xx->angle, xx->orig );
		REORIGIN( drawData.endPt[1], segPtr->u.l.pos[1], xx->angle, xx->orig );
		drawData.length = FindDistance( drawData.endPt[0], drawData.endPt[1] );
		drawData.angle = FindAngle( drawData.endPt[0], drawData.endPt[1] );
		drawDesc[LN].mode =
		drawDesc[AL].mode =
		drawDesc[PV].mode = 0;
		drawDesc[E0].mode =
		drawDesc[E1].mode = 0;
		switch (segPtr->type) {
		case SEG_STRLIN:
			title = _("Straight Line");
			break;
		case SEG_DIMLIN:
			title = _("Dimension Line");
			drawDesc[CO].mode = DESC_IGNORE;
			drawDesc[LW].mode = DESC_IGNORE;
			drawData.dimenSize = (wIndex_t)segPtr->u.l.option;
			drawDesc[DS].mode = 0;
			break;
		case SEG_BENCH:
			title = _("Lumber");
			drawDesc[LW].mode = DESC_IGNORE;
			drawDesc[BE].mode =
			drawDesc[OR].mode = 0;
			drawData.benchChoice = GetBenchListIndex( segPtr->u.l.option );
			drawData.benchOrient = (wIndex_t)(segPtr->u.l.option&0xFF);
			break;
		case SEG_TBLEDGE:
			title = _("Table Edge");
			drawDesc[CO].mode = DESC_IGNORE;
			drawDesc[LW].mode = DESC_IGNORE;
			break;
		}
		break;
	case SEG_CRVLIN:
		REORIGIN( drawData.center, segPtr->u.c.center, xx->angle, xx->orig );
		drawData.radius = segPtr->u.c.radius;
		drawDesc[CE].mode =
		drawDesc[RA].mode = 0;
		if ( segPtr->u.c.a1 >= 360.0 ) {
			title = _("Circle");
		} else {
			drawData.angle = segPtr->u.c.a1;
			drawData.angle0 = NormalizeAngle( segPtr->u.c.a0+xx->angle );
			drawData.angle1 = NormalizeAngle( drawData.angle0+drawData.angle );
			drawDesc[AL].mode =
			drawDesc[A1].mode =
			drawDesc[A2].mode = 0;
			title = _("Curved Line");
		}
		break;
    case SEG_BZRTRK:
    case SEG_BZRLIN:
        REORIGIN( drawData.endPt[0], segPtr->u.b.pos[0], xx->angle, xx->orig );
        REORIGIN( drawData.endPt[1], segPtr->u.b.pos[1], xx->angle, xx->orig );
        REORIGIN( drawData.endPt[2], segPtr->u.b.pos[2], xx->angle, xx->orig );
        REORIGIN( drawData.endPt[3], segPtr->u.b.pos[3], xx->angle, xx->orig );
        drawDesc[E0].mode =
        drawDesc[E1].mode =
        drawDesc[C1].mode =
        drawDesc[C2].mode = 0;
        break;
	case SEG_FILCRCL:
		REORIGIN( drawData.center, segPtr->u.c.center, xx->angle, xx->orig );
		drawData.radius = segPtr->u.c.radius;
		drawDesc[CE].mode =
		drawDesc[RA].mode = 0;
		drawDesc[LW].mode = DESC_IGNORE;
		title = _("Filled Circle");
		break;
	case SEG_POLY:
		drawData.pointCount = segPtr->u.p.cnt;
		drawDesc[VC].mode = DESC_RO;
		title = _("Poly Line");
		break;
	case SEG_FILPOLY:
		drawData.pointCount = segPtr->u.p.cnt;
		drawDesc[VC].mode = DESC_RO;
		drawDesc[LW].mode = DESC_IGNORE;
		title = _("Polygon");
		break;
	case SEG_TEXT:
		REORIGIN( drawData.endPt[0], segPtr->u.t.pos, xx->angle, xx->orig );
		//drawData.angle = NormalizeAngle( segPtr->u.t.angle );
		drawData.angle = NormalizeAngle( xx->angle );
		strncpy( drawData.text, segPtr->u.t.string, sizeof drawData.text );
		/*drawData.fontSize = segPtr->u.t.fontSize;*/
		/*(char*)drawDesc[TX].valueP = segPtr->u.t.string;*/
		drawDesc[TP].mode =
		drawDesc[TS].mode =
		drawDesc[TX].mode = 
		drawDesc[TA].mode = 
        drawDesc[CO].mode = 0;  /*Allow Text color setting*/
		drawDesc[LW].mode = DESC_IGNORE;
		title = _("Text");
		break;
	default:
		AbortProg( "bad seg type" );
	}

	sprintf( str, _("%s: Layer=%d"), title, GetTrkLayer(trk)+1 );

	DoDescribe( title, trk, drawDesc, UpdateDraw );
	if ( segPtr->type==SEG_BENCH && drawDesc[BE].control0!=NULL && drawDesc[OR].control0!=NULL) {
		BenchLoadLists( (wList_p)drawDesc[BE].control0, (wList_p)drawDesc[OR].control0 );
		wListSetIndex( (wList_p)drawDesc[BE].control0, drawData.benchChoice );
		BenchUpdateOrientationList( (long)wListGetItemContext((wList_p)drawDesc[BE].control0, drawData.benchChoice ), (wList_p)drawDesc[OR].control0 );
		wListSetIndex( (wList_p)drawDesc[OR].control0, drawData.benchOrient );
	}
	if ( segPtr->type==SEG_DIMLIN && drawDesc[DS].control0!=NULL ) {
		wListClear( (wList_p)drawDesc[DS].control0 );
		wListAddValue( (wList_p)drawDesc[DS].control0, _("Tiny"), NULL, (void*)0 );
		wListAddValue( (wList_p)drawDesc[DS].control0, _("Small"), NULL, (void*)1 );
		wListAddValue( (wList_p)drawDesc[DS].control0, _("Medium"), NULL, (void*)2 );
		wListAddValue( (wList_p)drawDesc[DS].control0, _("Large"), NULL, (void*)3 );
		wListSetIndex( (wList_p)drawDesc[DS].control0, drawData.dimenSize );
	}
	if ( segPtr->type==SEG_TEXT && drawDesc[TS].control0!=NULL ) {
		LoadFontSizeList( (wList_p)drawDesc[TS].control0, (long)segPtr->u.t.fontSize );
	}
}


static void DrawDraw( track_p t, drawCmd_p d, wDrawColor color )
{
	struct extraData * xx = GetTrkExtraData(t);
	if ( (d->options&DC_QUICK) == 0 )
		DrawSegs( d, xx->orig, xx->angle, xx->segs, xx->segCnt, 0.0, color );
}


static void DeleteDraw( track_p t )
{
}


static BOOL_T WriteDraw( track_p t, FILE * f )
{
	struct extraData * xx = GetTrkExtraData(t);
	BOOL_T rc = TRUE;
	rc &= fprintf(f, "DRAW %d %d 0 0 0 %0.6f %0.6f 0 %0.6f\n", GetTrkIndex(t), GetTrkLayer(t),
				xx->orig.x, xx->orig.y, xx->angle )>0;
	rc &= WriteSegs( f, xx->segCnt, xx->segs );
	return rc;
}


static void ReadDraw( char * header )
{
	track_p trk;
	wIndex_t index;
	coOrd orig;
	DIST_T elev;
	ANGLE_T angle;
	wIndex_t layer;
	struct extraData * xx;

	if ( !GetArgs( header+5, paramVersion<3?"dXpYf":paramVersion<9?"dL000pYf":"dL000pff",
				&index, &layer, &orig, &elev, &angle ) )
		return;
	ReadSegs();
	if (tempSegs_da.cnt == 1) {
		trk = MakeDrawFromSeg1( index, orig, angle, &tempSegs(0) );
		SetTrkLayer( trk, layer );
	} else {
		trk = NewTrack( index, T_DRAW, 0, sizeof *xx + (tempSegs_da.cnt-1) * sizeof *(trkSeg_p)0 );
		SetTrkLayer( trk, layer );
		xx = GetTrkExtraData(trk);
		xx->orig = orig;
		xx->angle = angle;
		xx->segCnt = tempSegs_da.cnt;
		memcpy( xx->segs, tempSegs_da.ptr, tempSegs_da.cnt * sizeof *(trkSeg_p)0 );
		ComputeDrawBoundingBox( trk );
	}
}


static void MoveDraw( track_p trk, coOrd orig )
{
	struct extraData * xx = GetTrkExtraData(trk);
	xx->orig.x += orig.x;
	xx->orig.y += orig.y;
	ComputeDrawBoundingBox( trk );
}


static void RotateDraw( track_p trk, coOrd orig, ANGLE_T angle )
{
	struct extraData * xx = GetTrkExtraData(trk);
	Rotate( &xx->orig, orig, angle );
	xx->angle = NormalizeAngle( xx->angle + angle );
	ComputeDrawBoundingBox( trk );
}


static void RescaleDraw( track_p trk, FLOAT_T ratio )
{
	struct extraData * xx = GetTrkExtraData(trk);
	xx->orig.x *= ratio;
	xx->orig.y *= ratio;
	RescaleSegs( xx->segCnt, xx->segs, ratio, ratio, ratio );
}


static STATUS_T ModifyDraw( track_p trk, wAction_t action, coOrd pos )
{
	struct extraData * xx = GetTrkExtraData(trk);
	STATUS_T rc;

	if (action == C_DOWN) {
		//UndrawNewTrack( trk );
	}
	if ( action == C_MOVE )
		ignoredDraw = trk;
	rc = DrawGeomModify( xx->orig, xx->angle, xx->segCnt, xx->segs, action, pos, GetTrkSelected(trk) );
	ignoredDraw = NULL;
	if (action == C_UP) {
		ComputeDrawBoundingBox( trk );
		DrawNewTrack( trk );
	}
	return rc;
}


static void UngroupDraw( track_p trk )
{
	struct extraData * xx = GetTrkExtraData(trk);
	int inx;
	if ( xx->segCnt <= 1 )
		return;
	DeleteTrack( trk, FALSE );
	for ( inx=0; inx<xx->segCnt; inx++ ) {
		trk = MakeDrawFromSeg( xx->orig, xx->angle, &xx->segs[inx] );
		if ( trk ) {
			SetTrkBits( trk, TB_SELECTED );
			DrawNewTrack( trk );
		}
	}
}


static ANGLE_T GetAngleDraw(
		track_p trk,
		coOrd pos,
		EPINX_T * ep0,
		EPINX_T * ep1 )
{
	struct extraData * xx = GetTrkExtraData(trk);
	ANGLE_T angle;

	pos.x -= xx->orig.x;
	pos.y -= xx->orig.y;
	Rotate( &pos, zero, -xx->angle );
	angle = GetAngleSegs( xx->segCnt, xx->segs, pos, NULL );
	if ( ep0 ) *ep0 = -1;
	if ( ep1 ) *ep1 = -1;
	return NormalizeAngle( angle + xx->angle );
}



static BOOL_T EnumerateDraw(
		track_p trk )
{
	struct extraData * xx;
	int inx;
	trkSeg_p segPtr;

	if ( trk ) {
		xx = GetTrkExtraData(trk);
		if ( xx->segCnt < 1 )
			return TRUE;
		for ( inx=0; inx<xx->segCnt; inx++ ) {
			segPtr = &xx->segs[inx];
			if ( segPtr->type == SEG_BENCH ) {
				CountBench( segPtr->u.l.option, FindDistance( segPtr->u.l.pos[0], segPtr->u.l.pos[1] ) );
			}
		}
	} else {
		TotalBench();
	}
	return TRUE;
}


static void FlipDraw(
		track_p trk,
		coOrd orig,
		ANGLE_T angle )
{
	struct extraData * xx = GetTrkExtraData(trk);
	FlipPoint( &xx->orig, orig, angle );
	xx->angle = NormalizeAngle( 2*angle - xx->angle + 180.0 );
	FlipSegs( xx->segCnt, xx->segs, zero, angle );
	ComputeDrawBoundingBox( trk );
}


static trackCmd_t drawCmds = {
		"DRAW",
		DrawDraw,
		DistanceDraw,
		DescribeDraw,
		DeleteDraw,
		WriteDraw,
		ReadDraw,
		MoveDraw,
		RotateDraw,
		RescaleDraw,
		NULL,
		GetAngleDraw, /* getAngle */
		NULL, /* split */
		NULL, /* traverse */
		EnumerateDraw,
		NULL, /* redraw */
		NULL, /* trim */
		NULL, /* merge */
		ModifyDraw,
		NULL, /* getLength */
		NULL, /* getTrackParams */
		NULL, /* moveEndPt */
		NULL, /* query */
		UngroupDraw,
		FlipDraw };

EXPORT BOOL_T OnTableEdgeEndPt( track_p trk, coOrd * pos )
{
	track_p trk1;
	struct extraData *xx;
	coOrd pos1 = *pos;

	ignoredTableEdge = trk;
	if ((trk1 = OnTrack( &pos1, FALSE, FALSE )) != NULL &&
		 GetTrkType(trk1) == T_DRAW) {
		ignoredTableEdge = NULL;
		xx = GetTrkExtraData(trk1);
		if (xx->segCnt < 1)
			return FALSE;
		if (xx->segs[0].type == SEG_TBLEDGE) {
			if ( IsClose( FindDistance( *pos, xx->segs[0].u.l.pos[0] ) ) ) {
				*pos = xx->segs[0].u.l.pos[0];
				return TRUE;
			} else if ( IsClose( FindDistance( *pos, xx->segs[0].u.l.pos[1] ) ) ) {
				*pos = xx->segs[0].u.l.pos[1];
				return TRUE;
			}
		}
	}
	ignoredTableEdge = NULL;
	return FALSE;
}




static void DrawRedraw(void);
static drawContext_t drawCmdContext = {
		InfoMessage,
		DrawRedraw,
		&mainD,
		OP_LINE };

static void DrawRedraw( void )
{
	MainRedraw();
}


#ifdef LATER
static void DrawOk( void * context )
{
	track_p t;
	struct extraData * xx;
	trkSeg_p sp;
	wIndex_t cnt;

	for ( cnt=0,sp=&DrawLineSegs(0); sp < &DrawLineSegs(drawCmdContext.Segs_da.cnt); sp++ )
		if (sp->type != ' ')
			cnt++;
	if (cnt == 0)
		return;
	UndoStart( _("Create Lines"), "newDraw" );
	for ( sp=&DrawLineSegs(0); sp < &DrawLineSegs(drawCmdContext.Segs_da.cnt); sp++ ) {
		if (sp->type != ' ') {
			t = NewTrack( 0, T_DRAW, 0, sizeof *xx + sizeof *(trkSeg_p)0 );
			xx = GetTrkExtraData( t );
			xx->orig = zero;
			xx->angle = 0.0;
			xx->segCnt = 1;
			memcpy( xx->segs, sp, sizeof *(trkSeg_p)0 );
			ComputeDrawBoundingBox( t );
			DrawNewTrack(t);
		}
	}
	UndoEnd();
	DYNARR_RESET( trkSeg_t, drawCmdContext.Segs_da );
	Reset();
}
#endif



static wIndex_t benchChoice;
static wIndex_t benchOrient;
static wIndex_t dimArrowSize;
static wDrawColor lineColor;
static wDrawColor benchColor;
#ifdef LATER
static wIndex_t benchInx;
#endif

static paramIntegerRange_t i0_100 = { 0, 100, 25 };
static paramData_t drawPLs[] = {
#define drawWidthPD				(drawPLs[0])
	{ PD_LONG, &drawCmdContext.Width, "linewidth", PDO_NORECORD, &i0_100, N_("Line Width") }, 
#define drawColorPD				(drawPLs[1])
	{ PD_COLORLIST, &lineColor, "linecolor", PDO_NORECORD, NULL, N_("Color") },
#define drawBenchColorPD		(drawPLs[2])
	{ PD_COLORLIST, &benchColor, "benchcolor", PDO_NORECORD, NULL, N_("Color") },
#define drawBenchChoicePD		(drawPLs[3])
	{ PD_DROPLIST, &benchChoice, "benchlist", PDO_NOPREF|PDO_NORECORD|PDO_LISTINDEX, (void*)80, N_("Lumber Type") },
#define drawBenchOrientPD		(drawPLs[4])
#ifdef WINDOWS
	{ PD_DROPLIST, &benchOrient, "benchorient", PDO_NOPREF|PDO_NORECORD|PDO_LISTINDEX, (void*)45, "", 0 },
#else
	{ PD_DROPLIST, &benchOrient, "benchorient", PDO_NOPREF|PDO_NORECORD|PDO_LISTINDEX, (void*)105, "", 0 },
#endif
#define drawDimArrowSizePD		(drawPLs[5])
	{ PD_DROPLIST, &dimArrowSize, "arrowsize", PDO_NORECORD|PDO_LISTINDEX, (void*)80, N_("Size") } };
static paramGroup_t drawPG = { "draw", 0, drawPLs, sizeof drawPLs/sizeof drawPLs[0] };

static char * objectName[] = {
		N_("Straight"),
		N_("Dimension"),
		N_("Lumber"),
		N_("Table Edge"),
		N_("Curved"),
		N_("Curved"),
		N_("Curved"),
		N_("Curved"),
		N_("Circle"),
		N_("Circle"),
		N_("Circle"),
		N_("Box"),
		N_("Polyline"),
		N_("Filled Circle"),
		N_("Filled Circle"),
		N_("Filled Circle"),
		N_("Filled Box"),
		N_("Polygon"),
        N_("Bezier"),
		NULL};

static STATUS_T CmdDraw( wAction_t action, coOrd pos )

{
	static BOOL_T infoSubst = FALSE;
	wControl_p controls[4];
	char * labels[3];
	static char labelName[40];

	switch (action&0xFF) {

	case C_START:
		ParamLoadControls( &drawPG );
		/*drawContext = &drawCmdContext;*/
		drawWidthPD.option |= PDO_NORECORD;
		drawColorPD.option |= PDO_NORECORD;
		drawBenchColorPD.option |= PDO_NORECORD;
		drawBenchChoicePD.option |= PDO_NORECORD;
		drawBenchOrientPD.option |= PDO_NORECORD;
		drawDimArrowSizePD.option |= PDO_NORECORD;
		drawCmdContext.Op = (wIndex_t)(long)commandContext;
		if ( drawCmdContext.Op < 0 || drawCmdContext.Op > OP_LAST ) {
			NoticeMessage( "cmdDraw: Op %d", _("Ok"), NULL, drawCmdContext.Op );
			drawCmdContext.Op = OP_LINE;
		}
		/*DrawGeomOp( (void*)(drawCmdContext.Op>=0?drawCmdContext.Op:OP_LINE) );*/
		infoSubst = TRUE;
		switch( drawCmdContext.Op ) {
		case OP_LINE:
		case OP_CURVE1:
		case OP_CURVE2:
		case OP_CURVE3:
		case OP_CURVE4:
		case OP_CIRCLE2:
		case OP_CIRCLE3:
		case OP_BOX:
		case OP_POLY:
        case OP_BEZLIN:
			controls[0] = drawWidthPD.control;
			controls[1] = drawColorPD.control;
			controls[2] = NULL;
			sprintf( labelName, _("%s Line Width"), _(objectName[drawCmdContext.Op]) );
			labels[0] = labelName;
			labels[1] = N_("Color");
			InfoSubstituteControls( controls, labels );
			drawWidthPD.option &= ~PDO_NORECORD;
			drawColorPD.option &= ~PDO_NORECORD;
			break;
		case OP_FILLCIRCLE2:
		case OP_FILLCIRCLE3:
		case OP_FILLBOX:
		case OP_FILLPOLY:
			controls[0] = drawColorPD.control;
			controls[1] = NULL;
			sprintf( labelName, _("%s Color"), _(objectName[drawCmdContext.Op]) );
			labels[0] = labelName;
			ParamLoadControls( &drawPG );
			InfoSubstituteControls( controls, labels );
			drawColorPD.option &= ~PDO_NORECORD;
			break;
		case OP_BENCH:
			controls[0] = drawBenchChoicePD.control;
			controls[1] = drawBenchOrientPD.control;
			controls[2] = drawBenchColorPD.control;
			controls[3] = NULL;
			labels[0] = N_("Lumber Type");
			labels[1] = "";
			labels[2] = N_("Color");
			if ( wListGetCount( (wList_p)drawBenchChoicePD.control ) == 0 )
				BenchLoadLists( (wList_p)drawBenchChoicePD.control, (wList_p)drawBenchOrientPD.control );
#ifdef LATER
			if ( benchInx >= 0 && benchInx < wListGetCount( (wList_p)drawBenchChoicePD.control ) )
				wListSetIndex( (wList_p)drawBenchChoicePD.control, benchInx );
#endif
			ParamLoadControls( &drawPG );
			BenchUpdateOrientationList( (long)wListGetItemContext( (wList_p)drawBenchChoicePD.control, benchChoice ), (wList_p)drawBenchOrientPD.control );
			wListSetIndex( (wList_p)drawBenchOrientPD.control, benchOrient );
			InfoSubstituteControls( controls, labels );
			drawBenchColorPD.option &= ~PDO_NORECORD;
			drawBenchChoicePD.option &= ~PDO_NORECORD;
			drawBenchOrientPD.option &= ~PDO_NORECORD;
			break;
		case OP_DIMLINE:
			controls[0] = drawDimArrowSizePD.control;
			controls[1] = NULL;
			labels[0] = N_("Dimension Line Size");
			if ( wListGetCount( (wList_p)drawDimArrowSizePD.control ) == 0 ) {
				wListAddValue( (wList_p)drawDimArrowSizePD.control, _("Tiny"), NULL, NULL );
				wListAddValue( (wList_p)drawDimArrowSizePD.control, _("Small"), NULL, NULL );
				wListAddValue( (wList_p)drawDimArrowSizePD.control, _("Medium"), NULL, NULL );
				wListAddValue( (wList_p)drawDimArrowSizePD.control, _("Large"), NULL, NULL );
			}
			ParamLoadControls( &drawPG );
			InfoSubstituteControls( controls, labels );
			drawDimArrowSizePD.option &= ~PDO_NORECORD;
			break;
		case OP_TBLEDGE:
			InfoSubstituteControls( NULL, NULL );
			InfoMessage( _("Drag to create Table Edge") );
			drawColorPD.option &= ~PDO_NORECORD;
			break;
		default:
			InfoSubstituteControls( NULL, NULL );
			infoSubst = FALSE;
		}
		ParamGroupRecord( &drawPG );
		DrawGeomMouse( C_START, pos, &drawCmdContext );

		return C_CONTINUE;

	case wActionLDown:
		ParamLoadData( &drawPG );
		if ( drawCmdContext.Op == OP_BENCH ) {
			drawCmdContext.benchOption = GetBenchData( (long)wListGetItemContext((wList_p)drawBenchChoicePD.control, benchChoice ), benchOrient );
			drawCmdContext.Color = benchColor;
#ifdef LATER
			benchInx = wListGetIndex( (wList_p)drawBenchChoicePD.control );
#endif
		} else if ( drawCmdContext.Op == OP_DIMLINE ) {
			drawCmdContext.benchOption = dimArrowSize;
		} else {
			drawCmdContext.Color = lineColor;
		}
		if ( infoSubst ) {
			InfoSubstituteControls( NULL, NULL );
			infoSubst = FALSE;
		}
	case wActionLDrag:
		ParamLoadData( &drawPG );
	case wActionMove:
	case wActionLUp:
	case wActionRDown:
	case wActionRDrag:
	case wActionRUp:
	case wActionText:
	case C_CMDMENU:
		SnapPos( &pos );
		return DrawGeomMouse( action, pos, &drawCmdContext );

	case C_CANCEL:
		InfoSubstituteControls( NULL, NULL );
		return DrawGeomMouse( action, pos, &drawCmdContext );

	case C_OK:
		return DrawGeomMouse( (0x0D<<8|wActionText), pos, &drawCmdContext );
		/*DrawOk( NULL );*/

	case C_FINISH:
		return DrawGeomMouse( (0x0D<<8|wActionText), pos, &drawCmdContext );
		/*DrawOk( NULL );*/

	case C_REDRAW:
		return DrawGeomMouse( action, pos, &drawCmdContext );

	default:
		return C_CONTINUE;
	}
}

#include "bitmaps/dline.xpm"
#include "bitmaps/ddimlin.xpm"
#include "bitmaps/dbench.xpm"
#include "bitmaps/dtbledge.xpm"
#include "bitmaps/dcurve1.xpm"
#include "bitmaps/dcurve2.xpm"
#include "bitmaps/dcurve3.xpm"
#include "bitmaps/dcurve4.xpm"
#include "bitmaps/dbezier.xpm"
/*#include "bitmaps/dcircle1.xpm"*/
#include "bitmaps/dcircle2.xpm"
#include "bitmaps/dcircle3.xpm"
/*#include "bitmaps/dflcrcl1.xpm"*/
#include "bitmaps/dflcrcl2.xpm"
#include "bitmaps/dflcrcl3.xpm"
#include "bitmaps/dbox.xpm"
#include "bitmaps/dfilbox.xpm"
#include "bitmaps/dpoly.xpm"
#include "bitmaps/dfilpoly.xpm"

typedef struct {
		char **xpm;
		int OP;
		char * shortName;
		char * cmdName;
		char * helpKey;
		long acclKey;
		} drawData_t;

static drawData_t dlineCmds[] = {
		{ dline_xpm, OP_LINE, N_("Line"), N_("Draw Line"), "cmdDrawLine", ACCL_DRAWLINE },
		{ ddimlin_xpm, OP_DIMLINE, N_("Dimension Line"), N_("Draw Dimension Line"), "cmdDrawDimLine", ACCL_DRAWDIMLINE },
		{ dbench_xpm, OP_BENCH, N_("Benchwork"), N_("Draw Benchwork"), "cmdDrawBench", ACCL_DRAWBENCH },
		{ dtbledge_xpm, OP_TBLEDGE, N_("Table Edge"), N_("Draw Table Edge"), "cmdDrawTableEdge", ACCL_DRAWTBLEDGE } };
static drawData_t dcurveCmds[] = {
		{ dcurve1_xpm, OP_CURVE1, N_("Curve End"), N_("Draw Curve from End"), "cmdDrawCurveEndPt", ACCL_DRAWCURVE1 },
		{ dcurve2_xpm, OP_CURVE2, N_("Curve Tangent"), N_("Draw Curve from Tangent"), "cmdDrawCurveTangent", ACCL_DRAWCURVE2 },
		{ dcurve3_xpm, OP_CURVE3, N_("Curve Center"), N_("Draw Curve from Center"), "cmdDrawCurveCenter", ACCL_DRAWCURVE3 },
		{ dcurve4_xpm, OP_CURVE4, N_("Curve Chord"), N_("Draw Curve from Chord"), "cmdDrawCurveChord", ACCL_DRAWCURVE4 },
        { dcurve4_xpm, OP_BEZLIN, N_("Bezier"), N_("Draw Bezier"), "cmdDrawBezier", ACCL_DRAWBEZ } };

static drawData_t dcircleCmds[] = {
		/*{ dcircle1_xpm, OP_CIRCLE1, "Circle Fixed Radius", "Draw Fixed Radius Circle", "cmdDrawCircleFixedRadius", ACCL_DRAWCIRCLE1 },*/
		{ dcircle2_xpm, OP_CIRCLE3, N_("Circle Tangent"), N_("Draw Circle from Tangent"), "cmdDrawCircleTangent", ACCL_DRAWCIRCLE2 },
		{ dcircle3_xpm, OP_CIRCLE2, N_("Circle Center"), N_("Draw Circle from Center"), "cmdDrawCircleCenter", ACCL_DRAWCIRCLE3 },
		/*{ dflcrcl1_xpm, OP_FILLCIRCLE1, "Circle Filled Fixed Radius", "Draw Fixed Radius Filled Circle", "cmdDrawFilledCircleFixedRadius", ACCL_DRAWFILLCIRCLE1 },*/
		{ dflcrcl2_xpm, OP_FILLCIRCLE3, N_("Circle Filled Tangent"), N_("Draw Filled Circle from Tangent"), "cmdDrawFilledCircleTangent", ACCL_DRAWFILLCIRCLE2 },
		{ dflcrcl3_xpm, OP_FILLCIRCLE2, N_("Circle Filled Center"), N_("Draw Filled Circle from Center"), "cmdDrawFilledCircleCenter", ACCL_DRAWFILLCIRCLE3 } };
static drawData_t dshapeCmds[] = {
		{ dbox_xpm, OP_BOX, N_("Box"), N_("Draw Box"), "cmdDrawBox", ACCL_DRAWBOX },
		{ dfilbox_xpm, OP_FILLBOX, N_("Filled Box"), N_("Draw Filled Box"), "cmdDrawFilledBox", ACCL_DRAWFILLBOX },
		{ dpoly_xpm, OP_POLY, N_("Poly Line"), N_("Draw Polyline"), "cmdDrawPolyline", ACCL_DRAWPOLYLINE },
		{ dfilpoly_xpm, OP_FILLPOLY, N_("Polygon"), N_("Draw Polygon"), "cmdDrawPolygon", ACCL_DRAWPOLYGON } };

typedef struct {
		char * helpKey;
		char * menuTitle;
		char * stickyLabel;
		int cnt;
		drawData_t * data;
		long acclKey;
		wIndex_t cmdInx;
		int curr;
		} drawStuff_t;
static drawStuff_t drawStuff[4];


static drawStuff_t drawStuff[4] = {
		{ "cmdDrawLineSetCmd", N_("Straight Objects"), N_("Draw Straight Objects"), 4, dlineCmds },
		{ "cmdDrawCurveSetCmd", N_("Curved Lines"), N_("Draw Curved Lines"), 4, dcurveCmds },
		{ "cmdDrawCircleSetCmd", N_("Circle Lines"), N_("Draw Circles"), 4, dcircleCmds },
		{ "cmdDrawShapeSetCmd", N_("Shapes"), N_("Draw Shapes"), 4, dshapeCmds} };
		

#ifdef LATER
static void SetDrawMode( char * modeName )
{
	wButton_p bb;
	int inx1, inx2;
	drawData_t * dp;

	for ( inx1=0; inx1<4; inx1++ ) {
		for ( inx2=0; inx2<drawStuff[inx1].cnt; inx2++ ) {
			dp = &drawStuff[inx1].data[inx2];
			if (strncmp( modeName, dp->modeS, strlen(dp->modeS) ) == 0 ) {
				bb = GetCommandButton(drawStuff[inx1].cmdInx);
				wButtonSetLabel( bb, (char*)(dp->icon) );
				wControlSetHelp( (wControl_p)bb, dp->help );
				drawStuff[inx1].curr = inx2;
				DoCommandB( (void*)(drawStuff[inx1].cmdInx) );
				return;
			}
		}
	}
}
#endif


static void ChangeDraw( long changes )
{
	wIndex_t choice, orient;
	if ( changes & CHANGE_UNITS ) {
		if ( drawBenchChoicePD.control && drawBenchOrientPD.control ) {
			choice = wListGetIndex( (wList_p)drawBenchChoicePD.control );
			orient = wListGetIndex( (wList_p)drawBenchOrientPD.control );
			BenchLoadLists( (wList_p)drawBenchChoicePD.control, (wList_p)drawBenchOrientPD.control );
			wListSetIndex( (wList_p)drawBenchChoicePD.control, choice );
			wListSetIndex( (wList_p)drawBenchOrientPD.control, orient );
		}
	}
}



static void DrawDlgUpdate(
		paramGroup_p pg,
		int inx,
		void * valueP )
{
	if ( inx >= 0 && pg->paramPtr[inx].valueP == &benchChoice )
		BenchUpdateOrientationList( (long)wListGetItemContext( (wList_p)drawBenchChoicePD.control, (wIndex_t)*(long*)valueP ), (wList_p)drawBenchOrientPD.control );
}

EXPORT void InitCmdDraw( wMenu_p menu )
{
	int inx1, inx2;
	drawStuff_t * dsp;
	drawData_t * ddp;
	wIcon_p icon;

	drawCmdContext.Color = wDrawColorBlack;
	lineColor = wDrawColorBlack;
	benchColor = wDrawFindColor( wRGB(255,192,0) );
	ParamCreateControls( &drawPG, DrawDlgUpdate );

	for ( inx1=0; inx1<4; inx1++ ) {
		dsp = &drawStuff[inx1];
		ButtonGroupBegin( _(dsp->menuTitle), dsp->helpKey, _(dsp->stickyLabel) );
		for ( inx2=0; inx2<dsp->cnt; inx2++ ) {
			ddp = &dsp->data[inx2];
			icon = wIconCreatePixMap( ddp->xpm );
			AddMenuButton( menu, CmdDraw, ddp->helpKey, _(ddp->cmdName), icon, LEVEL0_50, IC_STICKY|IC_POPUP2, ddp->acclKey, (void *)(intptr_t)ddp->OP );
		}
		ButtonGroupEnd();
	}

	ParamRegister( &drawPG );
	RegisterChangeNotification( ChangeDraw );
#ifdef LATER
		InitCommand( cmdDraw, N_("Draw"), draw_bits, LEVEL0_50, IC_POPUP|IC_CMDMENU, ACCL_DRAW );
#endif
}


BOOL_T ReadTableEdge( char * line )
{
	track_p trk;
	TRKINX_T index;
	DIST_T elev0, elev1;
	trkSeg_t seg;
	wIndex_t layer;

	if ( !GetArgs( line, paramVersion<3?"dXpYpY":paramVersion<9?"dL000pYpY":"dL000pfpf",
				&index, &layer, &seg.u.l.pos[0], &elev0, &seg.u.l.pos[1], &elev1 ) )
		return FALSE;
	seg.type = SEG_TBLEDGE;
	seg.color = wDrawColorBlack;
	seg.width = 0;
	trk = MakeDrawFromSeg1( index, zero, 0.0, &seg );
	SetTrkLayer(trk, layer);
	return TRUE;
}

/**
 * Create a new segment for text. The data are stored in a trk structure.
 * Storage for the string is dynamically allocated. 
 *
 * \param index IN of new element
 * \param pos IN coordinates of element
 * \param angle IN orientation 
 * \param text IN text itself
 * \param textSize IN font size in pts
 * \param color IN text color
 * \return    the newly allocated trk structure
 */

EXPORT track_p NewText(
		wIndex_t index,
		coOrd pos,
		ANGLE_T angle,
		char * text,
		CSIZE_T textSize,
        wDrawColor color )
{
	trkSeg_t tempSeg;
	track_p trk;
	tempSeg.type = SEG_TEXT;
	tempSeg.color = color;
	tempSeg.width = 0;
	tempSeg.u.t.pos = zero;
	tempSeg.u.t.angle = angle;
	tempSeg.u.t.fontP = NULL;
	tempSeg.u.t.fontSize = textSize;
	tempSeg.u.t.string = MyStrdup( text );
	trk = MakeDrawFromSeg1( index, pos, angle, &tempSeg );
	return trk;
}


EXPORT BOOL_T ReadText( char * line )
{
	coOrd pos;
	CSIZE_T textSize;
	char * text;
	wIndex_t index;
	wIndex_t layer;
	track_p trk;
	ANGLE_T angle;
    wDrawColor color = wDrawColorBlack;
    if ( paramVersion<3 ) {
        if (!GetArgs( line, "XXpYql", &index, &layer, &pos, &angle, &text, &textSize ))
            return FALSE;
    } else if (paramVersion<9 ) {
        if (!GetArgs(line, "dL000pYql", &index, &layer, &pos, &angle, &text, &textSize))
            return FALSE;
    } else {
        if (!GetArgs(line, "dLl00pfql", &index, &layer, &color, &pos, &angle, &text, &textSize ))
            return FALSE;
    }

	trk = NewText( index, pos, angle, text, textSize, color );
	SetTrkLayer( trk, layer );
	MyFree(text);
	return TRUE;
}


EXPORT void InitTrkDraw( void )
{
	T_DRAW = InitObject( &drawCmds );
	AddParam( "TABLEEDGE", ReadTableEdge );
	AddParam( "TEXT", ReadText );
}
