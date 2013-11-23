// amNESia.cpp : Defines the entry point for the application.
//
#include "amNESia.h"
#include "cart.h"
#include "cpu6502.h"
#include "HID.h"
#include "ppu.h"
#include "video_driver.h"


using namespace amnesia;


namespace amnesia 
{
	HID* g_hid = NULL;

	FineTimer g_globalClock;
	double g_fps  = 0;
	double g_glps = 0;

}


// Dangling :(
Video *g_gfx = NULL;
Ppu   *ppu = NULL;



///////////////////////////////////////////////////////////////////

// Global Variables:
#define MAX_LOADSTRING 100
HINSTANCE g_hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE hInstance, HWND* hwnd_out);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	// Locals 
	double secs;
	MSG msg;
	HWND hWnd = NULL;

	// Start the master high-resolution clock
	g_globalClock.start();

	// Set the application debugging level
	g_logger.setLevel(Logger::L_ERROR);
	g_logger.logDebug("Entering _tWinMain()... |%d|");

	// Get command line arguments (currently the .nes rom to load)
	std::wstring romArg(lpCmdLine);
	std::string romFilename(romArg.begin(), romArg.end());

	// Create instance of this program
	if (!InitInstance(hInstance, &hWnd))
		return FALSE;

	// Instantiate our HID (human interface device) object
	g_hid = new HID();

	// Create the Ppu
	ppu = new Ppu(g_gfx); 

	// Load cart
	g_logger.logDebug("Loading game cart '%s'...", romFilename.c_str());
	Cart* cart = new Cart();
	cart->loadCartFromFile( romFilename );
	if (!cart->isLoaded()) {
		g_logger.log("FATAL - failed to load cart '%s'... exiting", romFilename.c_str());
		return 1;
	}

	// Set up mirroring of ppu nametables based on header information
	if (cart->_iNesHeader.verticalMirroring) {
		ppu->setMirroring(Ppu::NS_VERTICAL);
	}
	else {
		ppu->setMirroring(Ppu::NS_HORIZONTAL);
	}
	//ppu->setMirroring(NS_SINGLE);
	//ppu->setMirroring(NS_FOURWAY);

	// Trainers are not supported - their 512 bytes preceed PRG-ROM
	int offset = cart->getRomHeader().hasTrainer ? 512 : 0;
	if (cart->getRomHeader().hasTrainer) {
		g_logger.log("cart has trainer.. exiting until trainer code is implemented");
		exit(1);
	}

	// Load PRG-ROM data into CPU memory
	if( cart->getRomHeader().numRomBanks == 1 ){
		cart->readRom(offset, &Cpu6502::PrgRom[0x0000], 0x4000); // 16kb cart
		cart->readRom(offset, &Cpu6502::PrgRom[0x4000], 0x4000); // 16kb cart
		offset += 0x4000; // update offset to point at CHR-ROM
		g_logger.logDebug("Loaded 16kb PRG ROM chunk into 0x8000 and 0xC000");
	}
	else{
		cart->readRom(offset, &Cpu6502::PrgRom[0x0000], 0x8000 ); // 32kb cart
		offset += 0x8000; // update offset to point at CHR-ROM
		g_logger.logDebug("Loaded 32kb PRG ROM chunk into 0x8000");
	}

	// Load CHR-ROM into PPU memory 
	const int chrBankSize = 0x1000;
	cart->readRom(offset, &ppu->patternTable0[0], chrBankSize); // hardcoded 8k TODO: unhardcode
	offset += chrBankSize;
	cart->readRom(offset, &ppu->patternTable1[0], chrBankSize); // hardcoded 8k TODO: unhardcode
	offset += chrBankSize;		

	// Attach the ppu to the cpu
	Cpu6502::AttachPpu(ppu);

	// Reset the cpu 
	g_logger.log("Resetting 6502...\n");
	Cpu6502::Reset();

	// Reveal screen
	ShowWindow(hWnd, nCmdShow);
	SetActiveWindow(hWnd);
	SetFocus(hWnd);
	g_logger.log(" *** Starting main game loop\n");

	// Main game loop
	FineTimer glpsTimer;
	glpsTimer.start();
	for(int gameLoopCycles = 0; ; gameLoopCycles++)
	{
		// Run NES cpu for 100 cpu cycles
		int prevCyclesElapsed = Cpu6502::getCyclesElapsed();
		Cpu6502::Run(100);

		// Run NES ppu for 3x as many cycles as the cpu ran. This owns display painting.
		int cpuCyclesExecuted = Cpu6502::getCyclesElapsed() - prevCyclesElapsed; 
		ppu->Run(PPU_CYCLES_PER_CPU_CYCLE * cpuCyclesExecuted); // TODO: implement against all three potential phase offsets...

		// Process Windows messages (including user input) 
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{	
			// Entertain any request to quit the program
			if (msg.message == WM_QUIT) {
				g_logger.log("Received WM_QUIT, exiting...");
				break;
			}

			// Process user input (both for program control and the NES controller)
			if (msg.message == WM_INPUT) {
				g_hid->processRawInput(msg.lParam);
			}
			else 
			{
				// Pass any unprocessed messages along
				TranslateMessage(&msg);             
				DispatchMessage(&msg);
			}
		}

		// High resolution timing
		if ((secs = glpsTimer.getSecondsElapsed()) >= 1.0) 
		{
			// Determine frames per second
			g_fps = g_gfx->getFrameCount() / secs;
			g_gfx->resetFrameCount();

			// Determine game loops per second
			g_glps = gameLoopCycles / secs;
			gameLoopCycles = 0;

			// Restart the timer
			glpsTimer.start();
		}
	}

	// Clean up
	delete cart; 
	delete g_hid;
	delete g_gfx;

	
	UNREFERENCED_PARAMETER(hPrevInstance);  // To make the compiler be quiet
	return (int) msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AMNESIA));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_AMNESIA);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex); // TODO check for zero return as failure
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//   PURPOSE: Saves instance handle and creates main window
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, HWND* hwnd_out)
{
	HWND hWnd;

	// Store instance handle in our global variable
	g_hInst = hInstance; 

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_AMNESIA, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	int		width  = 800;
	int		height = 600;

    DWORD   dwStyle   = WS_OVERLAPPEDWINDOW;					// windows style
	DWORD   dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;	// window extended style

	RECT    windowRect;
    windowRect.left =(long)0;						// set left value to 0
    windowRect.right =(long)width;					// set right value to requested width
    windowRect.top =(long)0;						// set top value to 0
    windowRect.bottom =(long)height;				// set bottom value to requested height	
	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);
	
	// This pushes out the actual WM_CREATE
	hWnd = CreateWindowEx(NULL, szWindowClass,  //class name
                      szTitle,       //app name
                      dwStyle |
                      WS_CLIPCHILDREN |
                      WS_CLIPSIBLINGS,
                      0, 0,                         //x and y coords
                      windowRect.right - windowRect.left,
                      windowRect.bottom - windowRect.top,//width, height
                      NULL,                 //handle to parent
                      NULL,                 //handle to menu
                      hInstance,    //application instance
                      NULL);                //no xtra params

   ASSERT( hWnd );
   /*if (!hWnd){
	   hwnd_out = NULL;
	   return FALSE;
   }*/
   *hwnd_out = hWnd;

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//  PURPOSE:  Processes messages for the main window.
//  WM_COMMAND	- process the application menu
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	UNREFERENCED_PARAMETER(ps);

	static HDC hDC = GetDC(hWnd); // get the device context for window

	switch (message)
	{
	//case WM_NCCREATE:
	case WM_CREATE:
		g_gfx = new Video(hDC);
		break;
	//case WM_MOVE:
	case WM_SIZE:
		//gfx->resizeWindow(HIWORD(lParam), LOWORD(lParam)); // TODO make sure we didnt reverse these
		g_gfx->resizeWindow(LOWORD(lParam), HIWORD(lParam));
        break;

	// case WM_SETFOCUS - parameter fFocus == FALSE when window loses keyboard focus, first thing that happens.
	// case WM_KILLFOCUS
	// case WM_SETSELECTION - indicates the window should remove the highlight from the current selection
	// case WM_ACTIVATEAPP - 
	// case WM_NCACTIVATE
	// case WM_SHOWWINDOW
	case WM_ACTIVATE: // fFocus == FALSE, window is no longer active. else, window is now active
		if (HIWORD(wParam))
			g_logger.log("(Window activated)"); 
		else
			g_logger.log("(Window deactivated)"); 
		break;

	//case WM_ERASEBKGND


	//case WM_APPCOMMAND:                  // http://bytes.com/topic/c-sharp/answers/249009-value-windows-api-notification
	case WM_SYSCOMMAND:
		switch (wParam)                    // Check System Calls
		{
			case SC_SCREENSAVE:            // Screensaver Trying To Start?
			case SC_MONITORPOWER:          // Monitor Trying To Enter Powersave?
				break;                     // Prevent either from happening :D

			default:
				return DefWindowProc(hWnd, message, wParam, lParam); // QUIT can be posted through WM_SYSCOMMAND so make sure to pass unprocessed messages through..
		}
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		// Parse the menu selections:
		switch (wmId)
		{
			case IDM_ABOUT:
				DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_MOUSEWHEEL:
		g_logger.log("MOUSEWHEEL!");
		break;

	//case WM_NOTIFY
	//case WM_USERCHANGED 

	//case WM_BUTTON1DOWN 
	//case WM_LBUTTONDOWN
	//case WM_HELP:
	//case WM_MOUSEMOVE:

	//case WM_NCPAINT
	/*case WM_PAINT:
		BeginPaint()
		QueryClientWindowRect
		FillRect
		EndPaint()
		return 0
	*/

	//case WM_TIMER 

	//case WM_QUIT:
	case WM_CLOSE:
		g_logger.log("got WM_CLOSE");
		wglMakeCurrent(g_gfx->getHdc(), NULL);	// deselect rendering context (hdc is ignored)
		wglDeleteContext(g_gfx->getHglrc());	// delete rendering context
		PostQuitMessage(WM_QUIT);		        // send WM_QUIT
		//PostQuitMessage(0);		            // ?instead?  might fix the quitting app from debugger hang problem
		break; 

	case WM_DESTROY: //  WM_DESTROY	- post a quit message and return
		g_logger.log("got WM_DESTROY");
		wglMakeCurrent(hDC, NULL);             	// deselect rendering context
		wglDeleteContext(g_gfx->getHglrc());	// delete rendering context
		DELETE_SAFE(g_gfx);
		PostQuitMessage(0);		             	// send some kinda quit (?) it's not WM_QUIT...
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
 ///////////////////
// RESEARCH BIN /////////////////////////////////////

// To synthesize keystrokes, mouse motions, and button clicks:
// UINT SendInput()

// To block keyboard and mouse input events from reaching applications:
// BOOL BlockInput()

// To create a standard message box -
//    three main parts = the icon, the message, the bootons:
/*
	CHAR  szMessageString[255];
    ULONG  ulResult;

    WinLoadString(hab, (HMODULE) NULL, MY_MESSAGESTR_ID, sizeof(szMessageString), szMessageString);

    ulResult = WinMessageBox(hwndFrame,  // Parent    
        hwndFrame,                       // Owner     
        szMessageString,                 // Text      
        (PSZ) NULL,                      // caption   
        MY_MESSAGEWIN,                   // Window ID 
        MB_YESNO |
        MB_ICONQUESTION |
        MB_DEFBUTTON1);                  // Style     

     if (ulResult == MBID_YES) {}      // Do yes case.
	 else {}  // Do no case.  
*/

// To generate a standard [modal] message box (which can be app- or system- modal):
// WinMessageBox()
// MessageBox(NULL, L"What's up world!", L"My First Windows Program", MB_OK);

// To generate an enhanced [modal or modeless] message box with customized text/icons:
// WinMessageBox2()

// To generate a dialog template:
// look up DLGTEMPLATE 
// WM_INITDLG fired off when created

//GetSystemMetrics
//GetMouseMovePointsEx   (GMMP_USE_HIGH_RESOLUTION_POINTS?)

//To subclass an instance of a window, call:
// SetWindowLong()
// specify hWnd to subclass the GWL_WNDPROC flag and a pointer to the subclass procedure
// http://msdn.microsoft.com/en-us/library/ms633570(v=VS.85).aspx#designing_proc

	// Load Windows HotKeys
	//HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AMNESIA));

// Check for Windows shortcuts / hotkeys
			/*if (TranslateAccelerator(msg->hwnd, *hAccelTable, msg)) {
				return 0;}*/
			
// GetDlgItem(hwndDlg, ID_EDIT)

