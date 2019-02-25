 /****************************************************************************
This program scans the Macintosh SCSI bus looking for SCSI devices. If it finds one,
it sends a SCSI inquiry and then displays the SCSI ID, firmware version, 
manufacturer name and model number.

****************************************************************************/
#include <scsi.h>
#include <devices.h>

/*****************************************************************************/
#define	IsHighBitSet( longNum )		( (longNum >> 23) & 1 )
#define	SetHighByte( longNum )		( longNum |= 0xFF000000 )
#define	ClearHighByte( longNum )	( longNum &= 0x00FFFFFF )

#define k1stScreenID				128		/* Opening screen PICT ID */
#define kAbtScreenID				127		/* About box screen PICT ID */
#define kInquiryCmd				0x12	/* SCSI Inquiry */
#define kCompletionTimeout			300		/* SCSI Inquiry */
	
typedef struct						/* SCSI Inquiry */
{
	char	opCode;
	char	lun;
	char	pageCode;
	char	reserved;
	char	allocationLen;
	char	control;
} CDBType;

/********************************* Globals *********************************/
Boolean			gDone;
GrafPtr			mainPort;					/*sets a global port for main window*/
short			IDNo;
Boolean			ScrollFlag = FALSE;
short			gWhichButton;				/* used by HandleMouseDown/Up */
CDBType			CDB;						/* SCSI Inquiry */
SCSIInstr		TIB[2];						/* SCSI Inquiry */
char			*cdbPtr;					/* SCSI Inquiry */
char			myDataBuffer[40];			/* SCSI Inquiry */
Rect			ID0Rect;					/*for testing Rect*/
Rect			ID1Rect;					/*for testing Rect*/
Rect			ID2Rect;					/*for testing Rect*/
Rect			ID3Rect;					/*for testing Rect*/
Rect			ID4Rect;					/*for testing Rect*/
Rect			ID5Rect;					/*for testing Rect*/
Rect			ID6Rect;					/*for testing Rect*/
Handle			iconHandle;					/*for testing Rect*/
short			gMySCSIID;					/* SCSI Inquiry */


/***********************  My Routines (All Procedures)  **********************/
void	ToolBoxInit( void );
void	CreateWindow( void );
void	aboutBox( void );
void	MenuBarInit( void );
void	LoadScreen( void );
void	CenterPict( PicHandle picture, Rect *destRectPtr );
void	EventLoop( void );							/* for "Quit" from FILE menu */
void	DoEvent( EventRecord *eventPtr );			/* for "Quit" from FILE menu */
void	HandleNull( EventRecord *eventPtr );		/* for "Quit" from FILE menu */
void	HandleMouseDown( EventRecord *eventPtr );	/* for "Quit" from FILE menu */
void	HandleMouseUp( EventRecord *eventPtr );		/* for "mouse up" */
OSErr	SCSIRoutine( void );						/* for "SCSI Inquiry" */
void	SCSIText( void );						/* for "SCSI Inquiry" */
void	ErrorText( void );						/* for "SCSI Inquiry" */
void	GestaltText( void );
void	HandleMenuChoice( long menuChoice );		/* for "Quit" from FILE menu */
void	HandleAppleChoice( short item );			/* for items from APPLE menu */
void	HandleFileChoice( short item );				/* for items from FILE menu  */
void	DoUpdate( EventRecord *eventPtr );					/* for updating window   */
void	ReDrawPicture( WindowPtr window, PicHandle picture );/* for updating window   */		={ 0x303c, 0x0306, 0xAA68 };


/*------------------------ MAIN MAIN MAIN MAIN ----------------------------*/
void	main( void)
{
	ToolBoxInit();							/*  My Routine   */
	CreateWindow();							/*  My Routine   */
	MenuBarInit();							/*  My Routine   */
	LoadScreen();							/*  My Routine   */
	EventLoop();							/*  My Routine   */		
}
/*-------------------------------------------------------------------------*/


/******************* PROCEDURE: Initializes the toolbox **********************/
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

/************ PROCEDURE: creates window from resource fork ****************/
void	CreateWindow( void )
{
	WindowPtr		window;

	window = GetNewWindow( k1stScreenID, nil, (WindowPtr)-1L );	/* k1stScreenID is window ID 128*/
	
	if ( window == nil )	/* In case program couldn't load resource */
	{
		SysBeep( 10 );
		ExitToShell();
	}

	SetWRefCon ( window, (long) (k1stScreenID) );	/* k1stScreenID is window ID 128*/
	ShowWindow( window );							/*  ROM Routine  */
	SetPort( window );								/*  ROM Routine  */
	GetPort( &mainPort );			/*makes main window the main port to draw in*/
}

/************** PROCEDURE: draws and inserts a menu ***************************/
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

/********************* PROCEDURE: loads opening screen ************************/
void	LoadScreen( void )
{
	Rect		pictureRect;
	WindowPtr	window;
	PicHandle	picture;
	ControlHandle	button;				/*for button support*/

	window = FrontWindow();

	pictureRect = window->portRect;

	picture = GetPicture (k1stScreenID);
	
	if ( picture == nil )							/* in case resource didn't load */
	{
		SysBeep( 10 );
		ExitToShell();
	}
	
	IDNo = 0;
	ScrollFlag = FALSE;

	CenterPict( picture, &pictureRect );			/*  My Routine   */
	DrawPicture( picture, &pictureRect );			/*  ROM Routine  */
}

/**************** PROCEDURE: centers picture in window ***********************/
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

/******************** PROCEDURE: waits for an event *************************/
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

/******************* PROCEDURE: doEvent ********************************/
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

/******************* PROCEDURE: HandleNull ********************************/
void	HandleNull( EventRecord *eventPtr )
{
	
}
/**************** PROCEDURE: HandleMouseDown *********************************/
void	HandleMouseDown( EventRecord *eventPtr )
{
	WindowPtr		whichWindow;
	short			thePart;
	long			menuChoice, refNum;
	Point			rectPoint, thePoint;				/*for button support*/
	ControlHandle	theControl, refNumber;				/*for button support*/
	
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
			if (theControl != 0)
			{
				thePart = TrackControl( theControl, thePoint, nil );
				if ( thePart == inButton )		/* check partcode*/	
				{
					refNum = GetCRefCon(theControl);
					if (refNum == 0)	/*gestalt = 0, SCSI calls = 1*/
					{
						GestaltText();
					}
					else if (refNum == 1)
					{
						SCSIRoutine();
					}
				}
			}
			
			if (PtInRect(rectPoint, &ID0Rect ))
			{
				iconHandle = GetResource( 'ICON', 5004 );
				PlotIcon( &ID0Rect, iconHandle );
				gWhichButton = 8;
				gMySCSIID = 0;
			}
			
			if (PtInRect(rectPoint, &ID1Rect ))
			{
				iconHandle = GetResource( 'ICON', 5006 );
				PlotIcon( &ID1Rect, iconHandle );
				gWhichButton = 1;
				gMySCSIID = 1;
			}
			
			if (PtInRect(rectPoint, &ID2Rect ))
			{
				iconHandle = GetResource( 'ICON', 5008 );
				PlotIcon( &ID2Rect, iconHandle );
				gWhichButton = 2;
				gMySCSIID = 2;
			}
			
			if (PtInRect(rectPoint, &ID3Rect ))
			{
				iconHandle = GetResource( 'ICON', 5010 );
				PlotIcon( &ID3Rect, iconHandle );
				gWhichButton = 3;
				gMySCSIID = 3;
			}
			
			if (PtInRect(rectPoint, &ID4Rect ))
			{
				iconHandle = GetResource( 'ICON', 5012 );
				PlotIcon( &ID4Rect, iconHandle );
				gWhichButton = 4;
				gMySCSIID = 4;
			}
			
			if (PtInRect(rectPoint, &ID5Rect ))
			{
				iconHandle = GetResource( 'ICON', 5014 );
				PlotIcon( &ID5Rect, iconHandle );
				gWhichButton = 5;
				gMySCSIID = 5;
			}
			
			if (PtInRect(rectPoint, &ID6Rect ))
			{
				iconHandle = GetResource( 'ICON', 5016 );
				PlotIcon( &ID6Rect, iconHandle );
				gWhichButton = 6;
				gMySCSIID = 6;
			}
			break;
	}
}
/************************** PROCEDURE: HandleMouseUp **************************/
void	HandleMouseUp( EventRecord *eventPtr )
{
	if (gWhichButton == 8)
	{
		iconHandle = GetResource( 'ICON', 5003 );
		PlotIcon( &ID0Rect, iconHandle );
		SCSIRoutine();
		gWhichButton = 0;
	}
	if (gWhichButton == 1)
	{
		iconHandle = GetResource( 'ICON', 5005 );
		PlotIcon( &ID1Rect, iconHandle );
		SCSIRoutine();
		gWhichButton = 0;
	}
	
	if (gWhichButton == 2)
	{
		iconHandle = GetResource( 'ICON', 5007 );
		PlotIcon( &ID2Rect, iconHandle );
		SCSIRoutine();
		gWhichButton = 0;
	}
	
	if (gWhichButton == 3)
	{
		iconHandle = GetResource( 'ICON', 5009 );
		PlotIcon( &ID3Rect, iconHandle );
		SCSIRoutine();
		gWhichButton = 0;
	}
	
	if (gWhichButton == 4)
	{
		iconHandle = GetResource( 'ICON', 5011 );
		PlotIcon( &ID4Rect, iconHandle );
		SCSIRoutine();
		gWhichButton = 0;
	}
	
	if (gWhichButton == 5)
	{
		iconHandle = GetResource( 'ICON', 5013 );
		PlotIcon( &ID5Rect, iconHandle );
		SCSIRoutine();
		gWhichButton = 0;
	}
	
	if (gWhichButton == 6)
	{
		iconHandle = GetResource( 'ICON', 5015 );
		PlotIcon( &ID6Rect, iconHandle );
		SCSIRoutine();
		gWhichButton = 0;
	}
}

/********************** PROCEDURE: HandleMenuChoice ************************/
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
				
		}
		HiliteMenu( 0 );
	}
}

/*************** PROCEDURE: handles items selected from APPLE menu *********/
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

/***************** PROCEDURE: handles items selected from FILE menu **********/
void	HandleFileChoice( short item )
{
	switch( item )
	{	
		case 1:							/*choose Print */
			SysBeep(10);
			break;
			
		case 3:							/*choose Quit */
			gDone = true;
			break;
	}
}

/******************** PROCEDURE: updates the windows *********************/
void	DoUpdate( EventRecord *eventPtr )
{
	short			pictureID;
	PicHandle		picture;
	WindowPtr		window;
	
	window = (WindowPtr)eventPtr->message;
	
	BeginUpdate( window );						/* ROM Routine */
	pictureID = GetWRefCon	( window );
	picture = GetPicture( pictureID + IDNo );
	
	ReDrawPicture( window, picture );			/* My Routine  */	
	EndUpdate( window );						/* ROM Routine */
} 

/*************** PROCEDURE: redraws the window contents *********************/
void	ReDrawPicture( WindowPtr window, PicHandle picture )
{
	Rect			drawingClipRect, windowRect;
	RgnHandle		tempRgn;
	ControlHandle	control;						/*for button support*/
	
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
	
	//				   L  T  R   B
	SetRect( &ID0Rect, 1, 1, 33, 33 );
	SetRect( &ID1Rect, 1, 1, 33, 33 );
	SetRect( &ID2Rect, 1, 1, 33, 33 );
	SetRect( &ID3Rect, 1, 1, 33, 33 );
	SetRect( &ID4Rect, 1, 1, 33, 33 );
	SetRect( &ID5Rect, 1, 1, 33, 33 );
	SetRect( &ID6Rect, 1, 1, 33, 33 );
	//					   L   T
	OffsetRect(&ID0Rect, 540, 10);
	OffsetRect(&ID1Rect, 540, 45);
	OffsetRect(&ID2Rect, 540, 80);
	OffsetRect(&ID3Rect, 540, 115);
	OffsetRect(&ID4Rect, 540, 150);
	OffsetRect(&ID5Rect, 540, 185);
	OffsetRect(&ID6Rect, 540, 220);
	
	iconHandle = GetResource( 'ICON', 5003 );
	PlotIcon( &ID0Rect, iconHandle );
	iconHandle = GetResource( 'ICON', 5005 );
	PlotIcon( &ID1Rect, iconHandle );
	iconHandle = GetResource( 'ICON', 5007 );
	PlotIcon( &ID2Rect, iconHandle );
	iconHandle = GetResource( 'ICON', 5009 );
	PlotIcon( &ID3Rect, iconHandle );
	iconHandle = GetResource( 'ICON', 5011 );
	PlotIcon( &ID4Rect, iconHandle );
	iconHandle = GetResource( 'ICON', 5013 );
	PlotIcon( &ID5Rect, iconHandle );
	iconHandle = GetResource( 'ICON', 5015 );
	PlotIcon( &ID6Rect, iconHandle );
	
	SetPort( mainPort );				/*sets port to main window*/
}

/************* PROCEDURE: draws the about box ****************************/
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
	
	SetPort(mainPort);						/*sets current port back to window "main"*/
}

/*********************** PROCEDURE: does SCSI Inquiry ***********************/
OSErr	SCSIRoutine( void )
{
	short			myErr;
	short			compErr;
	short			compStat;
	short			compMsg;
	
	cdbPtr = &CDB.opCode;
	
	CDB.opCode = kInquiryCmd;			// SCSI command code (0x00)
	CDB.lun = 0;
	CDB.pageCode = 0;
	CDB.reserved = 0;
	CDB.allocationLen = 30;					// max num of bytes target returns
	CDB.control = 0;
	
	TIB[0].scOpcode = scNoInc;
	TIB[0].scParam1 = (unsigned long) myDataBuffer;		// pointer to data buffer
	TIB[0].scParam2 = 30;			// number of bytes to be transferred
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
	
	myErr = SCSICmd( (Ptr) cdbPtr, 6 );
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

/************** PROCEDURE: writes text to the screen ***********************/
void	SCSIText( void )
{
	Rect			TextRect;	
	
	//				     L  T    R    B
	SetRect( &TextRect, 25, 250, 370, 300 );
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
	
	//MoveTo( 30, 300 );
	DrawString( "\pFirmware: " );
	DrawChar(myDataBuffer[32]);
	DrawChar(myDataBuffer[33]);
	DrawChar(myDataBuffer[34]);
	DrawChar(myDataBuffer[35]);
	DrawChar(myDataBuffer[36]);
	DrawChar(myDataBuffer[37]);
	DrawChar(myDataBuffer[38]);
	DrawChar(myDataBuffer[39]);
	
	ForeColor( blackColor);
}

/******************** PROCEDURE: writes text to the screen *****************/
void	ErrorText( void )
{
	Rect			TextRect;	
	
	SetRect( &TextRect, 25, 250, 370, 300 );
	EraseRect( &TextRect );
	
	TextSize( 12 );
	MoveTo( 30, 260 );
	ForeColor( redColor);
	DrawString( "\pError! No device present at this SCSI ID." );
	
	ForeColor( blackColor);
}

/*********** PROCEDURE: writes text to the screen ***********************/
void	GestaltText( void )
{
	Rect			TextRect;	
	
	SetRect( &TextRect, 25, 250, 220, 300 );
	EraseRect( &TextRect );
	
	TextSize( 12 );
	MoveTo( 30, 260 );
	DrawString( "\pNo Gestalt calls yet." );
	MoveTo( 30, 280 );
	DrawString( "\pBut my EraseRect call works!" );
	MoveTo( 30, 300 );
	DrawString( "\pBeta version 1.0a" );
}
