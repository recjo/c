/****************************************************************************
This Macintosh program sends SCSI commands to a SCSI CD-ROM drive to 
play audio CD's.


****************************************************************************/
#include <scsi.h>
#include <math.h>
/************************************************************************************/
#define	IsHighBitSet( longNum )		( (longNum >> 23) & 1 )
#define	SetHighByte( longNum )		( longNum |= 0xFF000000 )
#define	ClearHighByte( longNum )	( longNum &= 0x00FFFFFF )

#define k1stScreenID				128		/* Opening screen window ID*/
#define k1stScreenPictID			400		/* set to 128 for pic with buttons */
#define kAbtScreenID				127		/* About box screen PICT ID */
#define kInquiryCmd					0x12	/* SCSI Inquiry */
#define kCompletionTimeout			300		/* SCSI Inquiry */
#define kWaitTicks					60L		/* timer */
	
typedef struct						// 6 byte SCSI command
{
	char	opCode;
	char	lun;
	char	pageCode;
	char	reserved;
	char	allocationLen;
	char	control;
} CDBType6;
	
typedef struct						// 10 byte SCSI command
{
	char	byte0;
	char	byte1;
	char	byte2;
	char	byte3;
	char	byte4;
	char	byte5;
	char	byte6;
	char	byte7;
	char	byte8;
	char	byte9;
} CDBType10;

/********************************* Globals *****************************************/
Boolean		gDone;
Boolean		gPause = FALSE;
Boolean		gPlayDown = FALSE;				// HandleMouseDown
Boolean		gPauseDown = FALSE;				// HandleMouseDown
GrafPtr		gMainPort;						// sets a global port for main window
short		gWhichButton = 0;				// HandleMouseDown
long		gTicks;


WindowPtr	gMinWindow;
WindowPtr	gMainWindow;
short		gRepeatToggle = 1;
short		gShuffleToggle = 1;
short		gTimeToggle = 1;
short		gLSndToggle = 0;
short		gRSndToggle = 0;

long		gMaxTrackNumber = 0;				//Maximin Number of Tracks on CD
long		gTrackNumber = 1;				//Current Track Number
short		CDid = 3;						// SCSI ID of CD-ROM
short		gCounter = 0;					//EventLoop for minutes, seconds digits
short		gCounter2 = 0;					//EventLoop for minutes, seconds digits
short		gCounter3 = 0;					//EventLoop for minutes, seconds digits
RgnHandle	gTheRgn;						//for slider support

short		gMinRectTop;
short		gMinRectLeft;
short		gRectTop;
short		gRectLeft;

Rect		gLogoRect;

Rect		gStopRect;
Rect		gPlayRect;
Rect		gPauseRect;
Rect		gEjectRect;
Rect		gTimeRect;

Rect		gLTrakRect;
Rect		gLScanRect;
Rect		gRScanRect;
Rect		gRTrakRect;

Rect		gRepeatRect;
Rect		gShuffleRect;
Rect		gProgramRect;
Rect		gABRect;
Rect		gLeftSoundRect;
Rect		gRightSoundRect;

Rect		gRepeatPicRect;
Rect		gMinStopRect;
Rect		gMinPlayRect;
Rect		gMinPauseRect;
Rect		gSliderRect;
Rect		gSliderButtonRect;
Rect		gTrack1Rect;			//for 1 digit of trak pic
Rect		gTrack2Rect;			//for 2 digit of trak pic

Rect		gTime1Rect;				//for seconds digit
Rect		gTime2Rect;				//for seconds digit
Rect		gTime3Rect;				//for seconds digit
Rect		gTime4Rect;				//for seconds digit

PicHandle	gStopPict;
PicHandle	gStopUpPict;
PicHandle	gPlayPict;
PicHandle	gPlayUpPict;
PicHandle	gEjectPict;
PicHandle	gEjectUpPict;
PicHandle	gPausePict;
PicHandle	gPauseUpPict;
PicHandle	gTimePict;
PicHandle	gTimeUpPict;
PicHandle	gLTrakPict;
PicHandle	gLTrakUpPict;
PicHandle	gLScanPict;
PicHandle	gLScanUpPict;
PicHandle	gRTrakPict;
PicHandle	gRTrakUpPict;
PicHandle	gRScanPict;
PicHandle	gRScanUpPict;
PicHandle	gRepeatPic;
PicHandle	gShufflePic;
PicHandle	gProgramPic;
PicHandle	gABPic;
PicHandle	gLSoundPic;
PicHandle	gLSoundUpPic;
PicHandle	gRSoundPic;
PicHandle	gRSoundUpPic;
PicHandle	gCDRepeatPic;
PicHandle	gTrackRepeatPic;
PicHandle	gClearRepeatPic;
PicHandle	gTextStripPic;
PicHandle	gSliderPic;
PicHandle	gNumPic[10];

CDBType6	gCDB;						// SCSI Inquiry
CDBType10	gCDB10;						// SCSI Inquiry
SCSIInstr	gTIB[2];					// SCSI Inquiry 
char		*gCDBPtr;					// SCSI Inquiry
Byte		*gCDBPtr2;					// SCSI Inquiry
char		gDataBuffer[40];			// SCSI Inquiry 
Byte		gTOCBuff[16];
Byte		gCDB12[10];					// SCSI Inquiry 
short		gMyErr;			// SCSI Inquiry. used by SCSIText() to tell if device present
short		gDriverRefNum;				// DRVR

/*********************************  My Routines (All Procedures)  **********************/
void	ToolBoxInit( void );
void	CreateWindow( void );
void	aboutBox( void );
void	devicesWindow( void);
void	LoadDialog( void );
void	MenuBarInit( void );
void	PreLoad( void );
void	LoadScreen( void );
void	Minimize( void );
void	CenterPict( PicHandle picture, Rect *destRectPtr );
void	EventLoop( void );							// for "Quit" from FILE menu
void	DoEvent( EventRecord *eventPtr );			// for "Quit" from FILE menu
void	HandleNull( EventRecord *eventPtr );		// for "Quit" from FILE menu
void	HandleMouseDown( EventRecord *eventPtr );	// for "Quit" from FILE menu
void	HandleMouseUp( EventRecord *eventPtr );		// for "Quit" from FILE menu
void	HandleMenuChoice( long menuChoice );		// for "Quit" from FILE menu
void	HandleAppleChoice( short item );			// for items from APPLE menu
void	HandleFileChoice( short item );				// for items from FILE menu
void	HandleEditChoice( short item );				// for items from EDIT menu
void	HandleSpecialChoice( short item );			// for items from SPECIAL menu
void	HandleDeviceChoice( short item );			// for items from DEVICE menu
void	DoUpdate( EventRecord *eventPtr );						// for updating window
void	ReDrawPicture( WindowPtr window, PicHandle picture );	// for updating window
OSErr	SCSIRoutine( short id, short left, short top );			// for "SCSI Inquiry"
void	SCSIText( short id, short left, short top );			// for "SCSI Inquiry"
void	NoDriveSCSIText( short id, short left, short top );		// for "SCSI Inquiry"
void	SCSILoop( void );										// for "SCSI Inquiry"
void	TrackNumber( void );
void	DrawTimeString( void );
void	DrawRepeatSymbol( void );
void	ShuffleString( void );
void	DrawLSound( void );
void	DrawRSound( void );
void	DrawSlider(void);
void	ReadTOC(short id);
OSErr	PlayTrack(short id);
OSErr	StopTrack(short id);
OSErr	ResumeTrack(short id);
void	EjectCD(short id);


/*-------------------------------- MAIN MAIN MAIN MAIN --------------------------------*/
void	main( void)
{
	ToolBoxInit();							/*  My Routine   */
	CreateWindow();							/*  My Routine   */
	MenuBarInit();							/*  My Routine   */
	PreLoad();								/*  My Routine   */
	EventLoop();							/*  My Routine   */		
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
	InitDialogs(nil);
	InitCursor();
}

/******************** PROCEDURE: creates window from resource fork *********************/
void	CreateWindow( void)
{
	WindowPtr		window;

	window = GetNewWindow( k1stScreenID, nil, (WindowPtr)-1L );	/* k1stScreenID is window ID 128*/
	
	if ( window == nil )	/* In case program couldn't load resource */
	{
		SysBeep( 10 );
		ExitToShell();
	}
	gMainWindow = window;

	SetWRefCon ( window, (long) (k1stScreenPictID) );	// stashing Pict ID
	ShowWindow( window );							/*  ROM Routine  */
	SetPort( window );								/*  ROM Routine  */
	GetPort( &gMainPort );			/*makes main window the main port to draw in*/
}

/*********************** PROCEDURE: draws and inserts a menu ***************************/
void	MenuBarInit( void )
{
	Handle		menuBar;
	MenuHandle	menu;

	menuBar = GetNewMBar( 128 );		/* fetches MBAR resource (from ID=128) */
	SetMenuBar( menuBar );

	menu = GetMHandle( 128 );			/* Inserts Apple Menu (from ID=128) */
	AddResMenu( menu, 'DRVR' );

	DrawMenuBar();

}

/********* PROCEDURE: defines RECT for my buttons, also preloads button pics ***********/
void	PreLoad( void )
{
	short		anyErr;					// DRVR
	
	//				   L  T  R   B
	SetRect(&gStopRect, 1, 1, 47, 21);
	SetRect(&gPlayRect, 1, 1, 47, 21);
	SetRect(&gEjectRect, 1, 1, 47, 21);
	SetRect(&gPauseRect, 1, 1, 47, 21);
	SetRect(&gTimeRect, 1, 1, 47, 21);
	
	SetRect(&gLTrakRect, 1, 1, 47, 21);
	SetRect(&gLScanRect, 1, 1, 47, 21);
	SetRect(&gRScanRect, 1, 1, 47, 21);
	SetRect(&gRTrakRect, 1, 1, 47, 21);
	
	SetRect(&gRepeatRect, 1, 1, 38, 15);
	SetRect(&gShuffleRect, 1, 1, 38, 15);
	SetRect(&gProgramRect, 1, 1, 38, 15);
	SetRect(&gABRect, 1, 1, 38, 15);
	SetRect(&gLeftSoundRect, 1, 1, 38, 15);
	SetRect(&gRightSoundRect, 1, 1, 38, 15);
	
	SetRect(&gLogoRect, 1, 1, 37, 38);
	SetRect(&gRepeatPicRect, 1, 1, 23, 21);
	
	SetRect(&gSliderRect, 1, 1, 26, 69);
	SetRect(&gSliderButtonRect, 1, 1, 25, 11);
	SetRect(&gTrack1Rect, 1, 1, 19, 23);
	SetRect(&gTrack2Rect, 1, 1, 19, 23);
	
	SetRect(&gTime1Rect, 1, 1, 19, 23);
	SetRect(&gTime2Rect, 1, 1, 19, 23);
	SetRect(&gTime3Rect, 1, 1, 19, 23);
	SetRect(&gTime4Rect, 1, 1, 19, 23);
	
	//SetRect(&gMinStopRect, 1, 1, 46, 20);
	//SetRect(&gMinPlayRect, 1, 1, 46, 20);
	//SetRect(&gMinPauseRect, 1, 1, 46, 20);
	
	//					   L   T
	OffsetRect(&gStopRect, 19, 100);
	OffsetRect(&gPlayRect, 67, 100);
	OffsetRect(&gEjectRect, 115, 100);
	OffsetRect(&gPauseRect, 163, 100);
	OffsetRect(&gTimeRect, 211, 100);
	
	OffsetRect(&gLTrakRect, 346, 100);
	OffsetRect(&gLScanRect, 394, 100);
	OffsetRect(&gRScanRect, 442, 100);
	OffsetRect(&gRTrakRect, 490, 100);
	
	OffsetRect(&gRepeatRect, 321, 130);
	OffsetRect(&gShuffleRect, 364, 130);
	OffsetRect(&gProgramRect, 407, 130);
	OffsetRect(&gABRect,      450, 130);
	OffsetRect(&gLeftSoundRect, 493, 130);
	OffsetRect(&gRightSoundRect, 536, 130);
	
	OffsetRect(&gLogoRect, 494, 13);
	OffsetRect(&gRepeatPicRect, 93, 34);
	
	OffsetRect(&gSliderRect, 282, 73);
	OffsetRect(&gSliderButtonRect, 282, 77);
	OffsetRect(&gTrack1Rect, 34, 42);
	OffsetRect(&gTrack2Rect, 51, 42);
	
	OffsetRect(&gTime1Rect, 141, 42);
	OffsetRect(&gTime2Rect, 160, 42);
	OffsetRect(&gTime3Rect, 189, 42);
	OffsetRect(&gTime4Rect, 207, 42);
	
	gStopPict = GetPicture (129);
	gStopUpPict = GetPicture (200);
	gPlayPict = GetPicture (131);
	gPlayUpPict = GetPicture (201);
	gEjectPict = GetPicture (133);
	gEjectUpPict = GetPicture (202);
	gPausePict = GetPicture (135);
	gPauseUpPict = GetPicture (203);
	gTimePict = GetPicture (137);
	gTimeUpPict = GetPicture (204);
	gLTrakPict = GetPicture (139);
	gLTrakUpPict = GetPicture (205);
	gLScanPict = GetPicture (141);
	gLScanUpPict = GetPicture (206);
	gRScanPict = GetPicture (143);
	gRScanUpPict = GetPicture (207);
	gRTrakPict = GetPicture (145);
	gRTrakUpPict = GetPicture (208);
	gRepeatPic = GetPicture (180);
	gShufflePic = GetPicture (181);
	gProgramPic = GetPicture (182);
	gABPic = GetPicture (183);
	gLSoundPic = GetPicture (146);
	gLSoundUpPic = GetPicture (184);
	gRSoundPic = GetPicture (147);
	gRSoundUpPic = GetPicture (185);
	gCDRepeatPic = GetPicture (130);
	gTrackRepeatPic = GetPicture (132);
	gClearRepeatPic = GetPicture (134);
	gTextStripPic = GetPicture (136);
	gSliderPic = GetPicture (138);
	
	gNumPic[0] = GetPicture (150);
	gNumPic[1] = GetPicture (151);
	gNumPic[2] = GetPicture (152);
	gNumPic[3] = GetPicture (153);
	gNumPic[4] = GetPicture (154);
	gNumPic[5] = GetPicture (155);
	gNumPic[6] = GetPicture (156);
	gNumPic[7] = GetPicture (157);
	gNumPic[8] = GetPicture (158);
	gNumPic[9] = GetPicture (159);
	
	
	ReadTOC(CDid);
	//anyErr  = OpenDriver("\p.AppleCD", &gDriverRefNum );			// DRVR
	//if (anyErr != 0 ) SysBeep(10);
}

/********************* PROCEDURE: loads opening screen *********************************/
void	LoadScreen( void )
{
	Rect		pictureRect;
	WindowPtr	window;
	PicHandle	picture;

	window = FrontWindow();

	pictureRect = window->portRect;

	picture = GetPicture (k1stScreenPictID);
	
	if ( picture == nil )							/* in case resource didn't load */
	{
		SysBeep( 10 );
		ExitToShell();
	}

	CenterPict( picture, &pictureRect );			/*  My Routine   */
	DrawPicture( picture, &pictureRect );			/*  ROM Routine  */
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
	long			ticks, stopTicks;
	
	gDone = false;
	
	while ( gDone == false )
	{	
		/*if ( gPlayDown == 156 )	// change to 1 if you want this IF code to run
		{	
			ticks = TickCount();		//stick tickcounts in a variable
			
			if (ticks > gTicks + 60L)		//playdown put tickount in a variable
			{								//if it equals 60 of em . . .
				gTicks = gTicks + 60L;		//set gTicks to the next second (+60)
				gCounter = gCounter + 1;
				DrawPicture( gNumPic[gCounter], &gTime4Rect );
				if (gCounter > 9) 			//get ready to set the 10 digit
				{
					DrawPicture( gNumPic[0], &gTime4Rect );
					gCounter2 = gCounter2 + 1;
					gCounter = 0;
					DrawPicture( gNumPic[gCounter2], &gTime3Rect );
					
					if (gCounter2 > 5) 			//set the minutes digit for over 60 seconds
					{
						DrawPicture( gNumPic[0], &gTime4Rect );
						DrawPicture( gNumPic[0], &gTime3Rect );
						gCounter3 = gCounter3 + 1;
						gCounter = 0;
						gCounter2 = 0;
						DrawPicture( gNumPic[gCounter3], &gTime2Rect );
					}
				}
			}
		}*/
		
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
	WindowPtr		whichWindow, window;
	short			thePart;
	short			partcode;							/*for slider support*/
	long			menuChoice, refNum;
	Point			rectPoint, thePt;					/*for button support*/
	Point			startPoint;							/*for slider support*/
	ProcPtr			actionProc;							/*for slider support*/
	long			coords;							/*for slider support*/
	ControlHandle	theControl;							/*for button support*/
	PicHandle		picture;							/*for testing Rect*/
	Boolean			tfvalue;
	Rect			tempRect;
	Boolean			InRect;
	Point			tempPoint;

	thePart = FindWindow( eventPtr->where, &whichWindow );
	switch( thePart )
	{
		case inDrag:
			DragWindow( whichWindow, eventPtr->where, &screenBits.bounds );
			break;
		case inGoAway:
			tfvalue = TrackGoAway( whichWindow, thePt );
			if (tfvalue) gDone = true;
			break;
		case inMenuBar:
			menuChoice = MenuSelect( eventPtr->where );
			HandleMenuChoice( menuChoice );
			break;
		case inZoomIn:
		case inZoomOut:
			if ( TrackBox( whichWindow, eventPtr->where, thePart ));
			{
				window = gMinWindow;
				HideWindow( window );
				CreateWindow();
				LoadScreen();
			}
			break;
	// in window somewhere
		case inContent:
				
			rectPoint = eventPtr->where;
			GlobalToLocal(&rectPoint);
	
	//detects a mousedown in the minimize window
			if (whichWindow == gMinWindow) 
			{
				SetPort( whichWindow );
				
				if (PtInRect(rectPoint, &gMinStopRect ))
				{
					DrawPicture( gStopPict, &gMinStopRect );
					InRect	=	TRUE;
					do
					{
						GetMouse(&tempPoint);
						if (InRect)
						{	
							//when mouse leaves rect
							if (PtInRect(tempPoint, &gMinStopRect) == FALSE)
							{
								InRect	=	FALSE;
								InvalRect( &gMinStopRect );
								BeginUpdate(gMinWindow);
								ReDrawPicture( gMinWindow, GetPicture(GetWRefCon( gMinWindow)) );
								EndUpdate(whichWindow);
							}
						}
						else
						{
							//when mouse enters rect after leaving
							if (PtInRect(tempPoint, &gMinStopRect))
							{
								InRect	=	TRUE;
								DrawPicture( gStopPict, &gMinStopRect );
							}
						}
					} while (StillDown());
					InvalRect( &gMinStopRect );	
					BeginUpdate(gMinWindow);
					ReDrawPicture( gMinWindow, GetPicture(GetWRefCon( gMinWindow)) );
					EndUpdate(whichWindow);
					if (InRect)
					{
						// action items!!!!!!!!
						DrawPicture( gStopUpPict, &gMinStopRect );
						//DrawPicture( gPlayUpPict, &gMinPlayRect ); 
						gPlayDown = FALSE;
						//DrawPicture( gPauseUpPict, &gMinPauseRect ); 
						gPauseDown = FALSE;
						gCounter = 0;
						StopTrack(CDid);
					}
					return;
				}
				if (PtInRect(rectPoint, &gMinPlayRect ))
				{
					DrawPicture( gPlayPict, &gMinPlayRect );
					InRect	=	TRUE;
					do
					{
						GetMouse(&tempPoint);
						if (InRect)
						{	
							//when mouse leaves rect
							if (PtInRect(tempPoint, &gMinPlayRect) == FALSE)
							{
								InRect	=	FALSE;		// This is my out button hack  *
								InvalRect( &gMinPlayRect );		//** This is my out button hack 
								BeginUpdate(gMinWindow);
								ReDrawPicture( gMinWindow, GetPicture(GetWRefCon( gMinWindow)) );
								EndUpdate(whichWindow);
							}
						}
						else
						{
							//when mouse enters rect after leaving
							if (PtInRect(tempPoint, &gMinPlayRect))
							{
								InRect	=	TRUE;
								DrawPicture( gPlayPict, &gMinPlayRect );
							}
						}
					} while (StillDown());
					InvalRect( &gMinPlayRect );	
					BeginUpdate(gMinWindow);
					ReDrawPicture( gMinWindow, GetPicture(GetWRefCon( gMinWindow)) );
					EndUpdate(whichWindow);
					if (InRect)
					{
						// action items!!!!!!!!
						gPlayDown = TRUE;
						//if (gPauseDown == TRUE)
						//{
						//	DrawPicture( gPauseUpPict, &gPauseRect ); 
						//	gPauseDown = 0;
						//}
						PlayTrack(CDid);
					}
					return;
				}
				if (PtInRect(rectPoint, &gMinPauseRect ))
				{
					SysBeep(10);
					ForeColor( greenColor);
					FrameRect(&gMinPauseRect);
				}
				SetPort( gMainPort );
			}
			
	//detects a mousedown in button rects
	
//Stop
			if ( PtInRect(rectPoint, &gStopRect ) )		// && buttonsEnabled != FALSE )
			{
				DrawPicture( gStopPict, &gStopRect );
				InRect	=	TRUE;
				do
				{
					GetMouse(&tempPoint);
					if (InRect)
					{	
						//when mouse leaves rect
						if (PtInRect(tempPoint, &gStopRect) == FALSE)
						{
							InRect	=	FALSE;		// This is my out button hack  *
							InvalRect( &gStopRect );		//** This is my out button hack 
							BeginUpdate(gMainWindow);
							ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
							EndUpdate(whichWindow);
						}
					}
					else
					{
						//when mouse enters rect after leaving
						if (PtInRect(tempPoint, &gStopRect))
						{
							InRect	=	TRUE;
							DrawPicture( gStopPict, &gStopRect );
						}
					}
				} while (StillDown());
				if (InRect)
				{
					// action items!!!!!!!!
					DrawPicture( gStopUpPict, &gStopRect );
					DrawPicture( gPlayUpPict, &gPlayRect ); 
					gPlayDown = FALSE;
					DrawPicture( gPauseUpPict, &gPauseRect ); 
					gPauseDown = FALSE;
					gCounter = 0;
					StopTrack(CDid);
				}
				return;
			}
			
//Play
			if ( PtInRect(rectPoint, &gPlayRect ) )		// && buttonsEnabled != FALSE )
			{
				DrawPicture( gPlayPict, &gPlayRect );
				InRect	=	TRUE;
				do
				{
					GetMouse(&tempPoint);
					if (InRect)
					{	
						//when mouse leaves rect
						if (PtInRect(tempPoint, &gPlayRect) == FALSE)
						{
							InRect	=	FALSE;		// This is my out button hack  *
							InvalRect( &gPlayRect );		//** This is my out button hack 
							BeginUpdate(gMainWindow);
							ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
							EndUpdate(whichWindow);
						}
					}
					else
					{
						//when mouse enters rect after leaving
						if (PtInRect(tempPoint, &gPlayRect))
						{
							InRect	=	TRUE;
							DrawPicture( gPlayPict, &gPlayRect );
						}
					}
				} while (StillDown());
				if (InRect)
				{
					// action items!!!!!!!!
					gPlayDown = TRUE;
					if (gPauseDown == TRUE)
					{
						DrawPicture( gPauseUpPict, &gPauseRect ); 
						gPauseDown = 0;
					}
					PlayTrack(CDid);
				}
				return;
			}
		
//eject
			if ( PtInRect(rectPoint, &gEjectRect ) )		// && buttonsEnabled != FALSE )
			{
				DrawPicture( gEjectPict, &gEjectRect );
				InRect	=	TRUE;
				do
				{
					GetMouse(&tempPoint);
					if (InRect)
					{	
						//when mouse leaves rect
						if (PtInRect(tempPoint, &gEjectRect) == FALSE)
						{
							InRect	=	FALSE;		// This is my out button hack  *
							InvalRect( &gEjectRect );		//** This is my out button hack 
							BeginUpdate(gMainWindow);
							ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
							EndUpdate(whichWindow);
						}
					}
					else
					{
						//when mouse enters rect after leaving
						if (PtInRect(tempPoint, &gEjectRect))
						{
							InRect	=	TRUE;
							DrawPicture( gEjectPict, &gEjectRect );
						}
					}
				} while (StillDown());
				if (InRect)
				{
					// action items!!!!!!!!
					DrawPicture( gEjectUpPict, &gEjectRect );
					if (gPlayDown == TRUE) 
					{
						DrawPicture( gPlayUpPict, &gPlayRect );
						gPlayDown = FALSE;
					}
					if (gPauseDown == TRUE)
					{
						DrawPicture( gPauseUpPict, &gPauseRect );
						gPauseDown = FALSE;
					}
					EjectCD(CDid);
				}
				return;
			}
			
//Pause
			if ( PtInRect(rectPoint, &gPauseRect ) )		// && buttonsEnabled != FALSE )
			{
					
				DrawPicture( gPausePict, &gPauseRect );
				InRect	=	TRUE;
				do
				{
					GetMouse(&tempPoint);
					if (InRect)
					{	
						//when mouse leaves rect
						if (PtInRect(tempPoint, &gPauseRect) == FALSE)
						{
							InRect	=	FALSE;		// This is my out button hack  *
							InvalRect( &gPauseRect );		//** This is my out button hack 
							BeginUpdate(gMainWindow);
							ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
							EndUpdate(whichWindow);
						}
					}
					else
					{
						//when mouse enters rect after leaving
						if (PtInRect(tempPoint, &gPauseRect))
						{
							InRect	=	TRUE;
							DrawPicture( gPausePict, &gPauseRect );
						}
					}
				} while (StillDown());
				if (InRect)
				{
					if ( gPauseDown == FALSE )
					{
						gPauseDown = TRUE;
						StopTrack(CDid);
					}
					else
					{
						gPauseDown = FALSE;
						DrawPicture( gPauseUpPict, &gPauseRect );
						ResumeTrack(CDid);
					}
				}
				return;
			}
			
//Time
			if ( PtInRect(rectPoint, &gTimeRect ) )		// && buttonsEnabled != FALSE )
			{
				DrawPicture( gTimePict, &gTimeRect );
				InRect	=	TRUE;
				do
				{
					GetMouse(&tempPoint);
					if (InRect)
					{	
						//when mouse leaves rect
						if (PtInRect(tempPoint, &gTimeRect) == FALSE)
						{
							InRect	=	FALSE;		// This is my out button hack  *
							InvalRect( &gTimeRect );		//** This is my out button hack 
							BeginUpdate(gMainWindow);
							ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
							EndUpdate(whichWindow);
						}
					}
					else
					{
						//when mouse enters rect after leaving
						if (PtInRect(tempPoint, &gTimeRect))
						{
							InRect	=	TRUE;
							DrawPicture( gTimePict, &gTimeRect );
						}
					}
				} while (StillDown());
				if (InRect)
				{
					// action items!!!!!!!!
					DrawPicture( gTimePict, &gTimeRect );
					SetRect( &tempRect, 130, 30, 245, 39 );
					DrawPicture( gTextStripPic, &tempRect );
					gTimeToggle = gTimeToggle + 1;
					DrawTimeString();
					DrawPicture( gTimeUpPict, &gTimeRect );
				}
				return;
			}
			
//Left Trak
			if ( PtInRect(rectPoint, &gLTrakRect ) )		// && buttonsEnabled != FALSE )
			{
				DrawPicture( gLTrakPict, &gLTrakRect );
				InRect	=	TRUE;
				do
				{
					GetMouse(&tempPoint);
					if (InRect)
					{	
						//when mouse leaves rect
						if (PtInRect(tempPoint, &gLTrakRect) == FALSE)
						{
							InRect	=	FALSE;		// This is my out button hack  *
							InvalRect( &gLTrakRect );		//** This is my out button hack 
							BeginUpdate(gMainWindow);
							ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
							EndUpdate(whichWindow);
						}
					}
					else
					{
						//when mouse enters rect after leaving
						if (PtInRect(tempPoint, &gLTrakRect))
						{
							InRect	=	TRUE;
							DrawPicture( gLTrakPict, &gLTrakRect );
						}
					}
				} while (StillDown());
				if (InRect)
				{
					// action items!!!!!!!!
					gTrackNumber = gTrackNumber - 1;
					if ( gTrackNumber < 1 ) gTrackNumber = gMaxTrackNumber;
					TrackNumber();
					DrawPicture( gNumPic[0], &gTime1Rect );
					DrawPicture( gNumPic[0], &gTime2Rect );
					DrawPicture( gNumPic[0], &gTime3Rect );
					DrawPicture( gNumPic[0], &gTime4Rect );
					gCounter = 0;
					DrawPicture( gLTrakUpPict, &gLTrakRect );
					if (gPlayDown) PlayTrack(CDid);
				}
				return;
			}
			
//Left Scan
			if ( PtInRect(rectPoint, &gLScanRect ) )		// && buttonsEnabled != FALSE )
			{
				DrawPicture( gLScanPict, &gLScanRect );
				InRect	=	TRUE;
				do
				{
					GetMouse(&tempPoint);
					if (InRect)
					{	
						//when mouse leaves rect
						if (PtInRect(tempPoint, &gLScanRect) == FALSE)
						{
							InRect	=	FALSE;		// This is my out button hack  *
							InvalRect( &gLScanRect );		//** This is my out button hack 
							BeginUpdate(gMainWindow);
							ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
							EndUpdate(whichWindow);
						}
					}
					else
					{
						//when mouse enters rect after leaving
						if (PtInRect(tempPoint, &gLScanRect))
						{
							InRect	=	TRUE;
							DrawPicture( gLScanPict, &gLScanRect );
						}
					}
				} while (StillDown());
				if (InRect)
				{
					// action items!!!!!!!!
					DrawPicture( gLScanUpPict, &gLScanRect );
				}
				return;
			}
			
//Right Scan
			if ( PtInRect(rectPoint, &gRScanRect ) )		// && buttonsEnabled != FALSE )
			{
				DrawPicture( gRScanPict, &gRScanRect );
				InRect	=	TRUE;
				do
				{
					GetMouse(&tempPoint);
					if (InRect)
					{	
						//when mouse leaves rect
						if (PtInRect(tempPoint, &gRScanRect) == FALSE)
						{
							InRect	=	FALSE;		// This is my out button hack  *
							InvalRect( &gRScanRect );		//** This is my out button hack 
							BeginUpdate(gMainWindow);
							ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
							EndUpdate(whichWindow);
						}
					}
					else
					{
						//when mouse enters rect after leaving
						if (PtInRect(tempPoint, &gRScanRect))
						{
							InRect	=	TRUE;
							DrawPicture( gRScanPict, &gRScanRect );
						}
					}
				} while (StillDown());
				if (InRect)
				{
					// action items!!!!!!!!
					DrawPicture( gRScanUpPict, &gRScanRect );
				}
				return;
			}
			
//Right Trak
			if ( PtInRect(rectPoint, &gRTrakRect ) )		// && buttonsEnabled != FALSE )
			{
				DrawPicture( gRTrakPict, &gRTrakRect );
				InRect	=	TRUE;
				do
				{
					GetMouse(&tempPoint);
					if (InRect)
					{	
						//when mouse leaves rect
						if (PtInRect(tempPoint, &gRTrakRect) == FALSE)
						{
							InRect	=	FALSE;		// This is my out button hack  *
							InvalRect( &gRTrakRect );		//** This is my out button hack 
							BeginUpdate(gMainWindow);
							ReDrawPicture( gMainWindow, GetPicture(GetWRefCon( gMainWindow)) );
							EndUpdate(whichWindow);
						}
					}
					else
					{
						//when mouse enters rect after leaving
						if (PtInRect(tempPoint, &gRTrakRect))
						{
							InRect	=	TRUE;
							DrawPicture( gRTrakPict, &gRTrakRect );
						}
					}
				} while (StillDown());
				if (InRect)
				{
					// action items!!!!!!!!
					gTrackNumber = gTrackNumber + 1;
					if ( gTrackNumber > gMaxTrackNumber ) gTrackNumber = 1;
					TrackNumber();
					DrawPicture( gNumPic[0], &gTime1Rect );
					DrawPicture( gNumPic[0], &gTime2Rect );
					DrawPicture( gNumPic[0], &gTime3Rect );
					DrawPicture( gNumPic[0], &gTime4Rect );
					gCounter = 0;
					DrawPicture( gRTrakUpPict, &gRTrakRect );
					if (gPlayDown) PlayTrack(CDid);
				}
				return;
			}
			
//Repeat
			if (PtInRect(rectPoint, &gRepeatRect ))
			{
				ForeColor( redColor);
				FrameRect(&gRepeatRect);
				gWhichButton = 10;
				gRepeatToggle = gRepeatToggle + 1;
				DrawRepeatSymbol();
				return;
			}
			
//shuffle
			if (PtInRect(rectPoint, &gShuffleRect ))
			{
				ForeColor( redColor);
				FrameRect(&gShuffleRect);
				gWhichButton = 11;
				gShuffleToggle = gShuffleToggle + 1;
				ShuffleString();
				return;
			}
			
//program
			if (PtInRect(rectPoint, &gProgramRect ))
			{
				ForeColor( redColor);
				FrameRect(&gProgramRect);
				gWhichButton = 12;
				return;
			}
			
//AB
			if (PtInRect(rectPoint, &gABRect ))
			{
				ForeColor( redColor);
				FrameRect(&gABRect);
				gWhichButton = 13;
				return;
			}
			
//leftSound
			if (PtInRect(rectPoint, &gLeftSoundRect ))
			{
				ForeColor( redColor);
				FrameRect(&gLeftSoundRect);
				gWhichButton = 14;
				gLSndToggle = gLSndToggle + 1;
				return;
			}
			
//rightSound
			if (PtInRect(rectPoint, &gRightSoundRect ))
			{
				ForeColor( redColor);
				FrameRect(&gRightSoundRect);
				gWhichButton = 15;
				gRSndToggle = gRSndToggle + 1;
				return;
			}
			
//logo
			if (PtInRect(rectPoint, &gLogoRect ))
			{
				Minimize();
				return;
			}
//slider
/*			if (PtInRect(rectPoint, &gSliderButtonRect ))
			{
				gWhichButton = 16;
				actionProc = &DrawSlider;
				coords = DragGrayRgn( gTheRgn, rectPoint, &gSliderRect, 
						&gSliderRect, 2, actionProc);
				return;
			}
*/			
			break;
	}
}
/************************** PROCEDURE: HandleMouseUp *********************************/
void	HandleMouseUp( EventRecord *eventPtr )
{

//repeat button	
	if (gWhichButton == 10)		
	{
		gWhichButton = 0;
		DrawPicture( gRepeatPic, &gRepeatRect );
	}

//shuffle button	
	if (gWhichButton == 11)		
	{
		gWhichButton = 0;
		DrawPicture( gShufflePic, &gShuffleRect );
	}

//program button	
	if (gWhichButton == 12)		
	{
		gWhichButton = 0;
		DrawPicture( gProgramPic, &gProgramRect );
	}

//AB button	
	if (gWhichButton == 13)		
	{
		gWhichButton = 0;
		DrawPicture( gABPic, &gABRect );
	}

//leftSound button	
	if (gWhichButton == 14)		
	{
		gWhichButton = 0;
		DrawLSound();
	}

//rightSound button	
	if (gWhichButton == 15)		
	{
		gWhichButton = 0;
		DrawRSound();
	}

//slider button	
	if (gWhichButton == 16)		
	{
		gWhichButton = 0;
		//DrawPicture( gSliderPic, &gSliderButtonRect );
	}
}

/*************************** PROCEDURE: HandleMenuChoice *******************************/
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
			case 128:							/* handles Apple menu (ID=128) */
				HandleAppleChoice( item );
				break;
				
			case 129:							/* handles File menu (ID=129) */
				HandleFileChoice( item );
				break;
				
			case 130:							/* handles Special menu (ID=130) */
				HandleSpecialChoice( item );
				break;
				
			case 131:							/* handles Device menu (ID=131) */
				HandleDeviceChoice( item );
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
	switch( item )
	{
		case 1:							/*choose Quit */
			gDone = true;
			break;
	}
}

/*************************** PROCEDURE: handles items selected from EDIT menu **********/
void	HandleEditChoice( short item )
{
	switch( item )
	{
		case 1:								/* choose UNDO */
			SysBeep (10);
			break;
			
		case 3:								/* choose CUT */
			SysBeep (10);
			break;
			
		case 4:								/* choose COPY */
			SysBeep (10);
			break;
			
		case 5:								/* choose PASTE */
			SysBeep (10);
			break;
			
		case 6:								/* choose CLEAR */
			SysBeep (10);
			break;
	}
}

/************************ PROCEDURE: handles items selected from Special menu ********/
void	HandleSpecialChoice( short item )
{
	switch( item )
	{
		case 1:								/* choose devices    */
			devicesWindow();
			break;
			
		case 2:								/* choose 2 Item */
			SysBeep (10);
			break;
			
		case 4:								/* choose Minimize */
			Minimize();
			break;
			
	}
}

/*************************** PROCEDURE: handles items selected from FILE menu **********/
void	HandleDeviceChoice( short item )
{
	switch( item )
	{
		case 1:							// choose 1
			SysBeep(10);
			break;
			
		case 2:							// choose 2
			SysBeep(10);
			break;
			
		case 3:							// choose 3
			SysBeep(10);
			break;
			
		case 4:							// choose 4
			SysBeep(10);
			break;
			
		case 5:							// choose 5
			SysBeep(10);
			break;
			
		case 6:							// choose 6
			SysBeep(10);
			break;
	}
}

/************************** PROCEDURE: updates the windows *****************************/
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
	Rect		drawingClipRect, windowRect;
	RgnHandle	tempRgn;
	
	if (window == gMinWindow) SysBeep(10);
	SetPort( window );
	
	tempRgn = NewRgn();
	GetClip( tempRgn );
	
	drawingClipRect = window->portRect;
	
	windowRect = window ->portRect;
	CenterPict( picture, &windowRect );
	ClipRect( &drawingClipRect );
	DrawPicture( picture, &windowRect );
	
	if (window != gMinWindow)
	{
		DrawPicture( gNumPic[gCounter2], &gTime3Rect );
		DrawPicture( gNumPic[gCounter3], &gTime2Rect );
		
		DrawPicture( gStopUpPict, &gStopRect );
		
		if (gPlayDown == TRUE)
			DrawPicture( gPlayPict, &gPlayRect );
		else 
			DrawPicture( gPlayUpPict, &gPlayRect );
			
		DrawPicture( gEjectUpPict, &gEjectRect );
		
		if (gPauseDown == TRUE)
			DrawPicture( gPausePict, &gPauseRect );
		else 
			DrawPicture( gPauseUpPict, &gPauseRect );
			
		DrawPicture( gTimeUpPict, &gTimeRect );
		DrawPicture( gLTrakUpPict, &gLTrakRect );
		DrawPicture( gLScanUpPict, &gLScanRect );
		DrawPicture( gRScanUpPict, &gRScanRect );
		DrawPicture( gRTrakUpPict, &gRTrakRect );
		DrawPicture( gSliderPic, &gSliderButtonRect );
		
		TrackNumber();
		DrawTimeString();
		DrawRepeatSymbol();
		ShuffleString();
		DrawLSound();
		DrawRSound();
	}
	else
	{
		DrawPicture( gStopUpPict, &gMinStopRect );
		
		if (gPlayDown == TRUE)
			DrawPicture( gPlayPict, &gMinPlayRect );
		else 
			DrawPicture( gPlayUpPict, &gMinPlayRect );
	}
	
	SetPort( gMainPort );				/*sets port to main window*/
}

/************************ PROCEDURE: loads the Dialog box ******************************/
void	LoadDialog( void )
{
	DialogPtr	dialog;
	Boolean		dialogExit = false;
	short		itemHit, itemType;
	Handle		itemHandle;
	Rect		itemRect;
	
	/*LoadScreen();

	dialog = GetNewDialog( 128, nil, (WindowPtr)-1L );
	
	ShowWindow( dialog );
	SetPort( dialog );						//sets port to dialog window
				
	// now make the dialog box visible
	DrawDialog (dialog);
	
	SetDialogDefaultItem( dialog, 1 );
	SetDialogCancelItem( dialog, 2 );
	
	GetDItem( dialog, 4, &itemType, &itemHandle, &itemRect);
	SetCtlValue( itemHandle, 1 );
	
	while ( ! dialogExit )
	{
		ModalDialog( nil, &itemHit );
		
		switch( itemHit )
		{
			case 1:
				SetPort(gMainPort);			//sets current port back to window "main"
				dialogExit = true;
				DisposDialog( dialog );
				break;
			case 2:
				dialogExit = true;
				DisposDialog( dialog );
				SetPort(gMainPort);			//sets port to main window
				LoadScreen();
				break;
			
			case 4:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 4, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				SysBeep(10);
				break; 
			
			case 5:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 5, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				SysBeep(10);
				break; 
		}
	}*/
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

/******************** PROCEDURE: draws the about box ***********************************/
void	devicesWindow( void)
{
	WindowPtr		window;
	Rect			pictureRect;
	PicHandle		picture;

	window = GetNewWindow( 130, nil, (WindowPtr)-1L );
	
	if ( window == nil )	/* In case program couldn't load resource */
	{
		SysBeep( 10 );
		ExitToShell();
	}
	
	SetWRefCon ( window, (long) (130) );	/*  ROM Routine  */
	ShowWindow( window );							/*  ROM Routine  */
	SetPort( window );								/*  ROM Routine  */
	
	/* loads the graphic portion */
	
	window = FrontWindow();

	pictureRect = window->portRect;

	picture = GetPicture ( 126 );
	
	if ( picture == nil )				/* in case resource didn't load */
	{
		SysBeep( 10 );
		ExitToShell();
	}

	CenterPict( picture, &pictureRect );			/*  My Routine   */
	DrawPicture( picture, &pictureRect );			/*  ROM Routine  */
	SCSILoop();
	while ( !Button() );
	DisposeWindow( window );						/*  ROM Routine  */
	
	SetPort(gMainPort);						/*sets current port back to window "main"*/
}

/******************** PROCEDURE: draws the about box ***********************************/
void	Minimize( void)
{
	WindowPtr		window;
	PicHandle		picture;
	Rect			pictureRect;
	
	window = FrontWindow();
	
	HideWindow( window );

	window = GetNewWindow( 129, nil, (WindowPtr)-1L );
	
	if ( window == nil )	/* In case program couldn't load resource */
	{
		SysBeep( 10 );
		ExitToShell();
	}
	gMinWindow = window;
	SetWRefCon ( window, (long) (300) ); /*300= Pic ID (3 butons), used n ReDrawPicture*/

	ShowWindow( window );							/*  ROM Routine  */
	SetPort( window );								/*  ROM Routine  */

	pictureRect = window->portRect;

	picture = GetPicture (300);
	
	if ( picture == nil )							/* in case resource didn't load */
	{
		SysBeep( 10 );
		ExitToShell();
	}
	
	CenterPict( picture, &pictureRect );			/*  My Routine   */
	DrawPicture( picture, &pictureRect );			/*  ROM Routine  */
	
	gMinRectTop = gMinWindow->portRect.top;
	gMinRectLeft = gMinWindow->portRect.left;
	SetRect(&gMinStopRect, gMinRectLeft, gMinRectTop     , 50, 23);
	SetRect(&gMinPlayRect, gMinRectLeft, gMinRectTop + 23, 50, gMinRectTop + 46);
	SetRect(&gMinPauseRect, gMinRectLeft, gMinRectTop + 46, 50, gMinRectTop + 69);
}

/*********************** PROCEDURE: calls SCSIRoutine 6 times ***********************/
void	SCSILoop( void )
{
	SCSIRoutine(0, 50, 43);
	SCSIRoutine(1, 50, 53);
	SCSIRoutine(2, 50, 63);
	SCSIRoutine(3, 50, 73);
	SCSIRoutine(4, 50, 83);
	SCSIRoutine(5, 50, 93);
	SCSIRoutine(6, 50, 103);
}

/*********************** PROCEDURE: does SCSI Inquiry ***********************/
OSErr	SCSIRoutine( short id, short left, short top )
{
	//short			gMyErr;
	short			compErr;
	short			compStat;
	short			compMsg;
	
	gCDBPtr = &gCDB.opCode;
	
	gCDB.opCode = kInquiryCmd;			// SCSI command code (0x00)
	gCDB.lun = 0;
	gCDB.pageCode = 0;
	gCDB.reserved = 0;
	gCDB.allocationLen = 30;					// max num of bytes target returns
	gCDB.control = 0;
	
	gTIB[0].scOpcode = scNoInc;
	gTIB[0].scParam1 = (unsigned long) gDataBuffer;		// pointer to data buffer
	gTIB[0].scParam2 = 30;			// number of bytes to be transferred
	gTIB[1].scOpcode = scStop;
	gTIB[1].scParam1 = 0;
	gTIB[1].scParam2 = 0;
	
	gMyErr = SCSIGet();
	if (gMyErr == 0)
	{
		gMyErr = SCSISelect( id );
	}
	if (gMyErr != 0)
	{
		NoDriveSCSIText( id, left, top );
		return(gMyErr);
	}
	
	gMyErr = SCSICmd( (Ptr) gCDBPtr, 6 );
	if (gMyErr == 0)
	{
		gMyErr = SCSIRead( (Ptr)(gTIB) );
		if (gMyErr != 0)
		{
			NoDriveSCSIText( id, left, top );
			return(gMyErr);
		} 
	}
	
	compErr = SCSIComplete( &compStat, &compMsg, 10 );
	if (gMyErr != 0)
	{
		NoDriveSCSIText( id, left, top );
		return(gMyErr);
	}
	
	SCSIText(id, left, top);
}

/*********************** PROCEDURE: reads TOC of CD-ROM ***********************/
void	ReadTOC( short id )
{
	short			i;
	short			compErr;
	short			compStat;
	short			compMsg;
		
	gCDBPtr2 = &gCDB12[0];
	
	for(i = 0;i < 10; i++) gCDB12[i] = 0;
	gCDB12[0] = 0x43;
	gCDB12[1] = 0x02;
	gCDB12[8] = 0x14;
	
	gTIB[0].scOpcode = scInc;
	gTIB[0].scParam1 = (unsigned long) gTOCBuff;
	gTIB[0].scParam2 = 20;							// number of bytes to be transferred
	gTIB[1].scOpcode = scStop;
	gTIB[1].scParam1 = 0;
	gTIB[1].scParam2 = 0;
	
	gMyErr = SCSIGet();
	if (gMyErr == 0)
	{
		gMyErr = SCSISelect( id );
	}
	
	gMyErr = SCSICmd(  (Ptr) gCDBPtr2, 10 );
	if (gMyErr == 0)
	{
		gMyErr = SCSIRead( (Ptr)(gTIB) );
	}
	
	compErr = SCSIComplete( &compStat, &compMsg, 600 );
	gMaxTrackNumber	=	gTOCBuff[3];
}

/*********************** Issues Play command to CD ***********************/
OSErr	PlayTrack( short id )
{
	//short			gMyErr;
	short			compErr;
	short			compStat;
	short			compMsg;
	
	gCDBPtr = &gCDB10.byte0;
	
	gCDB10.byte0 = 0x48;			// SCSI command code (0x00)
	gCDB10.byte1 = 0;
	gCDB10.byte2 = 0;
	gCDB10.byte3 = 0;
	gCDB10.byte4 = gTrackNumber;
	gCDB10.byte5 = 0;
	gCDB10.byte6 = 0;
	gCDB10.byte7 = gMaxTrackNumber;
	gCDB10.byte8 = 0;
	gCDB10.byte9 = 0;
	
	gTIB[0].scOpcode = scInc;
	gTIB[0].scParam1 = 64267450;
	gTIB[0].scParam2 = 0;
	gTIB[1].scOpcode = scStop;
	gTIB[1].scParam1 = 0;
	gTIB[1].scParam2 = 0;
	
	gMyErr = SCSIGet();
	if (gMyErr == 0)
	{
		gMyErr = SCSISelect( id );
	}
	if (gMyErr != 0)
	{
		SysBeep(10);
		return(gMyErr);
	}
	
	gMyErr = SCSICmd( (Ptr) gCDBPtr, 10 );
	if (gMyErr == 0)
	{
		gMyErr = SCSIRead( (Ptr)(gTIB) );
		if (gMyErr != 0)
		{
			SysBeep(10);
			return(gMyErr);
		} 
	}
	
	compErr = SCSIComplete( &compStat, &compMsg, 10 );
	if (gMyErr != 0)
	{
		SysBeep(10);
		return(gMyErr);
	}
}

/*********************** Issues Stop command to CD ***********************/
OSErr	StopTrack( short id )
{
	//short			gMyErr;
	short			compErr;
	short			compStat;
	short			compMsg;
	
	gCDBPtr = &gCDB10.byte0;
	
	gCDB10.byte0 = 0x4B;			// SCSI command code (0x00)
	gCDB10.byte1 = 0;
	gCDB10.byte2 = 0;
	gCDB10.byte3 = 0;
	gCDB10.byte4 = 0;
	gCDB10.byte5 = 0;
	gCDB10.byte6 = 0;
	gCDB10.byte7 = 0;
	gCDB10.byte8 = 0;
	gCDB10.byte9 = 0;
	
	gTIB[0].scOpcode = scInc;
	gTIB[0].scParam1 = 53598458;
	gTIB[0].scParam2 = 0;
	gTIB[1].scOpcode = scStop;
	gTIB[1].scParam1 = 0;
	gTIB[1].scParam2 = 0;
	
	gMyErr = SCSIGet();
	if (gMyErr == 0)
	{
		gMyErr = SCSISelect( id );
	}
	if (gMyErr != 0)
	{
		SysBeep(10);
		return(gMyErr);
	}
	
	gMyErr = SCSICmd( (Ptr) gCDBPtr, 10 );
	if (gMyErr == 0)
	{
		gMyErr = SCSIRead( (Ptr)(gTIB) );
		if (gMyErr != 0)
		{
			SysBeep(10);
			return(gMyErr);
		} 
	}
	
	compErr = SCSIComplete( &compStat, &compMsg, 10 );
	if (gMyErr != 0)
	{
		SysBeep(10);
		return(gMyErr);
	}
}

/*********************** Issues resume command to CD ***********************/
OSErr	ResumeTrack( short id )
{
	//short			gMyErr;
	short			compErr;
	short			compStat;
	short			compMsg;
	
	gCDBPtr = &gCDB10.byte0;
	
	gCDB10.byte0 = 0x4B;			// SCSI command code (0x00)
	gCDB10.byte1 = 0;
	gCDB10.byte2 = 0;
	gCDB10.byte3 = 0;
	gCDB10.byte4 = 0;
	gCDB10.byte5 = 0;
	gCDB10.byte6 = 0;
	gCDB10.byte7 = 0;
	gCDB10.byte8 = 0x01;
	gCDB10.byte9 = 0;
	
	gTIB[0].scOpcode = scInc;
	gTIB[0].scParam1 = 53599130;
	gTIB[0].scParam2 = 0;
	gTIB[1].scOpcode = scStop;
	gTIB[1].scParam1 = 0;
	gTIB[1].scParam2 = 0;
	
	gMyErr = SCSIGet();
	if (gMyErr == 0)
	{
		gMyErr = SCSISelect( id );
	}
	if (gMyErr != 0)
	{
		SysBeep(10);
		return(gMyErr);
	}
	
	gMyErr = SCSICmd( (Ptr) gCDBPtr, 10 );
	if (gMyErr == 0)
	{
		gMyErr = SCSIRead( (Ptr)(gTIB) );
		if (gMyErr != 0)
		{
			SysBeep(10);
			return(gMyErr);
		} 
	}
	
	compErr = SCSIComplete( &compStat, &compMsg, 10 );
	if (gMyErr != 0)
	{
		SysBeep(10);
		return(gMyErr);
	}
}

/*********************** Issues resume command to CD ***********************/
void	EjectCD( short id )
{
	short			compErr;
	short			compStat;
	short			compMsg;
	
	gCDBPtr = &gCDB.opCode;
	
	gCDB.opCode = 0x1B;	
	gCDB.lun = 0;
	gCDB.pageCode = 0;
	gCDB.reserved = 0;
	gCDB.allocationLen = 0x02;
	gCDB.control = 0;
	
	gTIB[0].scOpcode = scInc;
	gTIB[0].scParam1 = 22421738;
	gTIB[0].scParam2 = 0;
	gTIB[1].scOpcode = scStop;
	gTIB[1].scParam1 = 0;
	gTIB[1].scParam2 = 0;
	
	gMyErr = SCSIGet();
	if (gMyErr == 0)
	{
		gMyErr = SCSISelect( id );
	}
	
	gMyErr = SCSICmd(  (Ptr) gCDBPtr, 6 );
	if (gMyErr == 0)
	{
		gMyErr = SCSIRead( (Ptr)(gTIB) );
	}
	
	compErr = SCSIComplete( &compStat, &compMsg, 600 );
}

/*********************** PROCEDURE: writes text to the screen ***********************/
void	SCSIText( short id, short left, short top )
{
	short			i;
	
	ForeColor( redColor);
	TextSize( 9 );
	
	// DRAW HEADINGS (left, top )
	MoveTo( left, 29 );
	DrawString( "\pID" );
	MoveTo( left + 20, 29 );
	DrawString( "\pVendor" );
	MoveTo( left + 80, 29 );
	DrawString( "\pModel" );
	//MoveTo( left + 117, 29 );
	//DrawString( "\p1.0i" );
	
	//DRAW LINE
	MoveTo( left, 32 );
	DrawString( "\p__________________________" );
	
	//DRAW SCSI ID
	TextSize( 9 );
	MoveTo( left, top ); //left = 50, top=23
	if (id == 0) DrawString( "\p0" );
	if (id == 1) DrawString( "\p1" );
	if (id == 2) DrawString( "\p2" );
	if (id == 3) DrawString( "\p3" );
	if (id == 4) DrawString( "\p4" );
	if (id == 5) DrawString( "\p5" );
	if (id == 6) DrawString( "\p6" );
	
	//DRAW VENDOR
	if (gMyErr == 0)			//if there's a device there
	{
	MoveTo( left + 20, top );
	for (i=8; i < 16; i++)
		DrawChar(gDataBuffer[i]);
	
	//DRAW MODEL
	MoveTo( left + 80, top );
	for (i=16; i < 32; i++)
		DrawChar(gDataBuffer[i]);
	}
}

/*********************** PROCEDURE: writes text to the screen ***********************/
void	NoDriveSCSIText( short id, short left, short top )
{
	ForeColor( redColor);
	TextSize( 9 );
	MoveTo( left, top ); //left = 50, top=23
	if (id == 0) DrawString( "\p0" );
	if (id == 1) DrawString( "\p1" );
	if (id == 2) DrawString( "\p2" );
	if (id == 3) DrawString( "\p3" );
	if (id == 4) DrawString( "\p4" );
	if (id == 5) DrawString( "\p5" );
	if (id == 6) DrawString( "\p6" );
}

/****************** PROCEDURE: writes track number to the screen ***********************/
void	TrackNumber( void )
{
	if (gTrackNumber < 10) DrawPicture( gNumPic[0], &gTrack1Rect );
	if (gTrackNumber == 0) DrawPicture( gNumPic[0], &gTrack2Rect );
	if (gTrackNumber == 1) DrawPicture( gNumPic[1], &gTrack2Rect );
	if (gTrackNumber == 2) DrawPicture( gNumPic[2], &gTrack2Rect );
	if (gTrackNumber == 3) DrawPicture( gNumPic[3], &gTrack2Rect );
	if (gTrackNumber == 4) DrawPicture( gNumPic[4], &gTrack2Rect );
	if (gTrackNumber == 5) DrawPicture( gNumPic[5], &gTrack2Rect );
	if (gTrackNumber == 6) DrawPicture( gNumPic[6], &gTrack2Rect );
	if (gTrackNumber == 7) DrawPicture( gNumPic[7], &gTrack2Rect );
	if (gTrackNumber == 8) DrawPicture( gNumPic[8], &gTrack2Rect );
	if (gTrackNumber == 9) DrawPicture( gNumPic[9], &gTrack2Rect );
	if (gTrackNumber == 10) {
		DrawPicture( gNumPic[1], &gTrack1Rect );
		DrawPicture( gNumPic[0], &gTrack2Rect ); }
	if (gTrackNumber == 11) {
		DrawPicture( gNumPic[1], &gTrack1Rect );
		DrawPicture( gNumPic[1], &gTrack2Rect ); }
	if (gTrackNumber == 12) {
		DrawPicture( gNumPic[1], &gTrack1Rect );
		DrawPicture( gNumPic[2], &gTrack2Rect ); }
	if (gTrackNumber == 13) {
		DrawPicture( gNumPic[1], &gTrack1Rect );
		DrawPicture( gNumPic[3], &gTrack2Rect ); }
	if (gTrackNumber == 14) {
		DrawPicture( gNumPic[1], &gTrack1Rect );
		DrawPicture( gNumPic[4], &gTrack2Rect ); }
	if (gTrackNumber == 15) {
		DrawPicture( gNumPic[1], &gTrack1Rect );
		DrawPicture( gNumPic[5], &gTrack2Rect ); }
	if (gTrackNumber == 16) {
		DrawPicture( gNumPic[1], &gTrack1Rect );
		DrawPicture( gNumPic[6], &gTrack2Rect ); }
	if (gTrackNumber == 17) {
		DrawPicture( gNumPic[1], &gTrack1Rect );
		DrawPicture( gNumPic[7], &gTrack2Rect ); }
	if (gTrackNumber == 18) {
		DrawPicture( gNumPic[1], &gTrack1Rect );
		DrawPicture( gNumPic[8], &gTrack2Rect ); }
	if (gTrackNumber == 19) {
		DrawPicture( gNumPic[1], &gTrack1Rect );
		DrawPicture( gNumPic[9], &gTrack2Rect ); }
	if (gTrackNumber == 20) {
		DrawPicture( gNumPic[2], &gTrack1Rect );
		DrawPicture( gNumPic[0], &gTrack2Rect ); }
}

/********************* PROCEDURE: writes time string to the screen *********************/
void	DrawTimeString( void )
{
	TextSize( 9 );
	ForeColor( blueColor);
	
	if (gTimeToggle == 4) gTimeToggle = 1;
	if (gTimeToggle == 1) { MoveTo (130, 39 ); DrawString( "\pTRACK ELAPSED TIME" ); }
	if (gTimeToggle == 2) { MoveTo (139, 39 ); DrawString( "\pTRACK REMAINING" ); }
	if (gTimeToggle == 3) { MoveTo (139, 39 ); DrawString( "\pTOTAL REMAINING" ); }
}

/********************* PROCEDURE: writes time string to the screen *********************/
void	DrawRepeatSymbol( void )
{
	if (gRepeatToggle == 4) gRepeatToggle = 1;
	if (gRepeatToggle == 1) DrawPicture( gClearRepeatPic, &gRepeatPicRect );
	if (gRepeatToggle == 2) DrawPicture( gTrackRepeatPic, &gRepeatPicRect );
	if (gRepeatToggle == 3) DrawPicture( gCDRepeatPic, &gRepeatPicRect );
}

/********************* PROCEDURE: writes time string to the screen *********************/
void	ShuffleString( void )
{	Rect	tempRect;

	MoveTo ( 87, 65 );
	TextSize( 9 );
	ForeColor( blueColor);
	
	if (gShuffleToggle == 3) gShuffleToggle = 1;
	if (gShuffleToggle == 1) 
	{
		SetRect( &tempRect, 87, 56, 127, 65 );
		DrawPicture( gTextStripPic, &tempRect );
	}
	if (gShuffleToggle == 2) DrawString( "\pRandom" );
}

/********************* PROCEDURE: writes time string to the screen *********************/
void	DrawLSound( void )
{
	if ( gLSndToggle == 1 )DrawPicture( gLSoundPic, &gLeftSoundRect );
	if ( gLSndToggle == 2 )DrawPicture( gLSoundUpPic, &gLeftSoundRect );
	if ( gLSndToggle == 2 ) gLSndToggle = 0;
}

/********************* PROCEDURE: writes time string to the screen *********************/
void	DrawRSound( void )
{
	if ( gRSndToggle == 1 )DrawPicture( gRSoundPic, &gRightSoundRect );
	if ( gRSndToggle == 2 )DrawPicture( gRSoundUpPic, &gRightSoundRect );
	if ( gRSndToggle == 2 ) gRSndToggle = 0;
}

/********************* PROCEDURE: beeps *********************/
void	DrawSlider( void )
{	
	Point	curMouse;

	//SysBeep(10);
	//GetMouse(&curMouse);
	//GlobalToLocal(&curMouse);
	//OffsetRect(&gSliderButtonRect, 0, curMouse.v - 30);
	//gSliderRect.left = 282;
	//gSliderRect.top = curMouse.v;
	//DrawPicture( g1Pic, &gSliderButtonRect );
}
