/****************************************************************************
This program communicates with a RAID device via the serial port and finds
out how many RAID modules are installed.

****************************************************************************/
#include <scsi.h>
#include <devices.h>
#include <Serial.h>
#include <Quickdraw.h>
#include <string.h>

/*************************************************************************/
#define	IsHighBitSet( longNum )		( (longNum >> 23) & 1 )
#define	SetHighByte( longNum )		( longNum |= 0xFF000000 )
#define	ClearHighByte( longNum )	( longNum &= 0x00FFFFFF )

#define k1stScreenID				128		// Opening screen PICT ID
#define kAbtScreenID				127		// About box screen PICT ID
#define kInquiryCmd					0x12	// SCSI Inquiry
#define kCompletionTimeout			300		// SCSI Inquiry

// display characteristics

#define CHARS_PER_LINE				80
#define ROWS_PER_WINDOW				15
#define LINESPACE					12
#define FONTSIZE					9					


#define SERBUFSIZ 					25000

#define XONCHAR 					0x11
#define XOFFCHAR 					0x13

#define OUTDRIVER 					"\p.AOut"
#define INDRIVER 					"\p.AIn"
#define kScanDelay 					60L

// Key codes

#define homeKey 			1
#define enterKey 			3
#define endKey 				4
#define deleteKey 			8
#define tabKey 				9
#define lineFeed 			10
#define pageUpKey 			11
#define pageDnKey 			12
#define returnKey 			13
#define escapeKey 			27
#define arrowLeftKey 		28
#define arrowRtKey 			29
#define arrowUpKey 			30
#define arrowDnKey 			31

#define hexEscKey 			0x1B
#define hexDashKey 			0x71
#define hexDashPrintKey 	0x5F
	
typedef struct								// SCSI Inquiry
{
	char	opCode;
	char	lun;
	char	pageCode;
	char	reserved;
	char	allocationLen;
	char	control;
} CDBType;

/********************************* Globals ****************************************/
GrafPtr			gMainPort;					//sets a global port for main window


CDBType			gCDB;						// SCSI Inquiry


SCSIInstr		TIB[2];						// SCSI Inquiry


char			*gCDBPtr;					// SCSI Inquiry
char 			*inbuf;						// serial code
char			myDataBuffer[256];			// SCSI Inquiry


PicHandle		quitPressedPic;
PicHandle		createPressedPic;
PicHandle		clearPressedPic;
PicHandle		scanPic;
PicHandle		scanClearPic;
PicHandle		module1Pic;
PicHandle		module2Pic;
PicHandle		module3Pic;
PicHandle		module4Pic;
PicHandle		module5Pic;
PicHandle		module6Pic;
PicHandle		module7Pic;
PicHandle		blankModulePic;
PicHandle		platterPic;
PicHandle		RAID0StripPic;
PicHandle		statusPic;
PicHandle		gCreateGrayPic;
PicHandle		gClearGrayPic;
PicHandle		module2BlankPic;
PicHandle		module3BlankPic;
PicHandle		module4BlankPic;
PicHandle		module5BlankPic;
PicHandle		module6BlankPic;
PicHandle		module7BlankPic;


WindowPtr		gMainWindow;


Ptr				buf;						// serial code


RgnHandle		gMod1Rgn;					// defined for dragging gray outline
RgnHandle		gMod2Rgn;					// defined for dragging gray outline
RgnHandle		gMod3Rgn;					// defined for dragging gray outline
RgnHandle		gMod4Rgn;					// defined for dragging gray outline
RgnHandle		gMod5Rgn;					// defined for dragging gray outline
RgnHandle		gMod6Rgn;					// defined for dragging gray outline
RgnHandle		gMod7Rgn;					// defined for dragging gray outline


long			gCoords;
long			count;						// serial code


Str255			gMod1String;
Str255			gMod2String;
Str255			gMod3String;
Str255			gMod4String;
Str255			gMod5String;
Str255			gMod6String;
Str255			gMod7String;


Rect			gQuitRect;
Rect			gCreateRect;
Rect			gClearRect;
Rect			gScanRect;
Rect			gFloatRect;
Rect			gModule1Rect;				// used for drawing picture of module
Rect			gModule2Rect;				// used for drawing picture of module
Rect			gModule3Rect;				// used for drawing picture of module
Rect			gModule4Rect;				// used for drawing picture of module
Rect			gModule5Rect;				// used for drawing picture of module
Rect			gModule6Rect;				// used for drawing picture of module
Rect			gModule7Rect;				// used for drawing picture of module
Rect			gBlankModuleRect;
Rect			gRAID0Rect;					// encompasses RAID 0 strip
Rect			gRAID1Rect;					// encompasses RAID 1 strip
Rect			gRAID2Rect;					// encompasses RAID 2 strip
Rect			gRAID5Rect;					// encompasses RAID 3 strip
Rect			gRAID6Rect;					// encompasses RAID 4 strip
Rect			gRAID0Mod1Rect;				// Rect for Module 1 in RAID 0 strip
Rect			gRAID0Mod2Rect;				// Rect for Module 2 in RAID 0 strip
Rect			gRAID0Mod3Rect;				// Rect for Module 3 in RAID 0 strip
Rect			gRAID0Mod4Rect;				// Rect for Module 4 in RAID 0 strip
Rect			gRAID0Mod5Rect;				// Rect for Module 5 in RAID 0 strip
Rect			gRAID0Mod6Rect;				// Rect for Module 6 in RAID 0 strip
Rect			gRAID0Mod7Rect;				// Rect for Module 7 in RAID 0 strip
Rect			gMouseCoordsRect;
Rect			icon1Rect;					// for lightning bolt icons
Rect			icon2Rect;					// for lightning bolt icons
Rect			icon3Rect;					// for lightning bolt icons
Rect			icon4Rect;					// for lightning bolt icons
Rect			icon5Rect;					// for lightning bolt icons
Rect			icon6Rect;					// for lightning bolt icons
Rect			gStatusRect;					// for drawing status bar
Rect			gMsgTextRect;					// for erasing message text


short			gMySCSIID;					// SCSI Inquiry ID
short			gMod1ID;					// holds the module ID
short			gMod2ID;					// holds the module ID
short			gMod3ID;					// holds the module ID
short			gMod4ID;					// holds the module ID
short			gMod5ID;					// holds the module ID
short			gMod6ID;					// holds the module ID
short			gMod7ID;					// holds the module ID
short			factorH;
short			factorV;
short 			inRefNum, outRefNum;		// serial code


Boolean			gDone;
Boolean			gFirstTime = TRUE;
Boolean			gModule1Used = FALSE;		// set true during mouseDown if 
Boolean			gModule2Used = FALSE;		// module dragged to RAIDrect
Boolean			gModule3Used = FALSE;		// set to FALSE is clear button clicked 
Boolean			gModule4Used = FALSE;		// module dragged to RAIDrect
Boolean			gModule5Used = FALSE;		// set true during mouseDown if 
Boolean			gModule6Used = FALSE;		// module dragged to RAIDrect
Boolean			gModule7Used = FALSE;		// set to FALSE is clear button clicked 
Boolean			gModule1Present = FALSE;	// set true during scan if module present
Boolean			gModule2Present = FALSE;	// set true during scan if module present
Boolean			gModule3Present = FALSE;	// set true during scan if module present
Boolean			gModule4Present = FALSE;	// set true during scan if module present
Boolean			gModule5Present = FALSE;	// set true during scan if module present
Boolean			gModule6Present = FALSE;	// set true during scan if module present
Boolean			gModule7Present = FALSE;	// set true during scan if module present
Boolean			gRAID0Mod1Used = FALSE;		// is true if there's a module in slot 1
Boolean			gRAID0Mod2Used = FALSE;		// is true if there's a module in slot 2
Boolean			gRAID0Mod3Used = FALSE;		// is true if there's a module in slot 3
Boolean			gRAID0Mod4Used = FALSE;		// is true if there's a module in slot 4
Boolean			gRAID0Mod5Used = FALSE;		// is true if there's a module in slot 5
Boolean			gRAID0Mod6Used = FALSE;		// is true if there's a module in slot 6
Boolean			gRAID0Mod7Used = FALSE;		// is true if there's a module in slot 7
Boolean			gRAID0Str = FALSE;			// draws "RAID 0" in blue when TRUE
Boolean			gClearButEnabled = FALSE;	// TRUE when 1st module dragged 2 RAIDRect
Boolean			gCreateButEnabled = FALSE;	// TRUE when 2nd module dragged 2 RAIDRect
Boolean			gRAIDCreated = FALSE;		// TRUE when create button clicked

/*********************************  My Routines (All Procedures)  **********************/
void	ToolBoxInit( void );
void	CreateWindow( void );
void	aboutBox( void );
void	restartSystem( void );
void	create( void );
void	MenuBarInit( void );
void	LoadScreen( void );
void	InitProgram( void );
void	ModuleScan( void );
void	CenterPict( PicHandle picture, Rect *destRectPtr );
void	EventLoop( void );							// for "Quit" from FILE menu
void	DoEvent( EventRecord *eventPtr );			// for "Quit" from FILE menu
void	HandleNull( EventRecord *eventPtr );		// for "Quit" from FILE menu
void	HandleMouseDown( EventRecord *eventPtr );	// for "Quit" from FILE menu
void	HandleMouseUp( EventRecord *eventPtr );		// for "mouse up"
OSErr	SCSIRoutine( void );						// for "SCSI Inquiry"
void	SCSIText( void );							// for "SCSI Inquiry"
void	ErrorText( void );							// for "SCSI Inquiry"
void	HandleMenuChoice( long menuChoice );		// for "Quit" from FILE menu
void	HandleAppleChoice( short item );			// for items from APPLE menu
void	HandleFileChoice( short item );				// for items from FILE menu
void	HandleStatusChoice( short item );			// for items from Status menu
void	HandleAdvancedOptionsChoice( short item );	// for items from Advanced Options menu
void	HandleRAIDToolsChoice( short item );		// for items from RAID Tools menu
void	RAIDStatus( void );
void	DoUpdate( EventRecord *eventPtr );			// for updating window
void	ReDrawPicture( WindowPtr window, PicHandle picture );// for updating window
void	InRAIDRect( void );							//for dragNdrop support
void	DrawMouseCoords(  Rect tempRect  );
void	DrawDDStrings( short IDnum );
void	FillRAID0( Str255 stringA );
void	DrawRAID0Text( void );
void	DriveFail( void );
void	ResetFail( void );
void	StatusText( void );
void	MsgText( Str255 stringA );

// serial routines
void	InitProgramICU( void );
OSErr 	serialinit(void);
void 	displaybuff(void);
void 	resetDisplay(void);
void 	cleanup(void);
void	FindWayBack( void );
void	SendEsc( void );
void	CheckSerBuf( void );
void	SendTab( void );
void	GoToCreateRAID( void );
void 	GoToEnclosureStatus( void );
void	AreDrivesThere( void );
void	WhichSlotsFilled(void);
void 	Dialog(Str255	string);

pascal	void	dragAction(void)
{ 
	InRAIDRect();
}

/*-------------------------------- MAIN MAIN MAIN MAIN --------------------------------*/
void	main( void)
{
	ToolBoxInit();
	CreateWindow();
	MenuBarInit();
	LoadScreen();
	InitProgram();
	EventLoop();
}
/*-------------------------------------------------------------------------------------*/


/************************ PROCEDURE: Initializes the toolbox ***************************/
void	ToolBoxInit( void )
{
	InitGraf( &thePort );
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	//InitDialogs();
	InitCursor();
}

/******************** PROCEDURE: creates window from resource fork *********************/
void	CreateWindow( void )
{
	WindowPtr		window;

	window = GetNewCWindow( k1stScreenID, nil, (WindowPtr)-1L );	// k1stScreenID is window ID 128
	
	if ( window == nil )	// In case program couldn't load resource
	{
		SysBeep( 10 );
		ExitToShell();
	}
	gMainWindow = window;

	SetWRefCon ( window, (long) (k1stScreenID) );	// k1stScreenID is window ID 128
	ShowWindow( window );							//  ROM Routine
	SetPort( window );								//  ROM Routine
	GetPort( &gMainPort );			//makes main window the main port to draw in
}

/*********************** PROCEDURE: draws and inserts a menu ***************************/
void	MenuBarInit( void )
{
	Handle		menuBar;
	MenuHandle	menu;

	menuBar = GetNewMBar( 128 );		// fetches MBAR resource (from ID=128)
	SetMenuBar( menuBar );

	menu = GetMHandle( 128 );			// Inserts Apple Menu (from ID=128)
	AddResMenu( menu, 'DRVR' );

	DrawMenuBar();

}

/********************* PROCEDURE: loads opening screen *********************************/
void	LoadScreen( void )
{
	Rect		pictureRect;
	WindowPtr	window;
	PicHandle	picture;
	ControlHandle	button;				//for button support

	window = FrontWindow();

	pictureRect = window->portRect;

	picture = GetPicture (k1stScreenID);
	
	if ( picture == nil )							// in case resource didn't load
	{
		SysBeep( 10 );
		ExitToShell();
	}

	CenterPict( picture, &pictureRect );			//  My Routine
	DrawPicture( picture, &pictureRect );			//  ROM Routine
}

/********************* PROCEDURE: sets recs and stuff *********************************/
void	InitProgram( void )
{
	Cursor		curWatch;
	CursHandle	cursH;
	
	cursH = GetCursor( watchCursor );
	HLock((Handle) cursH);
	curWatch = **cursH;
	SetCursor( &curWatch );
	HUnlock((Handle) cursH);
	
	//				    L  T  R   B
	SetRect( &gQuitRect, 1, 1, 95, 31 );
	SetRect( &gCreateRect, 1, 1, 95, 31 );
	SetRect( &gClearRect, 1, 1, 95, 31 );
	SetRect( &gScanRect, 1, 1, 95, 31 );
	SetRect( &gFloatRect, 487, 101, 545, 132 );
	SetRect( &gModule1Rect, 1, 1, 59, 32 );
	SetRect( &gModule2Rect, 1, 1, 59, 32 );
	SetRect( &gModule3Rect, 1, 1, 59, 32 );
	SetRect( &gModule4Rect, 1, 1, 59, 32 );
	SetRect( &gModule5Rect, 1, 1, 59, 32 );
	SetRect( &gModule6Rect, 1, 1, 59, 32 );
	SetRect( &gModule7Rect, 1, 1, 59, 32 );
	SetRect( &gBlankModuleRect, 1, 1, 68, 166 );
	SetRect( &gRAID0Rect, 1, 1, 352, 21 );
	SetRect( &gRAID0Mod1Rect, 1, 1, 48, 16 );
	SetRect( &gRAID0Mod2Rect, 1, 1, 48, 16 );
	SetRect( &gRAID0Mod3Rect, 1, 1, 48, 16 );
	SetRect( &gRAID0Mod4Rect, 1, 1, 48, 16 );
	SetRect( &gRAID0Mod5Rect, 1, 1, 48, 16 );
	SetRect( &gRAID0Mod6Rect, 1, 1, 48, 16 );
	SetRect( &gRAID0Mod7Rect, 1, 1, 48, 16 );
	SetRect( &gMouseCoordsRect, 1, 1, 59, 32 );
	SetRect( &gStatusRect, 1, 1, 451, 132 );
	SetRect( &gMsgTextRect, 1, 1, 251, 33 );
	SetRect(&icon1Rect, 1,1, 33, 33);
	SetRect(&icon2Rect, 1,1, 33, 33);
	SetRect(&icon3Rect, 1,1, 33, 33);
	SetRect(&icon4Rect, 1,1, 33, 33);
	SetRect(&icon5Rect, 1,1, 33, 33);
	SetRect(&icon6Rect, 1,1, 33, 33);
	//					   L   T
	OffsetRect(&gCreateRect, 496, 307);
	OffsetRect(&gClearRect, 496, 345);
	OffsetRect(&gQuitRect, 496, 383);
	OffsetRect(&gScanRect, 343, 29);
	OffsetRect(&gModule1Rect, 487, 101);
	OffsetRect(&gModule2Rect, 487, 123);
	OffsetRect(&gModule3Rect, 487, 145);
	OffsetRect(&gModule4Rect, 488, 165);
	OffsetRect(&gModule5Rect, 488, 186);
	OffsetRect(&gModule6Rect, 488, 207);
	OffsetRect(&gModule7Rect, 488, 227);
	OffsetRect(&gBlankModuleRect, 484, 96);
	OffsetRect(&gRAID0Rect, 84, 308);
	OffsetRect(&gRAID0Mod1Rect, 87, 311);
	OffsetRect(&gRAID0Mod2Rect, 137, 311);
	OffsetRect(&gRAID0Mod3Rect, 187, 311);
	OffsetRect(&gRAID0Mod4Rect, 237, 311);
	OffsetRect(&gRAID0Mod5Rect, 287, 311);
	OffsetRect(&gRAID0Mod6Rect, 337, 311);
	OffsetRect(&gRAID0Mod7Rect, 387, 311);
	OffsetRect(&gStatusRect, 25, 297);
	OffsetRect(&gMsgTextRect, 225, 40);
	
	OffsetRect(&icon1Rect, 466, 157);
	OffsetRect(&icon2Rect, 503, 160);
	OffsetRect(&icon3Rect, 537, 163);
	OffsetRect(&icon4Rect, 466, 211);
	OffsetRect(&icon5Rect, 503, 214);
	OffsetRect(&icon6Rect, 537, 217);
	
	quitPressedPic = GetPicture (129);
	clearPressedPic = GetPicture (130);
	createPressedPic = GetPicture (131);
	module1Pic = GetPicture (133);
	module2Pic = GetPicture (134);
	module3Pic = GetPicture (135);
	module4Pic = GetPicture (136);
	module5Pic = GetPicture (137);
	module6Pic = GetPicture (138);
	module7Pic = GetPicture (139);
	scanPic = GetPicture (140);
	scanClearPic = GetPicture (141);
	blankModulePic = GetPicture (142);
	platterPic = GetPicture (143);
	RAID0StripPic = GetPicture (144);
	statusPic = GetPicture (145);
	gCreateGrayPic = GetPicture (146);
	gClearGrayPic = GetPicture (147);
	module2BlankPic = GetPicture (149);
	module3BlankPic = GetPicture (150);
	module4BlankPic = GetPicture (151);
	module5BlankPic = GetPicture (152);
	module6BlankPic = GetPicture (153);
	module7BlankPic = GetPicture (154);
	
	gMod1Rgn = NewRgn();					//for dragNdrop support		
	OpenRgn();
		MoveTo(487, 101);
		Line(58,0);
		Line(0,31);
		Line(-58,0);
		Line(0,-31);
	CloseRgn(gMod1Rgn);
	
	gMod2Rgn = NewRgn();					//for dragNdrop support		
	OpenRgn();
		MoveTo(487, 123);
		Line(58,0);
		Line(0,31);
		Line(-58,0);
		Line(0,-31);
	CloseRgn(gMod2Rgn);
	
	gMod3Rgn = NewRgn();					//for dragNdrop support		
	OpenRgn();
		MoveTo(487, 145);
		Line(58,0);
		Line(0,31);
		Line(-58,0);
		Line(0,-31);
	CloseRgn(gMod3Rgn);
	
	gMod4Rgn = NewRgn();					//for dragNdrop support		
	OpenRgn();
		MoveTo(487, 165);
		Line(58,0);
		Line(0,31);
		Line(-58,0);
		Line(0,-31);
	CloseRgn(gMod4Rgn);
	
	gMod5Rgn = NewRgn();					//for dragNdrop support		
	OpenRgn();
		MoveTo(487, 186);
		Line(58,0);
		Line(0,31);
		Line(-58,0);
		Line(0,-31);
	CloseRgn(gMod5Rgn);
	
	gMod6Rgn = NewRgn();					//for dragNdrop support		
	OpenRgn();
		MoveTo(487, 207);
		Line(58,0);
		Line(0,31);
		Line(-58,0);
		Line(0,-31);
	CloseRgn(gMod6Rgn);
	
	gMod7Rgn = NewRgn();					//for dragNdrop support		
	OpenRgn();
		MoveTo(487, 227);
		Line(58,0);
		Line(0,31);
		Line(-58,0);
		Line(0,-31);
	CloseRgn(gMod7Rgn);
}
	

/********************* PROCEDURE: sets recs and stuff *********************************/
void	InitProgramICU( void )
{
	OSErr 	err1 = 0;						// serial code
	
	inRefNum = outRefNum = 0;				// serial code
	
	err1 = serialinit();					// serial code
	if (err1 != 0)
	{
		SysBeep(10);
		Dialog("\pThe serial initializations have failed.");
	}
	else
	{
		FindWayBack();
					//GoToCreateRAID();
				
					// choose menu option AreDrivesThere
					//AreDrivesThere();
					
		
		GoToEnclosureStatus();
		WhichSlotsFilled();
	}
}

/************************** PROCEDURE: looks for modules *************************/
void	ModuleScan( void )
{
	Cursor		curWatch;
	CursHandle	cursH;
	
	cursH = GetCursor( watchCursor );
	HLock((Handle) cursH);
	curWatch = **cursH;
	SetCursor( &curWatch );
	HUnlock((Handle) cursH);
	
		DrawPicture( scanPic, &gScanRect );
		Delay(kScanDelay,0);
		DrawPicture( scanClearPic, &gScanRect );
		Delay(kScanDelay,0);
		
		DrawPicture( module1Pic, &gModule1Rect );
		gModule1Present = TRUE;							//modulePresent variable is
		gMod1ID = 0;									// for ReDrawPicture()
		DrawDDStrings(gMod1ID);
		DrawPicture( scanPic, &gScanRect );
		Delay(kScanDelay,0);
		DrawPicture( scanClearPic, &gScanRect );
		Delay(kScanDelay,0);
		
		DrawPicture( module2Pic, &gModule2Rect );
		gModule2Present = TRUE;
		gMod2ID = 1;
		DrawDDStrings(gMod2ID);
		DrawPicture( scanPic, &gScanRect );
		Delay(kScanDelay,0);
		DrawPicture( scanClearPic, &gScanRect );
		Delay(kScanDelay,0);
		
		DrawPicture( module3Pic, &gModule3Rect );
		gModule3Present = TRUE;
		gMod3ID = 2;
		DrawDDStrings(gMod3ID);
		DrawPicture( scanPic, &gScanRect );
		Delay(kScanDelay,0);
		DrawPicture( scanClearPic, &gScanRect );
		Delay(kScanDelay,0);
		
		DrawPicture( module4Pic, &gModule4Rect );
		gModule4Present = TRUE;
		gMod4ID = 3;
		DrawDDStrings(gMod4ID);
		DrawPicture( scanPic, &gScanRect );
		Delay(kScanDelay,0);
		DrawPicture( scanClearPic, &gScanRect );
		Delay(kScanDelay,0);
		
		DrawPicture( module5Pic, &gModule5Rect );
		gModule5Present = TRUE;
		gMod5ID = 4;
		DrawDDStrings(gMod5ID);
		DrawPicture( scanPic, &gScanRect );
		Delay(kScanDelay,0);
		DrawPicture( scanClearPic, &gScanRect );
		Delay(kScanDelay,0);
		
		DrawPicture( module6Pic, &gModule6Rect );
		gModule6Present = TRUE;
		gMod6ID = 5;
		DrawDDStrings(gMod6ID);
		DrawPicture( scanPic, &gScanRect );
		Delay(kScanDelay,0);
		DrawPicture( scanClearPic, &gScanRect );
		Delay(kScanDelay,0);
		
		DrawPicture( module7Pic, &gModule7Rect );
		gModule7Present = TRUE;
		gMod7ID = 6;
		DrawDDStrings(gMod7ID);
		
	InitCursor();
}


/************************************  ***********************************/
void AreDrivesThere(void)
{
	char	*stringID0 = "12;4H";
	char	*stringID1 = "12;9H";
	char	*stringID2 = "12;14H";
	char	*stringID3 = "12;19H";
	char	*stringID4 = "12;24H";
	char	*stringID5 = "12;29H";
	char	*stringID6 = "12;34H";
	char	*stringID7 = "12;39H";
	char	*stringID8 = "12;44H";
	char	*stringID9 = "12;49H";
	char	*stringID10 = "12;54H";
	char	*stringID11 = "12;59H";
	char	*stringID12 = "12;64H";
	char	*stringID13 = "12;69H";
	char	*stringID14 = "12;74H";
	char	*stringID15 = "12;79H";
	
	char	*stringID0CHB = "19;4H";
	char	*stringID1CHB = "19;9H";
	char	*stringID2CHB = "19;14H";
	char	*stringID3CHB = "19;19H";
	char	*stringID4CHB = "19;24H";
	char	*stringID5CHB = "19;29H";
	char	*stringID6CHB = "19;34H";
	char	*stringID7CHB = "19;39H";
	char	*stringID8CHB = "19;44H";
	char	*stringID9CHB = "19;49H";
	char	*stringID10CHB = "19;54H";
	char	*stringID11CHB = "19;59H";
	char	*stringID12CHB = "19;64H";
	char	*stringID13CHB = "19;69H";
	char	*stringID14CHB = "19;74H";
	char	*stringID15CHB = "19;79H";
	char	*subString2 = "Press Enter Key";
	char	*result;
	char	*result2;
	
	if (result = strstr(inbuf, stringID0)) 
	{
		//Dialog("\pDrive at ID 0, Channel A.");
	}
	if (result = strstr(inbuf, stringID0CHB)) 
	{
		//Dialog("\pDrive at ID 0, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID1)) 
	{
		//Dialog("\pDrive at ID 1, Channel A.");
	}
	if (result = strstr(inbuf, stringID1CHB)) 
	{
		//Dialog("\pDrive at ID 1, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID2)) 
	{
		//Dialog("\pDrive at ID 2, Channel A.");
	}
	if (result = strstr(inbuf, stringID2CHB)) 
	{
		//Dialog("\pDrive at ID 2, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID3)) 
	{
		//Dialog("\pDrive at ID 3, Channel A.");
	}
	if (result = strstr(inbuf, stringID3CHB)) 
	{
		//Dialog("\pDrive at ID 3, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID4)) 
	{
		//Dialog("\pDrive at ID 4, Channel A.");
	}
	if (result = strstr(inbuf, stringID4CHB)) 
	{
		//Dialog("\pDrive at ID 4, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID5)) 
	{
		//Dialog("\pDrive at ID 5, Channel A.");
	}
	if (result = strstr(inbuf, stringID5CHB)) 
	{
		//Dialog("\pDrive at ID 5, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID6)) 
	{
		//Dialog("\pDrive at ID 6, Channel A.");
	}
	if (result = strstr(inbuf, stringID6CHB)) 
	{
		//Dialog("\pDrive at ID 6, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID7)) 
	{
		//Dialog("\pDrive at ID 7, Channel A.");
	}
	if (result = strstr(inbuf, stringID7CHB)) 
	{
		//Dialog("\pDrive at ID 7, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID8)) 
	{
		//Dialog("\pDrive at ID 8, Channel A.");
	}
	if (result = strstr(inbuf, stringID8CHB)) 
	{
		//Dialog("\pDrive at ID 8, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID9)) 
	{
		//Dialog("\pDrive at ID 9, Channel A.");
	}
	if (result = strstr(inbuf, stringID9CHB)) 
	{
		//Dialog("\pDrive at ID 9, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID10)) 
	{
		//Dialog("\pDrive at ID 10, Channel A.");
	}
	if (result = strstr(inbuf, stringID10CHB)) 
	{
		//Dialog("\pDrive at ID 10, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID11)) 
	{
		//Dialog("\pDrive at ID 11, Channel A.");
	}
	if (result = strstr(inbuf, stringID11CHB)) 
	{
		//Dialog("\pDrive at ID 11, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID12)) 
	{
		//Dialog("\pDrive at ID 12, Channel A.");
	}
	if (result = strstr(inbuf, stringID12CHB)) 
	{
		//Dialog("\pDrive at ID 12, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID13)) 
	{
		//Dialog("\pDrive at ID 13, Channel A.");
	}
	if (result = strstr(inbuf, stringID13CHB)) 
	{
		//Dialog("\pDrive at ID 13, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID14)) 
	{
		//Dialog("\pDrive at ID 14, Channel A.");
	}
	if (result = strstr(inbuf, stringID14CHB)) 
	{
		//Dialog("\pDrive at ID 14, Channel B.");
	}
	
	
	if (result = strstr(inbuf, stringID15)) 
	{
		//Dialog("\pDrive at ID 15, Channel A.");
	}
	if (result = strstr(inbuf, stringID15CHB)) 
	{
		//Dialog("\pDrive at ID 15, Channel B.");
	}
	
	
	result2 = strstr(inbuf, subString2);
	if (result2 != NULL) 
	{
		Dialog("\pNo drives found.");
		return;
	}
	
	// these items will not be executed if no drives are found because of a "return"
	GoToEnclosureStatus();
	WhichSlotsFilled();
}

/********************** draws ID & capacity of modules *************************/
void	DrawDDStrings(short IDnum)
{
	Str255	aString;
	
	ForeColor(blueColor);
	TextSize(10);
	NumToString((long)IDnum,aString);
	switch( IDnum )
	{
		case 0:									// ID = 0
			MoveTo(355,115);
			break;
		case 1:									// ID = 1
			MoveTo(355,139);
			break;
		case 2:									// ID = 2
			MoveTo(355,163);
			break;
		case 3:									// ID = 3
			MoveTo(355,187);
			break;
		case 4:									// ID = 4
			MoveTo(355,211);
			break;
		case 5:									// ID = 5
			MoveTo(355,235);
			break;
		case 6:									// ID = 6
			MoveTo(355,258);
			break;
	}
	//DrawString("\pID ");
	//DrawString(aString);
	//DrawString("\p 4GB");
	DrawString("\pChannel A, 4 GB");
	
	ForeColor(blackColor);
}

/********************** PROCEDURE: centers picture in window ***************************/
void	CenterPict( PicHandle picture, Rect *destRectPtr )
{
	Rect	windRect, pictRect;
	
	windRect = *destRectPtr;
	pictRect = (**( picture )).picFrame;
	OffsetRect( &pictRect, windRect.left - pictRect.left,
							windRect.top - pictRect.top);
	OffsetRect( &pictRect, (windRect.right - 
							pictRect.right)/2, (windRect.bottom - 
							pictRect.bottom)/2);
	*destRectPtr = pictRect;
}

/************************* PROCEDURE: waits for an event *******************************/
void	EventLoop( void )
{
	EventRecord		event;
	
	gDone = false;
	
	while ( gDone == false )
	{
		if ( WaitNextEvent( everyEvent, &event, 20L, nil ))
			DoEvent( &event );
		else
			HandleNull( &event );
	}
}

/************************** PROCEDURE: doEvent *****************************************/
void	DoEvent( EventRecord *eventPtr )
{
	char	theChar;

	switch( eventPtr->what )
	{
		case mouseDown:									/* Handles MouseDown */
			HandleMouseDown( eventPtr );
			break;
			
		case mouseUp:									/* Handles MouseDown */
			HandleMouseUp( eventPtr );
			break;
			
		case keyDown:									/* Handles Command Keys */
		case autoKey:
			theChar = eventPtr->message & charCodeMask;
			if (theChar == '9') DriveFail();
			if (theChar == '8') ResetFail();
			if (theChar == '1')
			{
				cleanup();
				gDone = true;
			}
			if ( (eventPtr->modifiers & cmdKey) != 0 )
				HandleMenuChoice( MenuKey( theChar ) );
			break;
			
		case updateEvt:									/* Updates Windows */
			DoUpdate( eventPtr );
			break;
	}
}

/************************** PROCEDURE: HandleNull **************************************/
void	HandleNull( EventRecord *eventPtr )
{
	
}

/************************** PROCEDURE: HandleMouseDown *********************************/
void	HandleMouseDown( EventRecord *eventPtr )
{
	WindowPtr		whichWindow;
	short			thePart;
	long			menuChoice, refNum;
	Point			rectPoint, thePoint, finalPoint;	/*for button support*/
	ControlHandle	theControl, refNumber;				/*for button support*/
	Point			tempPoint, tempPoint2;
	Boolean			InRect;
	RgnHandle		tempRgn;					//for dragNdrop support
	Rect			RAID0TextRect;
	
	thePart = FindWindow( eventPtr->where, &whichWindow );
	switch( thePart )
	{
		case inDrag:
			DragWindow( whichWindow, eventPtr->where, &screenBits.bounds );
		case inMenuBar:
			menuChoice = MenuSelect( eventPtr->where );
			HandleMenuChoice( menuChoice );
			break;
		/*for button support*/
		case inContent:
			thePoint = eventPtr->where;
			GlobalToLocal(&thePoint);
			rectPoint = eventPtr->where;
			GlobalToLocal(&rectPoint);
			thePart = FindControl( thePoint, whichWindow, &theControl);
			/*if (theControl != 0)
			{
				thePart = TrackControl( theControl, thePoint, nil );
				if ( thePart == inButton )		// check partcode
				{
					refNum = GetCRefCon(theControl);
					if (refNum == 0)	//gestalt = 0, SCSI calls = 1
					{
						
					}
					else if (refNum == 1)
					{
						SCSIRoutine();
					}
				}
			}*/
			
			
			
			//module 1							for dragNdrop support
			if (PtInRect(rectPoint, &gModule1Rect) && gModule1Used == FALSE && gModule1Present == TRUE)
			{
				tempRgn = NewRgn();
				CopyRgn(gMod1Rgn, tempRgn);
				GetMouse(&tempPoint2);
				factorH = tempPoint2.h - 487;
				factorV = tempPoint2.v - 101;
				//gCoords = DragGrayRgn( tempRgn, rectPoint, &gMainPort->portRect, 
				//		&gMainPort->portRect, 0, &dragAction);
				gCoords = DragGrayRgn( tempRgn, rectPoint, &gMainPort->portRect, 
						&gMainPort->portRect, 0, nil);
				CopyRgn(gMod1Rgn, tempRgn);
				
				GetMouse(&finalPoint);
				if (PtInRect(finalPoint, &gRAID0Rect ))
				{
					FillRAID0("\pID0 4GB");
					gModule1Used = TRUE;
				}
				
				factorH = 0;
				factorV = 0;
				// more code here for RAID1, RAID3, RAID5, RAID6
				
				return;
			}
			
			//module 2							for dragNdrop support
			if (PtInRect(rectPoint, &gModule2Rect ) && gModule2Used == FALSE && gModule2Present == TRUE)
			{
				tempRgn = NewRgn();
				CopyRgn(gMod2Rgn, tempRgn);
				gCoords = DragGrayRgn( tempRgn, rectPoint, &gMainPort->portRect, 
						&gMainPort->portRect, 0, nil);
				CopyRgn(gMod2Rgn, tempRgn);
				
				GetMouse(&finalPoint);
				if (PtInRect(finalPoint, &gRAID0Rect ))
				{
					FillRAID0("\pID1 4GB");
					gModule2Used = TRUE;
				}
				
				// more code here for RAID1, RAID3, RAID5, RAID6
				
				return;
			}
			
			//module 3							for dragNdrop support
			if (PtInRect(rectPoint, &gModule3Rect ) && gModule3Used == FALSE && gModule3Present == TRUE)
			{
				tempRgn = NewRgn();
				CopyRgn(gMod3Rgn, tempRgn);
				gCoords = DragGrayRgn( tempRgn, rectPoint, &gMainPort->portRect, 
						&gMainPort->portRect, 0, nil);
				CopyRgn(gMod3Rgn, tempRgn);
				
				GetMouse(&finalPoint);
				if (PtInRect(finalPoint, &gRAID0Rect ))
				{
					FillRAID0("\pID2 4GB");
					gModule3Used = TRUE;
				}
				
				// more code here for RAID1, RAID3, RAID5, RAID6
				
				return;
			}
			
			//module 4							for dragNdrop support
			if (PtInRect(rectPoint, &gModule4Rect ) && gModule4Used == FALSE && gModule4Present == TRUE)
			{
				tempRgn = NewRgn();
				CopyRgn(gMod4Rgn, tempRgn);
				gCoords = DragGrayRgn( tempRgn, rectPoint, &gMainPort->portRect, 
						&gMainPort->portRect, 0, nil);
				CopyRgn(gMod4Rgn, tempRgn);
				
				GetMouse(&finalPoint);
				if (PtInRect(finalPoint, &gRAID0Rect ))
				{
					FillRAID0("\pID3 4GB");
					gModule4Used = TRUE;
				}
				
				// more code here for RAID1, RAID3, RAID5, RAID6
				
				return;
			}
			
			//module 5							for dragNdrop support
			if (PtInRect(rectPoint, &gModule5Rect ) && gModule5Used == FALSE && gModule5Present == TRUE)
			{
				tempRgn = NewRgn();
				CopyRgn(gMod5Rgn, tempRgn);
				gCoords = DragGrayRgn( tempRgn, rectPoint, &gMainPort->portRect, 
						&gMainPort->portRect, 0, nil);
				CopyRgn(gMod5Rgn, tempRgn);
				
				GetMouse(&finalPoint);
				if (PtInRect(finalPoint, &gRAID0Rect ))
				{
					FillRAID0("\pID4 4GB");
					gModule5Used = TRUE;
				}
				
				// more code here for RAID1, RAID3, RAID5, RAID6
				
				return;
			}
			
			//module 6							for dragNdrop support
			if (PtInRect(rectPoint, &gModule6Rect ) && gModule6Used == FALSE && gModule6Present == TRUE)
			{
				tempRgn = NewRgn();
				CopyRgn(gMod6Rgn, tempRgn);
				gCoords = DragGrayRgn( tempRgn, rectPoint, &gMainPort->portRect, 
						&gMainPort->portRect, 0, nil);
				CopyRgn(gMod6Rgn, tempRgn);
				
				GetMouse(&finalPoint);
				if (PtInRect(finalPoint, &gRAID0Rect ))
				{
					FillRAID0("\pID5 4GB");
					gModule6Used = TRUE;
				}
				
				// more code here for RAID1, RAID3, RAID5, RAID6
				
				return;
			}
			
			//module 7							for dragNdrop support
			if (PtInRect(rectPoint, &gModule7Rect ) && gModule7Used == FALSE && gModule7Present == TRUE)
			{
				tempRgn = NewRgn();
				CopyRgn(gMod7Rgn, tempRgn);
				gCoords = DragGrayRgn( tempRgn, rectPoint, &gMainPort->portRect, 
						&gMainPort->portRect, 0, nil);
				CopyRgn(gMod7Rgn, tempRgn);
				
				GetMouse(&finalPoint);
				if (PtInRect(finalPoint, &gRAID0Rect ))
				{
					FillRAID0("\pID6 4GB");
					gModule7Used = TRUE;
				}
				
				// more code here for RAID1, RAID3, RAID5, RAID6
				
				return;
			}
			
			
			
			//quit button
			if (PtInRect(rectPoint, &gQuitRect ))
			{
				DrawPicture( quitPressedPic, &gQuitRect );
				
				
				DrawPicture( quitPressedPic, &gQuitRect );
				InRect	=	TRUE;
				do
				{
					GetMouse(&tempPoint);
					if (InRect)
					{
						if (PtInRect(tempPoint, &gQuitRect) == FALSE)
						{
							InRect	=	FALSE;		/*** This is my out button hack  ****/
							InvalRect( &gQuitRect );		/*** This is my out button hack  ****/
							BeginUpdate(gMainWindow);
							ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
							EndUpdate(whichWindow);
						}
					}
					else
					{
						if (PtInRect(tempPoint, &gQuitRect))
						{
							InRect	=	TRUE;
							DrawPicture( quitPressedPic, &gQuitRect );
						}
					}
				} while (StillDown());
				InvalRect( &gQuitRect );		/*** This is my out button hack  ****/
				BeginUpdate(gMainWindow);
				ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
				EndUpdate(whichWindow);
				if (InRect) gDone = true;
				return;
			}
			
			//create button
			if (PtInRect(rectPoint, &gCreateRect ) && gCreateButEnabled == TRUE)
			{
				DrawPicture( createPressedPic, &gCreateRect );
				
				
				DrawPicture( createPressedPic, &gCreateRect );
				InRect	=	TRUE;
				do
				{
					GetMouse(&tempPoint);
					if (InRect)
					{
						if (PtInRect(tempPoint, &gCreateRect) == FALSE)
						{
							InRect	=	FALSE;		/*** This is my out button hack  ****/
							InvalRect( &gCreateRect );		/*** This is my out button hack  ****/
							BeginUpdate(gMainWindow);
							ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
							EndUpdate(whichWindow);
						}
					}
					else
					{
						if (PtInRect(tempPoint, &gCreateRect))
						{
							InRect	=	TRUE;
							DrawPicture( createPressedPic, &gCreateRect );
						}
					}
				} while (StillDown());
				InvalRect( &gCreateRect );		/*** This is my out button hack  ****/
				BeginUpdate(gMainWindow);
				ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
				EndUpdate(whichWindow);
				if (InRect) create();
				return;
			}
			
			//clear button
			if (PtInRect(rectPoint, &gClearRect ) && gClearButEnabled == TRUE)
			{
				DrawPicture( clearPressedPic, &gClearRect );
				
				
				DrawPicture( clearPressedPic, &gClearRect );
				InRect	=	TRUE;
				do
				{
					GetMouse(&tempPoint);
					if (InRect)
					{
						if (PtInRect(tempPoint, &gClearRect) == FALSE)
						{
							InRect	=	FALSE;		/*** This is my out button hack  ****/
							InvalRect( &gClearRect );		/*** This is my out button hack  ****/
							BeginUpdate(gMainWindow);
							ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
							EndUpdate(whichWindow);
						}
					}
					else
					{
						if (PtInRect(tempPoint, &gClearRect))
						{
							InRect	=	TRUE;
							DrawPicture( clearPressedPic, &gClearRect );
						}
					}
				} while (StillDown());
				InvalRect( &gClearRect );		/*** This is my out button hack  ****/
				BeginUpdate(gMainWindow);
				ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
				EndUpdate(whichWindow);
				if (InRect)
				{
					InvalRect(&gRAID0Rect);
					gModule1Used = FALSE;
					gModule2Used = FALSE;
					gModule3Used = FALSE;
					gModule4Used = FALSE;
					gModule5Used = FALSE;
					gModule6Used = FALSE;
					gModule7Used = FALSE;
					gRAID0Mod1Used = FALSE;
					gRAID0Mod2Used = FALSE;
					gRAID0Mod3Used = FALSE;
					gRAID0Mod4Used = FALSE;
					gRAID0Mod5Used = FALSE;
					gRAID0Mod6Used = FALSE;
					gRAID0Mod7Used = FALSE;
					gRAID0Str = FALSE;
					//clear "RAID 0" text string
					SetRect(&RAID0TextRect, 31, 313, 81, 326);
					InvalRect(&RAID0TextRect);
					gCreateButEnabled = FALSE;
					gClearButEnabled = FALSE;
					InvalRect(&gCreateRect);
					InvalRect(&gClearRect);
				}
				return;
			}
			
			
			break;
	}
}



/********************************* for dragNdrop support *********************************/
void	InRAIDRect( void )				// for dragNdrop support
{	
	Point	tempPoint;
	Rect	tempRect;
	RgnHandle	tempRgn;
	
	tempRgn = NewRgn();					//for dragNdrop support		
	CopyRgn(gMod1Rgn,tempRgn);
	
	GetMouse(&tempPoint);
	
	//CopyBits(&srcMap, &destMap, &srcRect, &destRect, tMode, maskRgn);
	RectRgn(tempRgn, &tempRect);
			
	OffsetRect(&gFloatRect, (tempPoint.h - gFloatRect.left) - factorH, (tempPoint.v- gFloatRect.top) - factorV);
	DrawPicture( platterPic, &gFloatRect );
	
	//DrawMouseCoords(tempRect);
	
	//if (PtInRect(tempPoint, &gRAID0Rect ))
	//{
	//	//ForeColor(blueColor);
	//	FrameRect(&gRAID0Mod1Rect);
	//}
	
	DisposeRgn(tempRgn);
}


/************************** Draw points and coords *********************************/
void	DrawMouseCoords( Rect tempRect )
{
	long	lo;
	Str255	theString;
	
	DrawPicture( scanClearPic, &gMouseCoordsRect );
	
	//lo = (long)tempPoint.h;
	//lo = (long)gFloatBeforeRect.top;
	lo = (long)tempRect.left;
	//lo = HiWord(gCoords);
	NumToString(lo, theString);
	MoveTo(10,10);
	DrawString("\pL = ");
	DrawString(theString);
	
	//lo = (long)tempPoint.v;
	//lo = (long)gFloatBeforeRect.left;
	lo = (long)tempRect.top;
	//lo = LoWord(gCoords);
	NumToString(lo, theString);
	MoveTo(10,20);
	DrawString("\pt = ");
	DrawString(theString);
}


/************************** Fills RAID 0 strip *********************************/
void	FillRAID0( Str255 stringA )
{
/*
	ForeColor(blueColor);
	
	if (gRAID0Mod6Used == TRUE)
	{
		DrawPicture(RAID0StripPic,&gRAID0Mod7Rect);
		MoveTo(391,323);
		strcpy( gMod7String, stringA );
		DrawString(gMod7String);
		gRAID0Mod7Used = TRUE;
	}
	else
	{
		if (gRAID0Mod5Used == TRUE)
		{
			DrawPicture(RAID0StripPic,&gRAID0Mod6Rect);
			MoveTo(341,323);
			strcpy( gMod6String, stringA );
			DrawString(gMod6String);
			gRAID0Mod6Used = TRUE;
		}
		else
		{
			if (gRAID0Mod4Used == TRUE)
			{
				DrawPicture(RAID0StripPic,&gRAID0Mod5Rect);
				MoveTo(291,323);
				strcpy( gMod5String, stringA );
				DrawString(gMod5String);
				gRAID0Mod5Used = TRUE;
			}
			else
			{
				if (gRAID0Mod3Used == TRUE)
				{
					DrawPicture(RAID0StripPic,&gRAID0Mod4Rect);
					MoveTo(241,323);
					strcpy( gMod4String, stringA );
					DrawString(gMod4String);
					gRAID0Mod4Used = TRUE;
				}
				else
				{
					if (gRAID0Mod2Used == TRUE)
					{
						DrawPicture(RAID0StripPic,&gRAID0Mod3Rect);
						MoveTo(191,323);
						strcpy( gMod3String, stringA );
						DrawString(gMod3String);
						gRAID0Mod3Used = TRUE;
					}
					else
					{
						if (gRAID0Mod1Used == TRUE)
						{
							DrawPicture(RAID0StripPic,&gRAID0Mod2Rect);
							MoveTo(141,323);
							strcpy( gMod2String, stringA );
							DrawString(gMod2String);
							gRAID0Mod2Used = TRUE;
							gCreateButEnabled = TRUE;
							InvalRect(&gCreateRect);
						}
						else
						{
							if (gRAID0Mod1Used == FALSE)
							{
								DrawPicture(RAID0StripPic,&gRAID0Mod1Rect);
								MoveTo(91,323);
								strcpy( gMod1String, stringA );
								DrawString(gMod1String);
								gRAID0Mod1Used = TRUE;
								gClearButEnabled = TRUE;
								InvalRect(&gClearRect);
								DrawRAID0Text();
							}
						}
					}
				}
			}
		}
	}

	ForeColor(blackColor);
	*/
}


/************************** HandleMouseUp *********************************/
void	DrawRAID0Text( void )
{
	short	curFont, curSize;
	Style	curFace;
	
				// Get defaults so you can restore after printing destination string
	curFont = thePort->txFont;
	curFace = thePort->txFace;
	curSize = thePort->txSize;
	
	TextFont( 2 );
	TextSize( 12 );
	TextFace( 0 );
	
	gRAID0Str = TRUE;
	MoveTo(36,324);
	DrawString("\pRAID 0");
	
							//restore default text styles
	TextFont( curFont );
	TextSize( curSize );
	TextFace( curFace );
}


/************************** HandleMouseUp *********************************/
void	HandleMouseUp( EventRecord *eventPtr )
{

}


/*************************** HandleMenuChoice *******************************/
void	HandleMenuChoice( long menuChoice )
{
	short	menu;
	short	item;
	
	if ( menuChoice !=0 )
	{
		menu = HiWord( menuChoice );
		item = LoWord( menuChoice );
		
		switch( menu )
		{
			case 128:							// handles Apple menu (ID=128)
				HandleAppleChoice( item );
				break;
				
			case 129:							// handles File menu (ID=129)
				HandleFileChoice( item );
				break;
				
			case 130:							// handles Status menu (ID=130)
				HandleStatusChoice( item );
				break;
				
			case 131:							// handles RAID Tools menu (ID=131)
				HandleRAIDToolsChoice( item );
				break;
				
			case 132:							// handles Advanced Options menu (ID=132)
				HandleAdvancedOptionsChoice( item );
				break;
				
		}
		HiliteMenu( 0 );
	}
}

/*************************** PROCEDURE: handles items selected from APPLE menu *********/
void	HandleAppleChoice( short item)
{
	MenuHandle		appleMenu;
	Str255			accName;
	short			accNumber;
	
	switch( item )
	{
		case 1:											/*  choose the "about" item */
			aboutBox();
			break;
			
		default:										/* choose desk accessories  */
			appleMenu = GetMHandle( 128 );		/* apple menu ID 128 */
			GetItem( appleMenu, item, accName );
			accNumber = OpenDeskAcc( accName );
			break;
	}
}

/*************************** PROCEDURE: handles items selected from FILE menu **********/
void	HandleFileChoice( short item )
{
	OSErr	myErr;
	
	switch( item )
	{	
		case 1:							//choose Shutdown System
			SysBeep(10);
			break;
			
		case 2:							//choose Restart System
			SysBeep(10);
			break;
			
		case 4:							//choose Quit
			cleanup();					// disposes serial port buf
			gDone = true;
			break;
	}
}

/*************************** PROCEDURE: handles items selected from Status menu **********/
void	HandleStatusChoice( short item )
{
	OSErr	myErr;
	
	switch( item )
	{	
		case 1:							//choose Enclosure Status
			SysBeep(10);
			break;
			
		case 2:							//choose RAID System Status
			RAIDStatus();
			break;
	}
}

/*************************** PROCEDURE: handles items selected from RAID Tools menu **********/
void	HandleRAIDToolsChoice( short item )
{
	OSErr	myErr;
	
	switch( item )
	{	
		case 1:							//choose Rescan RAID Channel
			DrawPicture( blankModulePic, &gBlankModuleRect );
			ModuleScan();
			break;
			
		case 2:							//choose Rebuild RAID Set
			SysBeep(10);
			break;
			
		case 3:							//choose Drop Drive from RAID Set
			SysBeep(10);
			break;
			
		case 4:							//choose Add/Drop Spare Drive
			SysBeep(10);
			break;
	}
}

/******************* PROCEDURE: handles items selected from Advanced Options menu **********/
void	HandleAdvancedOptionsChoice( short item )
{
	OSErr	myErr;
	
	switch( item )
	{	
		case 1:							//choose Host Setup
			SysBeep(10);
			break;
			
		case 2:							//choose Pager Setup
			SysBeep(10);
			break;
			
		case 3:							//choose Date/Time Setup
			SysBeep(10);
			break;
			
		case 4:							//choose Password Setup
			SysBeep(10);
			break;
			
		case 6:							//choose Drive Slot Setup
			SysBeep(10);
			break;
			
		case 7:							//choose Drive Setup
			SysBeep(10);
			break;
	}
}

/********************** show RAID status in status strip ****************************/
void	RAIDStatus( void )
{

	short	curFont, curSize;
	Style	curFace;
	
	DrawPicture( statusPic, &gStatusRect );
	
	// Get defaults so you can restore after printing destination string
	curFont = thePort->txFont;
	curFace = thePort->txFace;
	curSize = thePort->txSize;
	
	TextFont( 0 );
	TextSize( 12 );
	TextFace( 1 );
	
	ForeColor(magentaColor);
	if (gRAIDCreated == TRUE)
	{
		MoveTo(45, 327);
		DrawString("\pLun 0 = RAID Set 0. RAID Level 0. 54GB. RAID Status OK.");
		MoveTo(45, 341);
		DrawString("\pLun 1 = (not defined)");
		MoveTo(45, 355);
		DrawString("\pLun 2 = (not defined)");
		MoveTo(45, 369);
		DrawString("\pLun 3 = (not defined)");
		MoveTo(45, 383);
		DrawString("\pLun 4 = (not defined)");
		MoveTo(45, 397);
		DrawString("\pLun 5 = (not defined)");
		MoveTo(45, 411);
		DrawString("\pLun 6 = (not defined)");
		//MoveTo(45, 425);
		//DrawString("\pLun 7 = (not defined)");
		ForeColor(blackColor);
	}
	else
	{
		MoveTo(45, 337);
		DrawString("\pNo RAID sets defined.");
	}
	
	//restore default text styles
	TextFont( curFont );
	TextSize( curSize );
	TextFace( curFace );
}

/************************ PROCEDURE: updates the windows ***************************/
void	DoUpdate( EventRecord *eventPtr )
{
	short			pictureID;
	PicHandle		picture;
	WindowPtr		window;
	
	window = (WindowPtr)eventPtr->message;
	
	BeginUpdate( window );						/* ROM Routine */
	pictureID = GetWRefCon	( window );
	picture = GetPicture( pictureID );
	
	ReDrawPicture( window, picture );			/* My Routine  */	
	EndUpdate( window );						/* ROM Routine */
} 

/************************** PROCEDURE: redraws the window contents *********************/
void	ReDrawPicture( WindowPtr window, PicHandle picture )
{
	Rect			drawingClipRect, windowRect;
	RgnHandle		tempRgn;
	ControlHandle	control;						/*for button support*/
	short			i;
	
	SetPort( window );
	tempRgn = NewRgn();
	GetClip( tempRgn );
	
	drawingClipRect = window->portRect;
	
	windowRect = window ->portRect;
	CenterPict( picture, &windowRect );
	ClipRect( &drawingClipRect );
	DrawPicture( picture, &windowRect );
	
	SetClip( tempRgn );
	DisposeRgn( tempRgn );
	
	//ForeColor(blueColor);
	//FrameRect(&gMsgTextRect);
	
	if (gCreateButEnabled == FALSE)
	{
		DrawPicture( gCreateGrayPic, &gCreateRect );
	}
	
	if (gClearButEnabled == FALSE)
	{
		DrawPicture( gClearGrayPic, &gClearRect );
	}
	
	if (gRAID0Str == TRUE){DrawRAID0Text();};
	
	if (gFirstTime)
	{
		gFirstTime = FALSE;
		InitProgramICU();
		//ModuleScan();
	}
	else
	{
		if (gModule1Present) 
		{
			DrawPicture(module1Pic,&gModule1Rect);
			DrawDDStrings(0);
		}
		if (gModule2Present) 
		{
			DrawPicture(module2Pic,&gModule2Rect);
			DrawDDStrings(1);
		}
		if (gModule3Present) 
		{
			DrawPicture(module3Pic,&gModule3Rect);
			DrawDDStrings(2);
		}
		if (gModule4Present) 
		{
			DrawPicture(module4Pic,&gModule4Rect);
			DrawDDStrings(3);
		}
		if (gModule5Present) 
		{
			DrawPicture(module5Pic,&gModule5Rect);
			DrawDDStrings(4);
		}
		if (gModule6Present) 
		{
			DrawPicture(module6Pic,&gModule6Rect);
			DrawDDStrings(5);
		}
		if (gModule7Present) 
		{
			DrawPicture(module7Pic,&gModule7Rect);
			DrawDDStrings(6);
		}
	}
	
	ForeColor(blueColor);
	if (gRAID0Str == TRUE){DrawRAID0Text();};
	// redraw the RAID strip 0 with blue rects and strings
	if (gRAID0Mod1Used == TRUE)
	{
		DrawPicture(RAID0StripPic,&gRAID0Mod1Rect);
		MoveTo(91,323);
		DrawString(gMod1String);
	}
	if (gRAID0Mod2Used == TRUE)
	{
		DrawPicture(RAID0StripPic,&gRAID0Mod2Rect);
		MoveTo(141,323);
		DrawString(gMod2String);
	}
	if (gRAID0Mod3Used == TRUE)
	{
		DrawPicture(RAID0StripPic,&gRAID0Mod3Rect);
		MoveTo(191,323);
		DrawString(gMod3String);
	}
	if (gRAID0Mod4Used == TRUE)
	{
		DrawPicture(RAID0StripPic,&gRAID0Mod4Rect);
		MoveTo(241,323);
		DrawString(gMod4String);
	}
	if (gRAID0Mod5Used == TRUE)
	{
		DrawPicture(RAID0StripPic,&gRAID0Mod5Rect);
		MoveTo(291,323);
		DrawString(gMod5String);
	}
	if (gRAID0Mod6Used == TRUE)
	{
		DrawPicture(RAID0StripPic,&gRAID0Mod6Rect);
		MoveTo(341,323);
		DrawString(gMod6String);
	}
	if (gRAID0Mod7Used == TRUE)
	{
		DrawPicture(RAID0StripPic,&gRAID0Mod7Rect);
		MoveTo(391,323);
		DrawString(gMod7String);
	}
	ForeColor(blackColor);
	
	SetPort( gMainPort );				/*sets port to main window*/
}

/******************** PROCEDURE: draws the about box ***********************************/
void	aboutBox( void)
{
	WindowPtr		window;
	Rect			pictureRect;
	PicHandle		picture;

	window = GetNewWindow( kAbtScreenID, nil, (WindowPtr)-1L );
	
	if ( window == nil )	/* In case program couldn't load resource */
	{
		SysBeep( 10 );
		ExitToShell();
	}
	
	SetWRefCon ( window, (long) (kAbtScreenID) );	/*  ROM Routine  */
	ShowWindow( window );							/*  ROM Routine  */
	SetPort( window );								/*  ROM Routine  */
	
	/* loads the graphic portion */
	
	window = FrontWindow();

	pictureRect = window->portRect;

	picture = GetPicture ( kAbtScreenID );
	
	if ( picture == nil )				/* in case resource didn't load */
	{
		SysBeep( 10 );
		ExitToShell();
	}

	CenterPict( picture, &pictureRect );			/*  My Routine   */
	DrawPicture( picture, &pictureRect );			/*  ROM Routine  */
	while ( !Button() );
	DisposeWindow( window );						/*  ROM Routine  */
	
	SetPort(gMainPort);						/*sets current port back to window "main"*/
}

/******************** PROCEDURE: dialog box for restarting the system *************************/
void	restartSystem( void)
{
	WindowPtr		window;
	Rect			pictureRect;
	PicHandle		dialog3D;

	window = GetNewWindow( kAbtScreenID, nil, (WindowPtr)-1L );		//ID 127
	
	if ( window == nil )	// In case program couldn't load resource
	{
		SysBeep( 10 );
		ExitToShell();
	}
	
	SetWRefCon ( window, (long) (132) );			/*  ROM Routine  */
	ShowWindow( window );							/*  ROM Routine  */
	SetPort( window );								/*  ROM Routine  */
	
	/* loads the graphic portion */
	
	window = FrontWindow();

	pictureRect = window->portRect;

	dialog3D = GetPicture ( 132 );
	
	if ( dialog3D == nil )				/* in case resource didn't load */
	{
		SysBeep( 10 );
		ExitToShell();
	}

	CenterPict( dialog3D, &pictureRect );			/*  My Routine   */
	DrawPicture( dialog3D, &pictureRect );			/*  ROM Routine  */
	MoveTo(40,80);
	ForeColor(whiteColor);
	DrawString("\pSystem will be reset upon clicking OK.");
	MoveTo(41,81);
	ForeColor(blackColor);
	DrawString("\pSystem will be reset upon clicking OK.");
	MoveTo(70,110);
	ForeColor(whiteColor);
	DrawString("\pDo you really want to do this?");
	MoveTo(71,111);
	ForeColor(blackColor);
	DrawString("\pDo you really want to do this?");
	while ( !Button() );
	DisposeWindow( window );						/*  ROM Routine  */
	
	SetPort(gMainPort);						/*sets current port back to window "main"*/
}

/******************** PROCEDURE: dialog box for restarting the system *************************/
void	create( void)
{
	WindowPtr		window;
	Rect			pictureRect;
	PicHandle		dialog3D;

	window = GetNewWindow( kAbtScreenID, nil, (WindowPtr)-1L );		//ID 127
	
	if ( window == nil )	// In case program couldn't load resource
	{
		SysBeep( 10 );
		ExitToShell();
	}
	
	SetWRefCon ( window, (long) (132) );			/*  ROM Routine  */
	ShowWindow( window );							/*  ROM Routine  */
	SetPort( window );								/*  ROM Routine  */
	
	/* loads the graphic portion */
	
	window = FrontWindow();

	pictureRect = window->portRect;

	dialog3D = GetPicture ( 132 );
	
	if ( dialog3D == nil )				/* in case resource didn't load */
	{
		SysBeep( 10 );
		ExitToShell();
	}

	CenterPict( dialog3D, &pictureRect );			/*  My Routine   */
	DrawPicture( dialog3D, &pictureRect );			/*  ROM Routine  */
	MoveTo(75,65);
	ForeColor(whiteColor);
	DrawString("\pNow ready to create RAID set 0.");
	MoveTo(76,66);
	ForeColor(blueColor);
	DrawString("\pNow ready to create RAID set 0.");
	MoveTo(75,95);
	ForeColor(whiteColor);
	DrawString("\pRAID Level = 0.");
	MoveTo(76,96);
	ForeColor(blueColor);
	DrawString("\pRAID Level = 0.");
	MoveTo(75,125);
	ForeColor(whiteColor);
	DrawString("\pStripe Unit Size: ");
	MoveTo(76,126);
	ForeColor(blueColor);
	DrawString("\pStripe Unit Size: ");
	while ( !Button() );
	DisposeWindow( window );						/*  ROM Routine  */
	ForeColor(blackColor);
	
	gRAIDCreated = TRUE;
	
	SetPort(gMainPort);						/*sets current port back to window "main"*/
}

/*********************** PROCEDURE: does SCSI Inquiry ***********************/
OSErr	SCSIRoutine( void )
{
	short			myErr;
	short			compErr;
	short			compStat;
	short			compMsg;
	
	gCDBPtr = &gCDB.opCode;
	
	gCDB.opCode = kInquiryCmd;			// SCSI command code (0x00)
	gCDB.lun = 0;
	gCDB.pageCode = 0;
	gCDB.reserved = 0;
	gCDB.allocationLen = 36;					// max num of bytes target returns
	gCDB.control = 0;
	
	TIB[0].scOpcode = scNoInc;
	TIB[0].scParam1 = (unsigned long) myDataBuffer;		// pointer to data buffer
	TIB[0].scParam2 = 36;			// number of bytes to be transferred
	TIB[1].scOpcode = scStop;
	TIB[1].scParam1 = 0;
	TIB[1].scParam2 = 0;
	
	myErr = SCSIGet();
	if (myErr == 0)
	{
		myErr = SCSISelect( gMySCSIID );
	}
	if (myErr != 0)
	{
		ErrorText();
		return(myErr);
	}
	
	myErr = SCSICmd( (Ptr) gCDBPtr, 6 );
	if (myErr == 0)
	{
		myErr = SCSIRead( (Ptr)(TIB) );
		if (myErr != 0)
		{
			ErrorText();
			return(myErr);
		} 
	}
	
	compErr = SCSIComplete( &compStat, &compMsg, 300 );
	if (myErr != 0)
	{
		ErrorText();
		return(myErr);
	}
	
	SCSIText();
}

/*********************** PROCEDURE: writes text to the screen ***********************/
void	SCSIText( void )
{
	Rect			TextRect;	
	
	//SetRect( &TextRect, 30, 250, 220, 300 );
	//				     L  T    R    B
	SetRect( &TextRect, 25, 250, 480, 300 );
	EraseRect( &TextRect );
	
	TextSize( 12 );
	MoveTo( 30, 260 );
	ForeColor( blueColor);
	
	DrawString( "\pManufacturer: " );
	//DrawChar(myDataBuffer[0]);
	//DrawChar(myDataBuffer[1]);
	//DrawChar(myDataBuffer[2]);
	//DrawChar(myDataBuffer[3]);
	//DrawChar(myDataBuffer[4]);
	//DrawChar(myDataBuffer[5]);
	//DrawChar(myDataBuffer[6]);
	//DrawChar(myDataBuffer[7]);
	DrawChar(myDataBuffer[8]);
	DrawChar(myDataBuffer[9]);
	DrawChar(myDataBuffer[10]);
	DrawChar(myDataBuffer[11]);
	DrawChar(myDataBuffer[12]);
	DrawChar(myDataBuffer[13]);
	DrawChar(myDataBuffer[14]);
	DrawChar(myDataBuffer[15]);
	
	MoveTo( 200, 260 );
	DrawString( "\pModel: " );
	DrawChar(myDataBuffer[16]);
	DrawChar(myDataBuffer[17]);
	DrawChar(myDataBuffer[18]);
	DrawChar(myDataBuffer[19]);
	DrawChar(myDataBuffer[20]);
	DrawChar(myDataBuffer[21]);
	DrawChar(myDataBuffer[22]);
	DrawChar(myDataBuffer[23]);
	DrawChar(myDataBuffer[24]);
	DrawChar(myDataBuffer[25]);
	DrawChar(myDataBuffer[26]);
	DrawChar(myDataBuffer[27]);
	DrawChar(myDataBuffer[28]);
	DrawChar(myDataBuffer[29]);
	DrawChar(myDataBuffer[30]);
	DrawChar(myDataBuffer[31]);
	
	MoveTo( 375, 260 );
	DrawString( "\pFirmware: " );
	DrawChar(myDataBuffer[32]);
	DrawChar(myDataBuffer[33]);
	DrawChar(myDataBuffer[34]);
	DrawChar(myDataBuffer[35]);
	//DrawChar(myDataBuffer[36]);
	//DrawChar(myDataBuffer[37]);
	//DrawChar(myDataBuffer[38]);
	//DrawChar(myDataBuffer[39]);
	
	ForeColor( blackColor);
}

/*********************** PROCEDURE: writes text to the screen ***********************/
void	ErrorText( void )
{
	Rect			TextRect;	
	
	SetRect( &TextRect, 25, 250, 480, 300 );
	EraseRect( &TextRect );
	
	TextSize( 12 );
	MoveTo( 30, 260 );
	ForeColor( redColor);
	DrawString( "\pError! No device present at this SCSI ID." );
	//MoveTo( 30, 260 );
	//DrawString( "\pNo device present at this SCSI ID." );
	//MoveTo( 30, 260 );
	//DrawString( "\pat this SCSI ID." );
	
	ForeColor( blackColor);
}

/************************** simulates a drive fail *******************************/
void	DriveFail( void )
{
	CIconHandle	hCIcon;
	OSErr		err;
	short		i;
	
	DrawPicture( statusPic, &gStatusRect );
	gClearButEnabled = FALSE;
	gCreateButEnabled = FALSE;
	InvalRect(&gCreateRect);
	InvalRect(&gClearRect);

	StatusText();
	
	for(i=0;i<7;i++)
	{
		hCIcon = GetCIcon(128);
		err = PlotCIconHandle(&icon1Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(129);
		err = PlotCIconHandle(&icon2Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(130);
		err = PlotCIconHandle(&icon3Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(131);
		err = PlotCIconHandle(&icon4Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(132);
		err = PlotCIconHandle(&icon5Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(133);
		err = PlotCIconHandle(&icon6Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		Delay(30L,0);
		
		hCIcon = GetCIcon(134);
		err = PlotCIconHandle(&icon1Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(135);
		err = PlotCIconHandle(&icon2Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(136);
		err = PlotCIconHandle(&icon3Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(137);
		err = PlotCIconHandle(&icon4Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(138);
		err = PlotCIconHandle(&icon5Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(139);
		err = PlotCIconHandle(&icon6Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		Delay(20L,0);
	}
		hCIcon = GetCIcon(128);
		err = PlotCIconHandle(&icon1Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(129);
		err = PlotCIconHandle(&icon2Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(130);
		err = PlotCIconHandle(&icon3Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(131);
		err = PlotCIconHandle(&icon4Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(132);
		err = PlotCIconHandle(&icon5Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
		hCIcon = GetCIcon(133);
		err = PlotCIconHandle(&icon6Rect, atHorizontalCenter, ttNone, hCIcon);
		DisposCIcon(hCIcon);
}

/******************** draws strings in status rect *******************************/
void	StatusText( void )
{

	short	curFont, curSize;
	Style	curFace;
	
	// Get defaults so you can restore after printing destination string
	curFont = thePort->txFont;
	curFace = thePort->txFace;
	curSize = thePort->txSize;
	
	TextFont( 0 );
	TextSize( 12 );
	TextFace( 1 );
	
	ForeColor(redColor);
	MoveTo(45, 337);
	DrawString("\pModule Drive Failure occured in Slot 5, SCSI ID 4.");
	MoveTo(45, 357);
	DrawString("\pDrive spun down and unlatched. RAID status degraded.");
	MoveTo(45, 377);
	DrawString("\pInsert new module or call MicroNet Tech Support for");
	MoveTo(45, 397);
	DrawString("\preplacement at (714) 453-6060.");
	ForeColor(blackColor);
	
	//restore default text styles
	TextFont( curFont );
	TextSize( curSize );
	TextFace( curFace );
}

/************************** unsets what DriveFail sets *****************************/
void	ResetFail( void )
{

	gClearButEnabled = TRUE;
	gCreateButEnabled = TRUE;
	
	InvalRect(&gCreateRect);
	InvalRect(&gClearRect);
	
	InvalRect(&icon1Rect);
	InvalRect(&icon2Rect);
	InvalRect(&icon3Rect);
	InvalRect(&icon4Rect);
	InvalRect(&icon5Rect);
	InvalRect(&icon6Rect);
	
	InvalRect(&gStatusRect);
}

/********************  ***********************************/

OSErr serialinit()
{
	OSErr   		err2 = 0;
	SerShk 			flags;

	/* Open Serial Drivers */
	err2 = OpenDriver(INDRIVER, &inRefNum);
	if (err2 != 0)
		return err2;
	err2 = OpenDriver(OUTDRIVER,&outRefNum);
	if (err2 != 0)
		return err2;
	
	/* Reset both input and output, and assign communication protocols */
	err2 = SerReset(inRefNum, baud9600 + data8 + stop10 + noParity);
	if (err2 != 0)
		return err2;
	err2 = SerReset(outRefNum, baud9600 + data8 + stop10 + noParity);
	if (err2 != 0)
		return err2;

	/* Give the serial input driver a SERBUFSIZ buffer */
	if(!(buf = NewPtr(SERBUFSIZ)))
		return MemError();
	err2 = SerSetBuf(inRefNum, buf, SERBUFSIZ);
	if (err2 != 0)
		return err2;

	/* Set handshaking for the input driver */
	flags.fXOn = TRUE;
    flags.fCTS = 0;
	flags.xOn = XONCHAR;
	flags.xOff = XOFFCHAR;
    flags.errs = 0;
    flags.evts = 0;
	flags.fInX = 0;
    flags.fDTR = 0;
    
    err2 = SerHShake(inRefNum,&flags);
	if (err2 != 0)
		return err2;

	if ( ! ( inbuf = (char *)NewPtr(SERBUFSIZ) ) )
		return MemError();

	return noErr;
}


/*****************   ******************************/
void	Dialog( Str255 string )
{
	DialogPtr	dialog;
	Boolean		dialogExit = FALSE;
	short		itemHit;

	dialog = GetNewDialog( 128, nil, (WindowPtr)-1L );
	if ( dialog == nil )					/* In case program couldn't load resource */
	{
		SysBeep( 10 );
	}
	
	ParamText(string, "\p", "\p", "\p");
	
	ShowWindow( dialog );
	SetPort( dialog );
	DrawDialog (dialog);
	
	SetDialogDefaultItem( dialog, 1 );
	//SetDialogCancelItem( dialog, 2 );
	
	while ( ! dialogExit )
	{
		ModalDialog( nil, &itemHit );
		
		switch( itemHit )
		{
			case 1:												// OK button
				dialogExit = TRUE;
				DisposDialog( dialog );
				SetPort( gMainPort );
				break;
		}
	}
			
}


/*************************** find way to begining screen ******************/
void	FindWayBack( void )
{
	char	*subString = "Press Any Key To Activate Menu";
	char	*result;
	
	//if (result = strstr(inbuf, subString)) SysBeep(10);
	MsgText( "\pWaiting for ICU. . .");
	do
	{
		SendEsc();
		Delay(kScanDelay,0);
		CheckSerBuf();
		result = strstr(inbuf, subString);
		//EventLoop();
	}
	while (!result);
	DrawPicture( scanClearPic, &gMsgTextRect );
}


/*************************** send ESC character over serial port to ICU ******************/
void	SendEsc( void )
{
	OSErr	iErr;
	long	inOutCount;
	char	escapeChar	=	escapeKey;
	
	
	inOutCount	=	sizeof(char);
	iErr = FSWrite(outRefNum, &inOutCount, &escapeChar);
}


/********************* puts characters from ICU  in buffer **************************/
void	CheckSerBuf( void )
{
	SerGetBuf(inRefNum, &count);
	if (count != 0)
	{
		(void) FSRead(inRefNum,&count,inbuf);
	}
}


/********************** navigates to begining of ICU menu ********************************/
void	GoToCreateRAID()
{
	char 		key;
	Cursor		curWatch;
	CursHandle	cursH;
	long 		num = 1;
	
	cursH = GetCursor( watchCursor );
	HLock((Handle) cursH);
	curWatch = **cursH;
	SetCursor( &curWatch );
	HUnlock((Handle) cursH);
	
	key = hexEscKey;
	(void) FSWrite(outRefNum, &num, &key);
	Delay(kScanDelay,0);
	CheckSerBuf();
	
	
	// push the 4 key
	key = '4';
	(void) FSWrite(outRefNum, &num, &key);
	Delay(kScanDelay,0);
	CheckSerBuf();
	Delay(kScanDelay,0);
	Delay(kScanDelay,0);
	Delay(kScanDelay,0);
	
	
	// choose menu optin CheckSerBuf
	CheckSerBuf();
	Delay(kScanDelay,0);
	Delay(kScanDelay,0);
	
		
	InitCursor();
}


/******************** navigates to Enclosure Status menu of ICU ************************/
void	GoToEnclosureStatus( void )
{
	char 		key;
	Cursor		curWatch;
	CursHandle	cursH;
	long 		num = 1;
	
	cursH = GetCursor( watchCursor );
	HLock((Handle) cursH);
	curWatch = **cursH;
	SetCursor( &curWatch );
	HUnlock((Handle) cursH);
	
	MsgText("\pCommunicating . . .");
	
	key = hexEscKey;
	(void) FSWrite(outRefNum, &num, &key);
	Delay(kScanDelay,0);
	CheckSerBuf();
	
	
	// push the 1 key
	key = '1';
	(void) FSWrite(outRefNum, &num, &key);
	Delay(kScanDelay,0);
	CheckSerBuf();
	Delay(kScanDelay,0);
	Delay(kScanDelay,0);
	Delay(kScanDelay,0);
	
	
	// the status menu is broken into two buffer dumps, so check buf again
	CheckSerBuf();
	Delay(kScanDelay,0);
	Delay(kScanDelay,0);
	
		
	DrawPicture( scanClearPic, &gMsgTextRect );
	InitCursor();
}


/************************************  ***********************************/
void	WhichSlotsFilled(void)
{
	short	numArray[14];
	short	x = 0;
	short	num0, num1, num2, num3, num4, num5, num6;
	char	*stringA = "YES";
	Str255	stringSlotA = "\p[7;25";
	Str255	stringSlotB = "\p[8;25";
	Str255	stringSlotC = "\p[9;25";
	Str255	stringSlotD = "\p10;25";
	Str255	stringSlotE = "\p11;25";
	Str255	stringSlotF = "\p12;25";
	Str255	stringSlotG = "\p13;25";
	Str255	slotA;
	Str255	slotB;
	Str255	slotC;
	Str255	slotD;
	Str255	slotE;
	Str255	slotF;
	Str255	slotG;
	
	char	*result;
	long	i;
	
	MsgText("\pModule Scan. . .");
	
	if (result = strstr(inbuf, stringA)) 
	{
		for (i=0; i < count; i++)
		{
			if (inbuf[i] == 'Y' || inbuf[i] == 'N' )
			{
				numArray[x] = (short) i;
				x = x + 1;
			}
		}

		num0 = numArray[0];
		num1 = numArray[1];
		num2 = numArray[2];
		num3 = numArray[3];
		num4 = numArray[4];
		num5 = numArray[5];
		num6 = numArray[6];
		
		slotA[0] = 5;
		slotA[1] = inbuf[num0 - 10];
		slotA[2] = inbuf[num0 - 9];
		slotA[3] = inbuf[num0 - 8];
		slotA[4] = inbuf[num0 - 7];
		slotA[5] = inbuf[num0 - 6];
		
		slotB[0] = 5;
		slotB[1] = inbuf[num1 - 10];
		slotB[2] = inbuf[num1 - 9];
		slotB[3] = inbuf[num1 - 8];
		slotB[4] = inbuf[num1 - 7];
		slotB[5] = inbuf[num1 - 6];
		
		slotC[0] = 5;
		slotC[1] = inbuf[num2 - 10];
		slotC[2] = inbuf[num2 - 9];
		slotC[3] = inbuf[num2 - 8];
		slotC[4] = inbuf[num2 - 7];
		slotC[5] = inbuf[num2 - 6];
		
		slotD[0] = 5;
		slotD[1] = inbuf[num3 - 10];
		slotD[2] = inbuf[num3 - 9];
		slotD[3] = inbuf[num3 - 8];
		slotD[4] = inbuf[num3 - 7];
		slotD[5] = inbuf[num3 - 6];
		
		slotE[0] = 5;
		slotE[1] = inbuf[num4 - 10];
		slotE[2] = inbuf[num4 - 9];
		slotE[3] = inbuf[num4 - 8];
		slotE[4] = inbuf[num4 - 7];
		slotE[5] = inbuf[num4 - 6];
		
		slotF[0] = 5;
		slotF[1] = inbuf[num5 - 10];
		slotF[2] = inbuf[num5 - 9];
		slotF[3] = inbuf[num5 - 8];
		slotF[4] = inbuf[num5 - 7];
		slotF[5] = inbuf[num5 - 6];
		
		slotG[0] = 5;
		slotG[1] = inbuf[num6 - 10];
		slotG[2] = inbuf[num6 - 9];
		slotG[3] = inbuf[num6 - 8];
		slotG[4] = inbuf[num6 - 7];
		slotG[5] = inbuf[num6 - 6];
		
		if (slotA[1] == stringSlotA[1] && slotA[2] == stringSlotA[2] && slotA[3] == stringSlotA[3] &&
		slotA[4] == stringSlotA[4] && slotA[5] == stringSlotA[5])
		{
			DrawPicture( module1Pic, &gModule1Rect );
			gModule1Present = TRUE;
			gMod1ID = 0;
			DrawDDStrings(gMod1ID);
			Delay(kScanDelay,0);
		}
		
		if (slotB[1] == stringSlotB[1] && slotB[2] == stringSlotB[2] && slotB[3] == stringSlotB[3] &&
		slotB[4] == stringSlotB[4] && slotB[5] == stringSlotB[5])
		{
			if (gModule1Present == TRUE)
				DrawPicture( module2Pic, &gModule2Rect );
			else
				DrawPicture( module2BlankPic, &gModule2Rect );
			gModule2Present = TRUE;
			gMod2ID = 1;
			DrawDDStrings(gMod2ID);
			Delay(kScanDelay,0);
		}
		
		if (slotC[1] == stringSlotC[1] && slotC[2] == stringSlotC[2] && slotC[3] == stringSlotC[3] &&
		slotC[4] == stringSlotC[4] && slotC[5] == stringSlotC[5])
		{
			if (gModule2Present == TRUE)
				DrawPicture( module3Pic, &gModule3Rect );
			else
				DrawPicture( module3BlankPic, &gModule3Rect );
			
			gModule3Present = TRUE;
			gMod3ID = 2;
			DrawDDStrings(gMod3ID);
			Delay(kScanDelay,0);
		}
		
		if (slotD[1] == stringSlotD[1] && slotD[2] == stringSlotD[2] && slotD[3] == stringSlotD[3] &&
		slotD[4] == stringSlotD[4] && slotD[5] == stringSlotD[5])
		{
			if (gModule3Present == TRUE)
				DrawPicture( module4Pic, &gModule4Rect );
			else
				DrawPicture( module4BlankPic, &gModule4Rect );
			gModule4Present = TRUE;
			gMod4ID = 3;
			DrawDDStrings(gMod4ID);
			Delay(kScanDelay,0);
		}
		
		if (slotE[1] == stringSlotE[1] && slotE[2] == stringSlotE[2] && slotE[3] == stringSlotE[3] &&
		slotE[4] == stringSlotE[4] && slotE[5] == stringSlotE[5])
		{
			if (gModule4Present == TRUE)
				DrawPicture( module5Pic, &gModule5Rect );
			else
				DrawPicture( module5BlankPic, &gModule5Rect );
			gModule5Present = TRUE;
			gMod5ID = 4;
			DrawDDStrings(gMod5ID);
			Delay(kScanDelay,0);
		}
		
		if (slotF[1] == stringSlotF[1] && slotF[2] == stringSlotF[2] && slotF[3] == stringSlotF[3] &&
		slotF[4] == stringSlotF[4] && slotF[5] == stringSlotF[5])
		{
			if (gModule5Present == TRUE)
				DrawPicture( module6Pic, &gModule6Rect );
			else
				DrawPicture( module6BlankPic, &gModule6Rect );
			gModule6Present = TRUE;
			gMod6ID = 5;
			DrawDDStrings(gMod6ID);
			Delay(kScanDelay,0);
		}
		
		if (slotG[1] == stringSlotG[1] && slotG[2] == stringSlotG[2] && slotG[3] == stringSlotG[3] &&
		slotG[4] == stringSlotG[4] && slotG[5] == stringSlotG[5])
		{
			if (gModule6Present == TRUE)
				DrawPicture( module7Pic, &gModule7Rect );
			else
				DrawPicture( module7BlankPic, &gModule7Rect );
			gModule7Present = TRUE;
			gMod7ID = 6;
			DrawDDStrings(gMod7ID);
		}
		
	// end of if strstr in buffer
	}
	DrawPicture( scanClearPic, &gMsgTextRect );
}


/******************************** disposes serial port drives *******************************/
void cleanup( void )
{
	if(outRefNum)
		CloseDriver(outRefNum);
	if(inRefNum)
		CloseDriver(inRefNum);
	if (inbuf)
		DisposPtr(inbuf);
}


/****CURRENTLY NOT USED******* send TAB character to serial port ****************************/
void	SendTab( void )
{
	OSErr	iErr;
	long	inOutCount;
	char	tabChar		=	tabKey;
	
	
	inOutCount	=	sizeof(char);
	iErr = FSWrite(outRefNum, &inOutCount, &tabChar);
}


/*************************** writes message text ******************/
void	MsgText( Str255 stringMsg )
{	
		TextFont( 6 );				//Monaco, a mono-spaced font
		TextSize( 24 );
		TextFace( 0 );	
		ForeColor(greenColor);
		
	MoveTo(230,65);
	DrawString(stringMsg);
}
