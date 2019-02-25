/****************************************************************************
This program communicates with a Macintosh Printer and prints off a series
of production travelers based on text the user types in.

****************************************************************************/
#include <PrintTraps.h>
#include <stdio.h>
#include <pascal.h>
#include <time.h>							/*for writing the date*/
#include <Sound.h>							/*for playing sound*/

/***************************************************************************************/
#define	IsHighBitSet( longNum )		( (longNum >> 23) & 1 )
#define	SetHighByte( longNum )		( longNum |= 0xFF000000 )
#define	ClearHighByte( longNum )	( longNum &= 0x00FFFFFF )

#define k1stScreenID				128		/* Opening screen PICT ID */
#define kAbtScreenID				127		/* About box screen PICT ID */

/********************************* Globals *********************************************/
Boolean			gDone;
Boolean			DoPageSetUp = TRUE;		/*print support*/
Str255          word, wordb, wordc;
Str255			soString, verString, intString;		/*global S.O. & initials stings*/
GrafPtr			mainPort;					/*sets a global port for Traveler window*/
char 			s[40];						/*for writing the date*/

/*********************************  My Routines (All Procedures)  **********************/
void	ToolBoxInit( void );
void	CreateWindow( void );
void	aboutBox( void );
void	LoadDialog( void );
void	LoadCapDialog();
void	LoadSufDialog();
void	MenuBarInit( void );
void	LoadScreen( void );
void	CenterPict( PicHandle picture, Rect *destRectPtr );
void	EventLoop( void );							/* for "Quit" from FILE menu */
void	DoEvent( EventRecord *eventPtr );			/* for "Quit" from FILE menu */
void	HandleNull( EventRecord *eventPtr );		/* for "Quit" from FILE menu */
void	HandleMouseDown( EventRecord *eventPtr );	/* for "Quit" from FILE menu */
void	HandleMenuChoice( long menuChoice );		/* for "Quit" from FILE menu */
void	HandleAppleChoice( short item );			/* for items from APPLE menu */
void	HandleFileChoice( short item );				/* for items from FILE menu  */
void	HandleEditChoice( short item );				/* for items from EDIT menu  */
void	HandleSpecialChoice( short item );			/* for items from SPECIAL menu */
void	DoUpdate( EventRecord *eventPtr );					/* for updating window   */
void	ReDrawPicture( WindowPtr window, PicHandle picture );/* for updating window  */
void	DrawingText( void );								/*print support*/
void	InfoDialogs( void );
void	IntDialogs( void);
void	CancelDialogs( void);
void	PreSoftwareVersion( void );							/*supports sepcial menu*/
Boolean	SoftwareVersion( StringHandle oldTextHandle );		/*supports sepcial menu*/
void	PrintTrav( void );									/*print support*/
void	PlaySound( void );

/* see tech note 304 */
pascal	OSErr	SetDialogDefaultItem( DialogPtr theDialog, short newItem )
					={ 0x303c, 0x0304, 0xAA68 };
pascal	OSErr	SetDialogCancelItem( DialogPtr theDialog, short newItem )
					={ 0x303c, 0x0305, 0xAA68 };
pascal	OSErr	SetDialogTracksCursor( DialogPtr theDialog, Boolean tracks )
					={ 0x303c, 0x0306, 0xAA68 };


/*-------------------------------- MAIN MAIN MAIN MAIN --------------------------------*/
void	main( void)
{
	ToolBoxInit();							/*  My Routine   */
	CreateWindow();							/*  My Routine   */
	MenuBarInit();							/*  My Routine   */
	LoadScreen();							/*  My Routine   */
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
	MaxApplZone();							/*print support*/
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

	SetWRefCon ( window, (long) (k1stScreenID) );	/* k1stScreenID is window ID 128*/
	ShowWindow( window );							/*  ROM Routine  */
	SetPort( window );								/*  ROM Routine  */
	GetPort( &mainPort );			/*makes Traveler window the main port to draw in*/
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

/********************* PROCEDURE: loads opening screen *********************************/
void	LoadScreen( void )
{
	Rect		pictureRect;
	WindowPtr	window;
	PicHandle	picture;

	window = FrontWindow();

	pictureRect = window->portRect;

	picture = GetPicture (k1stScreenID);
	
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
	WindowPtr	whichWindow;
	short		thePart;
	long		menuChoice;
	
	thePart = FindWindow( eventPtr->where, &whichWindow );
	switch( thePart )
	{
		case inMenuBar:
			menuChoice = MenuSelect( eventPtr->where );
			HandleMenuChoice( menuChoice );
			break;
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
				
			case 130:							/* handles Edit menu (ID=130) */
				HandleEditChoice( item );
				break;
				
			case 131:							/* handles Special menu (ID=131) */
				HandleSpecialChoice( item );
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
		case 1:							/*choose New travler */
			LoadDialog();
			break;
			
		case 2:							/*choose Print */
			InfoDialogs();
			break;
			
		case 4:							/*choose Quit */
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
		case 1:								/* choose Software version */
			PreSoftwareVersion();
			break;
			
		case 2:								/* choose Majid Honavar */
			PlaySound();
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
	SetPort( mainPort );				/*sets port to Travler window*/
}

/***************** PROCEDURE: loads the Prefix Dialog box ******************************/
void	LoadDialog( void )
{
	DialogPtr	dialog;
	Boolean		dialogExit = false;
	short		itemHit, itemType;
	Handle		itemHandle;
	Rect		itemRect;
	
	LoadScreen();

	dialog = GetNewDialog( 128, nil, (WindowPtr)-1L );
	
	ShowWindow( dialog );
	SetPort( dialog );						/*sets port to dialog window*/
				
	/* now make the dialog box visible */
	DrawDialog (dialog);
	
	SetDialogDefaultItem( dialog, 1 );
	SetDialogCancelItem( dialog, 2 );
	
	GetDItem( dialog, 5, &itemType, &itemHandle, &itemRect);
	SetCtlValue( itemHandle, 1 );
	
	/* draws default model number if user just selects "OK" */
	strcpy( word, "\pCPKD-");
	
	while ( ! dialogExit )
	{
		ModalDialog( nil, &itemHit );
		
		switch( itemHit )
		{
			case 1:
				SetPort(mainPort);			/*sets current port back to window "trav"*/
				MoveTo(100, 39);
				DrawString( word );
				dialogExit = true;
				DisposDialog( dialog );
				LoadCapDialog();
				break;
			case 2:
				dialogExit = true;
				DisposDialog( dialog );
				SetPort(mainPort);			/*sets port to Traveler window*/
				LoadScreen();
				break;
			
			case 4:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 4, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pCPK-");
				break; 
			
			case 5:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 5, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pCPKD-");
				break; 
			
			case 6:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 6, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pCPKT-");
				break; 
			
			case 7:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 7, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pCPX-");
				break; 
			
			case 8:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 8, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pDDi-");
				break; 
			
			case 9:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 9, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pIIci/Q7i-");
				break; 
			
			case 10:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 10, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pIIcx/ci-");
				break; 
			
			case 11:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 11, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pIIi-");
				break; 
			
			case 12:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 12, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pIIsi/LC-");
				break; 
			
			case 13:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 13, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pIIv/650L-");
				break; 
			
			case 14:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 14, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pIIv/650U-");
				break; 
			
			case 15:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 15, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pMCD-");
				break; 
			
			case 16:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 16, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pMCi-");
				break; 
			
			case 17:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 17, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pMO-");
				break; 
			
			case 18:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 18, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pMR-");
				break; 
			
			case 19:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 19, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pMS-");
				break; 
			
			case 20:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 20, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pMT-");
				break;
			
			case 21:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 21, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pPBi-");
				break; 
			
			case 22:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 22, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pQ8i-");
				break; 
			
			case 23:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 23, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pQ8i/i-");
				break; 
			
			case 24:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 24, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pQ8e/e-");
				break; 
			
			case 25:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 25, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pQ8RUI-");
				break; 
			
			case 26:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 26, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pQ9i-");
				break; 
			
			case 27:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 27, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pQ9i/i-");
				break; 
			
			case 28:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 28, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pQ9e/e-");
				break; 
			
			case 29:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 29, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pQ9RUI-");
				break;
			
			case 30:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 30, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pRC-");
				break; 
			
			case 31:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 31, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pSB-");
				break; 
			
			case 32:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 32, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pSBS-");
				break; 
			
			case 33:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 33, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pSBT-");
				break; 
			
			case 34:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 34, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pSS-");
				break; 
			
			case 35:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 35, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pSE-IIi-");
				break; 
			
			case 36:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 36, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\p610i-");
				break;
			
			case 37:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 37, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( word, "\pADV");
				break; 
		}
	}
			
}

/**************** PROCEDURE: loads the Capacity Dialog box *****************************/
void	LoadCapDialog( void )
{
	DialogPtr	dialog;
	Boolean		dialogExit = false;
	short		itemHit, itemType;
	Handle		itemHandle;
	Rect		itemRect;

	dialog = GetNewDialog( 129, nil, (WindowPtr)-1L );
	
	ShowWindow( dialog );
	SetPort( dialog );

	/* now make it visible */
	DrawDialog (dialog);
	
	SetDialogDefaultItem( dialog, 1 );
	SetDialogCancelItem( dialog, 2 );
	
	GetDItem( dialog, 4, &itemType, &itemHandle, &itemRect);
	SetCtlValue( itemHandle, 1 );
	
	/* draws default model number if user just selects "OK" */
	strcpy( wordb, "\p45");
	
	while ( ! dialogExit )
	{
		ModalDialog( nil, &itemHit );
		
		switch( itemHit )
		{
			case 1:
				SetPort( mainPort );			/*sets port to Traveler window*/
				DrawString( wordb );
				dialogExit = true;
				DisposDialog( dialog );
				LoadSufDialog();
				break;
			case 2:
				dialogExit = true;
				DisposDialog( dialog );
				SetPort( mainPort );			/*sets port to Traveler window*/
				LoadScreen();
				break;
			
			case 4:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 4, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p45");
				break; 
			
			case 5:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 5, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\pR45");
				break; 
			
			case 6:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 6, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p4545");
				break; 
			
			case 7:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 7, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p88");
				break; 
			
			case 8:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 8, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p90C");
				break; 
			
			case 9:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 9, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p9090C");
				break; 
			
			case 10:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 10, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\pR90C");
				break; 
			
			case 11:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 11, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p120");
				break; 
			
			case 12:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 12, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p128");
				break; 
			
			case 13:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 13, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\pMO128");
				break; 
			
			case 14:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 14, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p150");
				break; 
			
			case 15:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 15, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p155");
				break; 
			
			case 16:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 16, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p170");
				break; 
			
			case 17:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 17, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p200");
				break; 
			
			case 18:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 18, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p203");
				break; 
			
			case 19:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 19, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p240");
				break; 
			
			case 20:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 20, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p270");
				break;
			
			case 21:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 21, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p340");
				break; 
			
			case 22:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 22, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p345");
				break; 
			
			case 23:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 23, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p500");
				break; 
			
			case 24:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 24, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p515");
				break; 
			
			case 25:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 25, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p540");
				break; 
			
			case 26:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 26, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p600");
				break; 
			
			case 27:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 27, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p1000");
				break; 
			
			case 28:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 28, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p1001");
				break; 
			
			case 29:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 29, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p1010");
				break;
			
			case 30:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 30, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p1012");
				break; 
			
			case 31:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 31, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p1015");
				break; 
			
			case 32:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 32, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p1030");
				break; 
			
			case 33:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 33, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p1080");
				break; 
			
			case 34:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 34, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p1240");
				break; 
			
			case 35:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 35, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p1300");
				break;
			
			case 36:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 36, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p1350");
				break;
			
			case 37:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 37, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p1601");
				break;
			
			case 38:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 38, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p2000");
				break;
			
			case 39:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 39, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\pD200");
				break;
			
			case 40:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 40, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p2000");
				break;
			
			case 41:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 41, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p2012");
				break;
			
			case 42:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 42, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p2020");
				break;
			
			case 43:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 43, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p2024");
				break;
			
			case 44:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 44, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p2050");
				break;
			
			case 45:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 45, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p2130");
				break;
			
			case 46:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 46, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p2480");
				break;
			
			case 47:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 47, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p2780");
				break;
			
			case 48:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 48, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p3800");
				break;
			
			case 49:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 49, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p4100");
				break;
			
			case 50:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 50, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p4260");
				break;
			
			case 51:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 51, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p5000");
				break;
			
			case 52:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 52, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p5560");
				break;
			
			case 53:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 53, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p8000");
				break;
			
			case 54:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 54, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\pD8000");
				break;
			
			case 55:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 55, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\pD16000");
				break;
			
			case 56:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 56, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\pCD");
				break;
			
			case 57:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 57, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\pROM");
				break;
			
			case 58:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 58, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\pSMO-650");
				break;
			
			case 59:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 59, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\pTMO-1000");
				break;
			
			case 60:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 60, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\pTMO-1300");
				break;
			
			case 61:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 61, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\pDAT16-4");
				break;
			
			case 62:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 62, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p44");
				break;
			
			case 63:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 63, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordb, "\p200");
				break;
		}
	}
}

/**************** PROCEDURE: loads the Suffix Dialog box *****************************/
void	LoadSufDialog( void )
{
	DialogPtr	dialog;
	Boolean		dialogExit = false;
	short		itemHit, itemType;
	Handle		itemHandle;
	Rect		itemRect;

	dialog = GetNewDialog( 130, nil, (WindowPtr)-1L );
	
	ShowWindow( dialog );
	SetPort( dialog );

	/* now make it visible */
	DrawDialog (dialog);
	
	SetDialogDefaultItem( dialog, 1 );
	SetDialogCancelItem( dialog, 2 );
	
	GetDItem( dialog, 4, &itemType, &itemHandle, &itemRect);
	SetCtlValue( itemHandle, 1 );
	
	/*draws default suffix in case user just clicks "ok"*/
	strcpy( wordc, "\p ");
	
	while ( ! dialogExit )
	{
		ModalDialog( nil, &itemHit );
		
		switch( itemHit )
		{
			case 1:
				SetPort( mainPort );			/*sets port to Traveler window*/
				DrawString( wordc );
				dialogExit = true;
				DisposDialog( dialog );
				break;
				
			case 2:
				dialogExit = true;
				DisposDialog( dialog );
				SetPort( mainPort );			/*sets port to Traveler window*/
				LoadScreen();
				break;
			
			case 4:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 4, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\p");
				break; 
			
			case 5:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 5, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pR");
				break; 
			
			case 6:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 6, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\p-R");
				break; 
			
			case 7:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 7, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pM");
				break; 
			
			case 8:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 8, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\p/NP");
				break; 
			
			case 9:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 9, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\p/NPR");
				break; 
			
			case 10:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 10, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\px");
				break; 
			
			case 11:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 11, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pp");
				break;
			
			case 12:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 12, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pE");
				break;
			
			case 13:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 13, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pI(1)");
				break;
			
			case 14:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 14, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pI(2)");
				break;
			
			case 15:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 15, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pI(3)");
				break;
			
			case 16:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 16, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pI(4)");
				break;
			
			case 17:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 17, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pRI(5)");
				break;
			
			case 18:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 18, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pRI(6)");
				break;
			
			case 19:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 19, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pRI(7)");
				break;
			
			case 20:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 20, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pRE");
				break;
			
			case 21:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 21, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pMOE");
				break;
			
			case 22:
				SetCtlValue( itemHandle, 0 );
				GetDItem( dialog, 22, &itemType, &itemHandle, &itemRect);
				SetCtlValue( itemHandle, 1 );
				strcpy( wordc, "\pMOI(  )");
				break; 
		}
	}
}

/************ PROCEDURE: writes the date, version, & S.O. on screen Traveler************/
void	InfoDialogs( void)
{
	DialogPtr	dialog;
	Boolean		dialogExit = false;
	short		itemHit, itemType;
	Handle		itemHandle;
	Rect		itemRect;
	/*for writing the date*/
	time_t 		now;
	struct tm 	*date;
	
	/*for writing the date*/
	now = time( NULL );
	date = localtime ( &now );
	strftime( s, 40, "%m/%d/%y", date);
	CtoPstr( s );
	MoveTo( 85, 56 );
	SetPort(mainPort);					/*sets port to Traveler window*/
	DrawString( s );
	
	/*for writing the software version number*/
	MoveTo( 2, 10 );
	DrawString( * GetString( 128 ) );
	
	/*loading the Shop Order dialog Box*/
	dialog = GetNewDialog( 131, nil, (WindowPtr)-1L );
	
	if ( dialog == nil )	/* In case program couldn't load resource */
	{
		SysBeep( 10 );
	}
	
	ShowWindow( dialog );
	SetPort( dialog );						/*sets port to dialog window*/
				
	/* now make the dialog box visible */
	DrawDialog (dialog);
	
	SetDialogDefaultItem( dialog, 1 );
	SetDialogCancelItem( dialog, 2 );
	SetDialogTracksCursor( dialog, true );
	
	GetDItem( dialog, 4, &itemType, &itemHandle, &itemRect);
	SetIText( itemHandle, "\p" );
	
	while ( ! dialogExit )
	{
		ModalDialog( nil, &itemHit );
		
		switch( itemHit )
		{
			case 1:
				SetPort(mainPort);					/*sets port to Traveler window*/
				GetIText( itemHandle, soString );
				MoveTo( 200, 56);
				DrawString( soString );
				dialogExit = true;
				DisposDialog( dialog );
				IntDialogs();
				break;
				
			case 2:
				dialogExit = true;
				DisposDialog( dialog );
				SetPort(mainPort);			/*sets port to Traveler window*/
				LoadScreen();
				break;
		}
	}
}

/************ PROCEDURE: writes the initials on screen Traveler************/
void	IntDialogs( void)
{
	DialogPtr	dialog;
	Boolean		dialogExit = false;
	short		itemHit, itemType;
	Handle		itemHandle;
	Rect		itemRect;
	
	/*loading the Initials dialog Box*/
	dialog = GetNewDialog( 132, nil, (WindowPtr)-1L );
	
	if ( dialog == nil )	/* In case program couldn't load resource */
	{
		SysBeep( 10 );
	}
	
	ShowWindow( dialog );
	SetPort( dialog );						/*sets port to dialog window*/
				
	/* now make the dialog box visible */
	DrawDialog (dialog);
	
	SetDialogDefaultItem( dialog, 1 );
	SetDialogCancelItem( dialog, 2 );
	SetDialogTracksCursor( dialog, true );
	
	GetDItem( dialog, 4, &itemType, &itemHandle, &itemRect);
	SetIText( itemHandle, "\p" );
	
	while ( ! dialogExit )
	{
		ModalDialog( nil, &itemHit );
		
		switch( itemHit )
		{
			case 1:
				SetPort(mainPort);					/*sets port to Traveler window*/
				GetIText( itemHandle, intString );
				MoveTo( 138, 73);
				DrawString( intString );
				dialogExit = true;
				DisposDialog( dialog );
				PrintTrav();
				break;
				
			case 2:
				dialogExit = true;
				DisposDialog( dialog );
				SetPort(mainPort);			/*sets port to Traveler window*/
				LoadScreen();
				break;
		}
	}
	
}

/************ PROCEDURE: loads the cancel printing dialog ****************/
void	CancelDialogs( void)
{
	DialogPtr	dialog;
	
	/*loading the Cancel dialog Box*/
	dialog = GetNewDialog( 134, nil, (WindowPtr)-1L );
	
	if ( dialog == nil )	/* In case program couldn't load resource */
	{
		SysBeep( 10 );
	}
	
	ShowWindow( dialog );
	SetPort( dialog );						/*sets port to dialog window*/
				
	/* now make the dialog box visible */
	DrawDialog (dialog);
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
	
	SetPort(mainPort);						/*sets current port back to window "trav"*/
}

/******************** PROCEDURE: draws info to printer Traveler ************************/
void	DrawingText( )
{
	TextSize( 12 );
	TextFont( helvetica );
	
	MoveTo( 10, 55 );
	DrawString( * GetString( 128 ) );	/*Draws the software version number*/
	
	MoveTo(100, 91);					/*Draws Model Number*/
	DrawString( word );
	DrawString( wordb );
	DrawString( wordc );
	
	MoveTo(85, 110);					/*Draws Date*/
	DrawString( s );
	
	MoveTo(200, 110);					/*Draws Shop Order*/
	DrawString( soString );
	
	MoveTo(133, 127);					/*Draws Initials*/
	DrawString( intString );
}

/*******************************   PrintTrav  *****************************************/
void	PrintTrav( void )
{
	GrafPtr		savedPort;
	TPrStatus	prStatus;
	TPPrPort	printPort;
	THPrint		hPrint;
	Boolean	ok;
	DialogPtr	dialog;
	
	GetPort( &savedPort );
	PrOpen();
	hPrint = (THPrint) NewHandle( sizeof(TPrint));
	PrintDefault(hPrint);
	if (DoPageSetUp)
		ok = PrStlDialog(hPrint);
	else
		ok = PrValidate(hPrint);
	ok = PrJobDialog(hPrint);
	if (ok)
	{
		CancelDialogs();
		printPort = PrOpenDoc(hPrint, nil, nil);
		SetPort( &printPort->gPort);
		PrOpenPage(printPort, nil);
		DrawingText();
		PrClosePage(printPort);
		PrCloseDoc(printPort);
		if ((**hPrint).prJob.bJDocLoop == bSpoolLoop && PrError() == noErr)
			PrPicFile(hPrint, nil, nil, nil, &prStatus);
	}
	else
	{
		SysBeep(5);
	}
	PrClose();
	dialog = FrontWindow();
	DisposDialog( dialog );
	SysBeep(10);
	SysBeep(10);
	SysBeep(10);
	SetPort( savedPort );
}

/****************************   PreSoftware Version  ***********************************/
void	PreSoftwareVersion( void )
{
	StringHandle	textHandle;
	
	textHandle = GetString( 128 );
	
	if ( textHandle == nil )
	{
		SysBeep( 20 );
	}
	
	if ( SoftwareVersion( textHandle ) == true )
	{
		ChangedResource( (Handle) textHandle );
		WriteResource( (Handle) textHandle );
	}
}
/****************************   Software Version  **************************************/
Boolean	SoftwareVersion( StringHandle textHandle )
{
	DialogPtr	dialog;
	Boolean		dialogExit = false;
	short		itemHit, itemType;
	Handle		OKitemHandle, itemHandle;
	Rect		itemRect;
	
	/*loading the Software Version dialog Box*/
	dialog = GetNewDialog( 133, nil, (WindowPtr)-1L );
	
	if ( dialog == nil )	/* In case program couldn't load resource */
	{
		SysBeep( 10 );
	}
	
	ShowWindow( dialog );
	SetPort( dialog );						/*sets port to dialog window*/
				
	/* now make the dialog box visible */
	DrawDialog (dialog);
	
	SetDialogDefaultItem( dialog, 1 );
	SetDialogCancelItem( dialog, 2 );
	SetDialogTracksCursor( dialog, true );
	
	GetDItem( dialog, 1, &itemType, &OKitemHandle, &itemRect);
	GetDItem( dialog, 4, &itemType, &itemHandle, &itemRect);
	
	HLock( (Handle) textHandle );
	SetIText( itemHandle, *textHandle );
	HUnlock( (Handle) textHandle);
	
	SelIText( dialog, 4, 0, 32767 );
	
	while ( ! dialogExit )
	{
		GetIText( itemHandle, verString );
		if ( verString[ 0 ] == 0 )
			HiliteControl( (ControlHandle) OKitemHandle, 255 );
		else
			HiliteControl( (ControlHandle) OKitemHandle, 0 );
			
		
		ModalDialog( nil, &itemHit );
		
		switch( itemHit )
		{
			case 1:
				/*SetPort(mainPort);*/					/*sets port to Traveler window*/
				GetIText( itemHandle, verString );
				SetHandleSize( (Handle) textHandle, (Size) (verString[0] +1) );
				HLock( (Handle) textHandle );
				GetIText( itemHandle, *textHandle );
				HUnlock( (Handle) textHandle );
				/*MoveTo( 2, 10);
				DrawString( verString );*/
				dialogExit = true;
				DisposDialog( dialog );
				return( true );
				break;
				
			case 2:
				dialogExit = true;
				DisposDialog( dialog );
				SetPort(mainPort);			/*sets port to Traveler window*/
				return( false );
				break;
		}
	}
}

/************************ PROCEDURE: Plays sound resource ******************************/
void	PlaySound( void )
{
	Handle	soundHandle;
	OSErr		err;
	
	soundHandle = GetResource( 'snd ',128 );
	
	err = SndPlay( nil, soundHandle, false );
}