/*
 * $Header: /home/dmarkle/xtrkcad-fork-cvs/xtrkcad/app/bin/dlayer.c,v 1.1 2005-12-07 15:47:21 rc-flyer Exp $
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


/*****************************************************************************
 *
 * LAYERS
 *
 */

#define NUM_LAYERS		(99)
#define NUM_BUTTONS		(20)
EXPORT LAYER_T curLayer;
EXPORT long layerCount = 10;
static long newLayerCount = 10;
static LAYER_T layerCurrent = NUM_LAYERS;


static BOOL_T layoutLayerChanged = FALSE;

static wIcon_p show_layer_bmps[NUM_BUTTONS];
static wIcon_p hide_layer_bmps[NUM_BUTTONS];
static wButton_p layer_btns[NUM_BUTTONS];
static wList_p setLayerL;
/*static wMessage_p layerNumM;*/
typedef struct {
		char name[STR_SHORT_SIZE];
		wDrawColor color;
		BOOL_T frozen;
		BOOL_T visible;
		BOOL_T onMap;
		long objCount;
		} layer_t;
static layer_t layers[NUM_LAYERS];
static layer_t layers_save[NUM_LAYERS];


static int oldColorMap[][3] = {
		{ 255, 255, 255 },		/* White */
		{   0,   0,   0 },      /* Black */
		{ 255,   0,   0 },      /* Red */
		{   0, 255,   0 },      /* Green */
		{   0,   0, 255 },      /* Blue */
		{ 255, 255,   0 },      /* Yellow */
		{ 255,   0, 255 },      /* Purple */
		{   0, 255, 255 },      /* Aqua */
		{ 128,   0,   0 },      /* Dk. Red */
		{   0, 128,   0 },      /* Dk. Green */
		{   0,   0, 128 },      /* Dk. Blue */
		{ 128, 128,   0 },      /* Dk. Yellow */
		{ 128,   0, 128 },      /* Dk. Purple */
		{   0, 128, 128 },      /* Dk. Aqua */
		{  65, 105, 225 },      /* Royal Blue */
		{   0, 191, 255 },      /* DeepSkyBlue */
		{ 125, 206, 250 },      /* LightSkyBlue */
		{  70, 130, 180 },      /* Steel Blue */
		{ 176, 224, 230 },      /* Powder Blue */
		{ 127, 255, 212 },      /* Aquamarine */
		{  46, 139,  87 },      /* SeaGreen */
		{ 152, 251, 152 },      /* PaleGreen */
		{ 124, 252,   0 },      /* LawnGreen */
		{  50, 205,  50 },      /* LimeGreen */
		{  34, 139,  34 },      /* ForestGreen */
		{ 255, 215,   0 },      /* Gold */
		{ 188, 143, 143 },      /* RosyBrown */
		{ 139, 69, 19 },        /* SaddleBrown */
		{ 245, 245, 220 },      /* Beige */
		{ 210, 180, 140 },      /* Tan */
		{ 210, 105, 30 },       /* Chocolate */
		{ 165, 42, 42 },        /* Brown */
		{ 255, 165, 0 },        /* Orange */
		{ 255, 127, 80 },       /* Coral */
		{ 255, 99, 71 },        /* Tomato */
		{ 255, 105, 180 },      /* HotPink */
		{ 255, 192, 203 },      /* Pink */
		{ 176, 48, 96 },        /* Maroon */
		{ 238, 130, 238 },      /* Violet */
		{ 160, 32, 240 },       /* Purple */
		{  16,  16,  16 },      /* Gray */
		{  32,  32,  32 },      /* Gray */
		{  48,  48,  48 },      /* Gray */
		{  64,  64,  64 },      /* Gray */
		{  80,  80,  80 },      /* Gray */
		{  96,  96,  96 },      /* Gray */
		{ 112, 112, 122 },      /* Gray */
		{ 128, 128, 128 },      /* Gray */
		{ 144, 144, 144 },      /* Gray */
		{ 160, 160, 160 },      /* Gray */
		{ 176, 176, 176 },      /* Gray */
		{ 192, 192, 192 },      /* Gray */
		{ 208, 208, 208 },      /* Gray */
		{ 224, 224, 224 },      /* Gray */
		{ 240, 240, 240 },      /* Gray */
		{   0,   0,   0 }       /* BlackPixel */
		};


EXPORT BOOL_T GetLayerVisible( LAYER_T layer )
{
	if (layer < 0 || layer >= NUM_LAYERS)
		return TRUE;
	else
		return layers[(int)layer].visible;
}


EXPORT BOOL_T GetLayerFrozen( LAYER_T layer )
{
	if (layer < 0 || layer >= NUM_LAYERS)
		return TRUE;
	else
		return layers[(int)layer].frozen;
}


EXPORT BOOL_T GetLayerOnMap( LAYER_T layer )
{
	if (layer < 0 || layer >= NUM_LAYERS)
		return TRUE;
	else
		return layers[(int)layer].onMap;
}


EXPORT char * GetLayerName( LAYER_T layer )
{
	if (layer < 0 || layer >= NUM_LAYERS)
		return NULL;
	else
		return layers[(int)layer].name;
}


EXPORT void NewLayer( void )
{
}


EXPORT wDrawColor GetLayerColor( LAYER_T layer )
{
	return layers[(int)layer].color;
}


static void FlipLayer( void * arg )
{
	LAYER_T l = (LAYER_T)(long)arg;
	wBool_t visible;
	if ( l < 0 || l >= NUM_LAYERS )
		return;
	if ( l == curLayer && layers[(int)l].visible) {
		NoticeMessage( MSG_LAYER_HIDE, "Ok", NULL );
		return;
	}
	RedrawLayer( l, FALSE );
	visible = !layers[(int)l].visible;
	layers[(int)l].visible = visible;
	if (l<NUM_BUTTONS)
		wButtonSetLabel( layer_btns[(int)l], (char*)(visible?show_layer_bmps[(int)l]:hide_layer_bmps[(int)l]) );
	RedrawLayer( l, TRUE );
}


/*void SetLayer( void * arg )*/
static void SetCurrLayer( wIndex_t inx, const char * name, wIndex_t op, void * listContext, void * arg )
{
	LAYER_T newLayer = (LAYER_T)(long)inx;
	if (layers[(int)newLayer].frozen) {
		NoticeMessage( MSG_LAYER_SEL_FROZEN, "Ok", NULL );
		wListSetIndex( setLayerL, curLayer );
		return;
	}
	curLayer = newLayer;
	if ( curLayer < 0 || curLayer >= NUM_LAYERS )
		curLayer = 0;
	if ( !layers[(int)curLayer].visible )
		FlipLayer( (void*)inx );
	if ( recordF )
		fprintf( recordF, "SETCURRLAYER %d\n", inx );
}

static void PlaybackCurrLayer( char * line )
{
	wIndex_t layer;
	layer = atoi(line);
	wListSetIndex( setLayerL, layer );
	SetCurrLayer( layer, NULL, 0, NULL, NULL );
}


static void SetLayerColor( int inx, wDrawColor color )
{
	if ( color != layers[inx].color ) {
		if (inx < NUM_BUTTONS) {
			wIconSetColor( show_layer_bmps[inx], color );
			wIconSetColor( hide_layer_bmps[inx], color );
			wButtonSetLabel( layer_btns[inx], (char*)(layers[inx].visible?show_layer_bmps[inx]:hide_layer_bmps[inx]) );
		}
		layers[inx].color = color;
		layoutLayerChanged = TRUE;
	}
}


#include "l1.bmp"
#include "l2.bmp"
#include "l3.bmp"
#include "l4.bmp"
#include "l5.bmp"
#include "l6.bmp"
#include "l7.bmp"
#include "l8.bmp"
#include "l9.bmp"
#include "l10.bmp"
#include "l11.bmp"
#include "l12.bmp"
#include "l13.bmp"
#include "l14.bmp"
#include "l15.bmp"
#include "l16.bmp"
#include "l17.bmp"
#include "l18.bmp"
#include "l19.bmp"
#include "l20.bmp"
#include "k1.bmp"
#include "k2.bmp"
#include "k3.bmp"
#include "k4.bmp"
#include "k5.bmp"
#include "k6.bmp"
#include "k7.bmp"
#include "k8.bmp"
#include "k9.bmp"
#include "k10.bmp"
#include "k11.bmp"
#include "k12.bmp"
#include "k13.bmp"
#include "k14.bmp"
#include "k15.bmp"
#include "k16.bmp"
#include "k17.bmp"
#include "k18.bmp"
#include "k19.bmp"
#include "k20.bmp"
static char * show_layer_bits[NUM_BUTTONS] = { l1_bits, l2_bits, l3_bits, l4_bits, l5_bits, l6_bits, l7_bits, l8_bits, l9_bits, l10_bits,
 l11_bits, l12_bits, l13_bits, l14_bits, l15_bits, l16_bits, l17_bits, l18_bits, l19_bits, l20_bits };
static char * hide_layer_bits[NUM_BUTTONS] = { k1_bits, k2_bits, k3_bits, k4_bits, k5_bits, k6_bits, k7_bits, k8_bits, k9_bits, k10_bits,
 k11_bits, k12_bits, k13_bits, k14_bits, k15_bits, k16_bits, k17_bits, k18_bits, k19_bits, k20_bits };
static EXPORT long layerRawColorTab[] = {
		wRGB(  0,  0,255),      /* blue */
		wRGB(  0,  0,128),      /* dk blue */
		wRGB(  0,128,  0),      /* dk green */
		wRGB(255,255,  0),      /* yellow */
		wRGB(  0,255,  0),      /* green */
		wRGB(  0,255,255),      /* lt cyan */
		wRGB(128,  0,  0),      /* brown */
		wRGB(128,  0,128),      /* purple */
		wRGB(128,128,  0),      /* green-brown */
		wRGB(255,  0,255)};     /* lt-purple */
static EXPORT wDrawColor layerColorTab[COUNT(layerRawColorTab)];


static wWin_p layerW;
static char layerName[STR_SHORT_SIZE];
static wDrawColor layerColor;
static long layerVisible = TRUE;
static long layerFrozen = FALSE;
static long layerOnMap = TRUE;
static void LayerOk( void * );
static BOOL_T layerRedrawMap = FALSE;


static char *visibleLabels[] = { "", NULL };
static char *frozenLabels[] = { "", NULL };
static char *onMapLabels[] = { "", NULL };
static paramIntegerRange_t i0_20 = { 0, NUM_BUTTONS };

static paramData_t layerPLs[] = {
#define I_LIST	(0)
	 { PD_DROPLIST, NULL, "layer", PDO_LISTINDEX|PDO_DLGNOLABELALIGN, (void*)250 },
#define I_NAME	(1)
	 { PD_STRING, layerName, "name", PDO_NOPREF, (void*)(250-54), "Name" },
#define I_COLOR	(2)
	 { PD_COLORLIST, &layerColor, "color", PDO_NOPREF, NULL, "Color" },
#define I_VIS	(3)
	 { PD_TOGGLE, &layerVisible, "visible", PDO_NOPREF, visibleLabels, "Visible", BC_HORZ|BC_NOBORDER },
#define I_FRZ	(4)
	 { PD_TOGGLE, &layerFrozen, "frozen", PDO_NOPREF|PDO_DLGHORZ, frozenLabels, "Frozen", BC_HORZ|BC_NOBORDER },
#define I_MAP	(5)
	 { PD_TOGGLE, &layerOnMap, "onmap", PDO_NOPREF|PDO_DLGHORZ, onMapLabels, "On Map", BC_HORZ|BC_NOBORDER },
#define I_COUNT (6)
	 { PD_STRING, NULL, "object-count", PDO_NOPREF|PDO_DLGBOXEND, (void*)(80), "Count", BO_READONLY },
	 { PD_LONG, &newLayerCount, "button-count", PDO_DLGBOXEND|PDO_DLGRESETMARGIN, &i0_20, "Number of Layer Buttons" } };
static paramGroup_t layerPG = { "layer", 0, layerPLs, sizeof layerPLs/sizeof layerPLs[0] };

#define layerL	((wList_p)layerPLs[I_LIST].control)


static void LayerSetCounts( void )
{
	int inx;
	track_p trk;
	for ( inx=0; inx<NUM_LAYERS; inx++ )
		layers[inx].objCount = 0;
	for ( trk=NULL; TrackIterate(&trk); ) {
		inx = GetTrkLayer(trk);
		if ( inx >= 0 && inx < NUM_LAYERS )
			layers[inx].objCount++;
	}
}


static void LayerUpdate( void )
{
	BOOL_T redraw;
	ParamLoadData( &layerPG );
	if (layerCurrent < 0 || layerCurrent >= NUM_LAYERS)
		return;
	if (layerCurrent == curLayer && layerFrozen) {
		NoticeMessage( MSG_LAYER_FREEZE, "Ok", NULL );
		layerFrozen = FALSE;
		ParamLoadControl( &layerPG, I_FRZ );
	}
	if (layerCurrent == curLayer && !layerVisible) {
		NoticeMessage( MSG_LAYER_HIDE, "Ok", NULL );
		layerVisible = TRUE;
		ParamLoadControl( &layerPG, I_VIS );
	}
	if ( layerL ) {
		strncpy( layers[(int)layerCurrent].name, layerName, sizeof layers[(int)layerCurrent].name );
		sprintf( message, "%2d %c %s", (int)layerCurrent+1, layers[(int)layerCurrent].objCount>0?'+':'-', layers[(int)layerCurrent].name );
		wListSetValues( layerL, layerCurrent, message, NULL, NULL );
	}
	sprintf( message, "%2d - %s", (int)layerCurrent+1, layers[(int)layerCurrent].name );
	wListSetValues( setLayerL, layerCurrent, message, NULL, NULL );
	if (layerCurrent < NUM_BUTTONS) {
		if (strlen(layers[(int)layerCurrent].name)>0)
			wControlSetBalloonText( (wControl_p)layer_btns[(int)layerCurrent], layers[(int)layerCurrent].name );
		else
			wControlSetBalloonText( (wControl_p)layer_btns[(int)layerCurrent], "Show/Hide Layer" );
	}
	redraw = ( layerColor != layers[(int)layerCurrent].color ||
			   (BOOL_T)layerVisible != layers[(int)layerCurrent].visible );
	if ( (!layerRedrawMap) && redraw)
		RedrawLayer( (LAYER_T)layerCurrent, FALSE );
	SetLayerColor( layerCurrent, layerColor );
	if (layerCurrent<NUM_BUTTONS && layers[(int)layerCurrent].visible!=(BOOL_T)layerVisible)
		wButtonSetLabel( layer_btns[(int)layerCurrent], (char*)(layerVisible?show_layer_bmps[(int)layerCurrent]:hide_layer_bmps[(int)layerCurrent]) );
	layers[(int)layerCurrent].visible = (BOOL_T)layerVisible;
	layers[(int)layerCurrent].frozen = (BOOL_T)layerFrozen;
	layers[(int)layerCurrent].onMap = (BOOL_T)layerOnMap;
	if ( layerRedrawMap )
		DoRedraw();
	else if (redraw)
		RedrawLayer( (LAYER_T)layerCurrent, TRUE );
	layerRedrawMap = FALSE;
}


static void LayerSelect( 
		wIndex_t inx )
{
	LayerUpdate();
	if (inx < 0 || inx >= NUM_LAYERS)
		return;
	layerCurrent = (LAYER_T)inx;
	strcpy( layerName, layers[inx].name );
	layerVisible = layers[inx].visible;
	layerFrozen = layers[inx].frozen;
	layerOnMap = layers[inx].onMap;
	layerColor = layers[inx].color;
	sprintf( message, "%ld", layers[inx].objCount );
	ParamLoadMessage( &layerPG, I_COUNT, message );
	ParamLoadControls( &layerPG );
}


static void LoadLayerList( void )
{
	wIndex_t inx;

	if ( layerL ) wListClear(layerL);
	wListClear(setLayerL);
	LayerSetCounts();
	for ( inx=0; inx<NUM_LAYERS; inx++ ) {
		if ( layerL ) {
			sprintf( message, "%2d %c %s", inx+1, layers[inx].objCount>0?'+':'-', layers[inx].name );
			wListAddValue( layerL, message, NULL, NULL );
		}
		sprintf( message, "%2d - %s", inx+1, layers[inx].name );
		wListAddValue( setLayerL, message, NULL, NULL );
	}
	if ( layerL ) wListSetIndex( layerL, curLayer );
	wListSetIndex( setLayerL, curLayer );
	LayerSelect( curLayer );
}


EXPORT void ResetLayers( void )
{
	int inx;
	for ( inx=0;inx<NUM_LAYERS; inx++ ) {
		strcpy( layers[inx].name, inx==0?"Main":"" );
		layers[inx].visible = TRUE;
		layers[inx].frozen = FALSE;
		layers[inx].onMap = TRUE;
		layers[inx].objCount = 0;
		SetLayerColor( inx, layerColorTab[inx%COUNT(layerColorTab)] );
		if ( inx<NUM_BUTTONS ) {
			wButtonSetLabel( layer_btns[inx], (char*)show_layer_bmps[inx] );
		}
	}
	wControlSetBalloonText( (wControl_p)layer_btns[0], "Main" );
	for ( inx=1; inx<NUM_BUTTONS; inx++ ) {
		wControlSetBalloonText( (wControl_p)layer_btns[inx], "Show/Hide Layer" );
	}
	curLayer = 0;
	layerVisible = TRUE;
	layerFrozen = FALSE;
	layerOnMap = TRUE;
	layerColor = layers[inx].color;
	strcpy( layerName, layers[0].name );
	if (layerL) {
		ParamLoadControls( &layerPG );
		ParamLoadMessage( &layerPG, I_COUNT, "0" );
	}
	LoadLayerList();
}


EXPORT void SaveLayers( void )
{
	memcpy( layers_save, layers, NUM_LAYERS * sizeof layers[0] );
	ResetLayers();
}

EXPORT void RestoreLayers( void )
{
	int inx;
	char * label;
	wDrawColor color;
	memcpy( layers, layers_save, NUM_LAYERS * sizeof layers[0] );
	for ( inx=0; inx<NUM_BUTTONS; inx++ ) {
		color = layers[inx].color;
		layers[inx].color = -1;
		SetLayerColor( inx, color );
		if ( layers[inx].name[0] == '\0' ) {
			if ( inx == 0 ) {
				label = "Main";
			} else {
				label = "Show/Hide Layer";
			}
		} else {
			label = layers[inx].name;
		}
		wControlSetBalloonText( (wControl_p)layer_btns[inx], label );
		wButtonSetLabel( layer_btns[inx], (char*)(layers[inx].visible?show_layer_bmps[inx]:hide_layer_bmps[inx]) );
	}
	if (layerL) {
		ParamLoadControls( &layerPG );
		ParamLoadMessage( &layerPG, I_COUNT, "0" );
	}
	LoadLayerList();
}


static void LayerOk( void * junk )
{
	LayerUpdate();
	if (newLayerCount != layerCount) {
		layoutLayerChanged = TRUE;
		if ( newLayerCount > NUM_BUTTONS )
			newLayerCount = NUM_BUTTONS;
		layerCount = newLayerCount;
	}
	if (layoutLayerChanged)
		MainProc( mainW, wResize_e, NULL );
	wHide( layerW );
}


static void LayerDlgUpdate(
		paramGroup_p pg,
		int inx,
		void * valueP )
{
	switch (inx) {
	case I_LIST:
		LayerSelect( (wIndex_t)*(long*)valueP );
		break;
	case I_NAME:
		LayerUpdate();
		break;
	case I_MAP:
		layerRedrawMap = TRUE;
		break;
	}
}


static void DoLayer( void * junk )
{
	if (layerW == NULL)
		layerW = ParamCreateDialog( &layerPG, MakeWindowTitle("Layers"), "Done", LayerOk, NULL, TRUE, NULL, 0, LayerDlgUpdate );
	ParamLoadControls( &layerPG );
	LoadLayerList();
	layerRedrawMap = FALSE;
	wShow( layerW );
	layoutLayerChanged = FALSE;
}


EXPORT BOOL_T ReadLayers( char * line )
{
	char * name;
	int inx, visible, frozen, color, onMap;
	long rgb;
	if (paramVersion < 7)
		return TRUE;
	if ( strncmp( line, "CURRENT", 7 ) == 0 ) {
		curLayer = atoi( line+7 );
		if ( curLayer < 0 )
			curLayer = 0;
	if (layerL)
		wListSetIndex( layerL, curLayer );
	if (setLayerL)
		wListSetIndex( setLayerL, curLayer );
		return TRUE;
	}
	if (!GetArgs( line, "ddddl0000q", &inx, &visible, &frozen, &onMap, &rgb, &name ))
		return FALSE;
	if (paramVersion < 9) {
		if ( rgb >= 0 && (int)rgb < sizeof oldColorMap/sizeof oldColorMap[0] )
			rgb = wRGB( oldColorMap[(int)rgb][0], oldColorMap[(int)rgb][1], oldColorMap[(int)rgb][2] );
		else
			rgb = 0;
	}
	if (inx < 0 || inx >= NUM_LAYERS)
		return FALSE;
	color = wDrawFindColor(rgb);
	SetLayerColor( inx, color );
	strncpy( layers[inx].name, name, sizeof layers[inx].name );
	layers[inx].visible = visible;
	layers[inx].frozen = frozen;
	layers[inx].onMap = onMap;
	layers[inx].color = color;
	if (layerL) {
		sprintf( message, "%2d %c %s", inx+1, layers[inx].objCount>0?'+':'-', name );
		wListSetValues( layerL, inx, message, NULL, NULL );
	}
	if (setLayerL) {
		sprintf( message, "%2d - %s", inx+1, name );
		wListSetValues( setLayerL, inx, message, NULL, NULL );
	}
	if (inx<NUM_BUTTONS) {
		if (strlen(name) > 0) {
			wControlSetBalloonText( (wControl_p)layer_btns[(int)inx], layers[inx].name );
		}
		wButtonSetLabel( layer_btns[(int)inx], (char*)(visible?show_layer_bmps[(int)inx]:hide_layer_bmps[(int)inx]) );
	}
	return TRUE;
}


EXPORT BOOL_T WriteLayers( FILE * f )
{
	int inx;
	BOOL_T rc = TRUE;
	for (inx=0; inx<NUM_LAYERS; inx++) 
		if ((!layers[inx].visible) || layers[inx].frozen || (!layers[inx].onMap) ||
			layers[inx].color!=layerColorTab[inx%(COUNT(layerColorTab))] ||
			layers[inx].name[0] )
			rc &= fprintf( f, "LAYERS %d %d %d %d %ld %d %d %d %d \"%s\"\n", inx, layers[inx].visible, layers[inx].frozen, layers[inx].onMap, wDrawGetRGB(layers[inx].color), 0, 0, 0, 0, PutTitle(layers[inx].name) )>0;
	rc &= fprintf( f, "LAYERS CURRENT %d\n", curLayer )>0;
	return TRUE;
}


EXPORT void InitLayers( void )
{
	int i;

	wPrefGetInteger( PREFSECT, "layer-button-count", &layerCount, layerCount );
	for ( i = 0; i<COUNT(layerRawColorTab); i++ )
		layerColorTab[i] = wDrawFindColor( layerRawColorTab[i] );
	for ( i = 0; i<NUM_BUTTONS; i++ ) {
		show_layer_bmps[i] = wIconCreateBitMap( l1_width, l1_height, show_layer_bits[i], layerColorTab[i%(COUNT(layerColorTab))] );
		hide_layer_bmps[i] = wIconCreateBitMap( l1_width, l1_height, hide_layer_bits[i], layerColorTab[i%(COUNT(layerColorTab))] );
		layers[i].color = layerColorTab[i%(COUNT(layerColorTab))];
	}
	setLayerL = wDropListCreate( mainW, 0, 0, "cmdLayerSet", NULL, 0, 10, 200, NULL, SetCurrLayer, NULL );
	wControlSetBalloonText( (wControl_p)setLayerL, GetBalloonHelpStr("cmdLayerSet") );
	AddToolbarControl( (wControl_p)setLayerL, IC_MODETRAIN_TOO );

	for ( i = 0; i<NUM_LAYERS; i++ ) {
		if (i<NUM_BUTTONS) {
		   sprintf( message, "cmdLayerShow%d", i );
		   layer_btns[i] = wButtonCreate( mainW, 0, 0, message,
				(char*)(show_layer_bmps[i]),
				BO_ICON, 0, (wButtonCallBack_p)FlipLayer, (void*)i );
			wControlSetBalloonText( (wControl_p)layer_btns[i], "Shows or hides layers" );
#ifdef LATER
		   AddSimpleButton( message, "SetShowLayer", NULL, NULL, 0, NULL, 0, 0,
				(wControl_p)layer_btns[i] );
#endif
			AddToolbarControl( (wControl_p)layer_btns[i], IC_MODETRAIN_TOO );
		}
		sprintf( message, "%2d - %s", i+1, (i==0?"Main":"") );
		wListAddValue( setLayerL, message, NULL, (void*)i );
	}
	AddPlaybackProc( "SETCURRLAYER", PlaybackCurrLayer, NULL );
	AddPlaybackProc( "LAYERS", (playbackProc_p)ReadLayers, NULL );
}


EXPORT addButtonCallBack_t InitLayersDialog( void ) {
	ParamRegister( &layerPG );
	return &DoLayer;
}