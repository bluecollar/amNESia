// amNESia.cpp : Defines the entry point for the application.
//
#include "stdafx.h"

#include "amNESia.h"
#include "logger.h"
#include "ppu.h"
#include "video_driver.h"

using namespace amnesia;
///////////////////////////////////////////////////////////////////

#define DELETE_SAFE(x) if (x) { delete x; x=NULL; }

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

Video *gfx=NULL;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE hInstance, HWND* hwnd_out);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

extern int g_log_debug;

void drawTile(int tilenum, int base_x, int base_y)
{
	static float color_lookup[4][3] = {
			{0.5f, 0.8f, 0.2f},
			{0.9f, 0.2f, 0.1f},
			{0.5f, 0.2f, 0.7f},
			{0.9f, 0.5f, 0.9f} };

	static const int bytes_per_tile = 16;

	int offs = tilenum * bytes_per_tile;

	// DRAW TILE
	for (int y=0; y<8; y++) 
	{
		for (int x=0; x<8; x++)
		{
			int val_r = ((Ppu::PpuRam[y+offs]>>(7-x)) & 0x01) + ((Ppu::PpuRam[y+offs+8]>>(7-x)) & 0x01);
			float rgb[3] = { color_lookup[val_r][0],
				color_lookup[val_r][1],
				color_lookup[val_r][2] };

			gfx->drawPixel(base_x + x, base_y + y, rgb[0], rgb[1], rgb[2]);
		}
	}
}


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	g_log_debug = 1;

	logdbg("Entering _tWinMain()...");

	UNREFERENCED_PARAMETER(hPrevInstance);
	
	std::wstring romArg(lpCmdLine);
	std::string romFilename(romArg.begin(), romArg.end());

	MSG msg;
	HACCEL hAccelTable;

	HWND hWnd = NULL;
	if (!InitInstance (hInstance, &hWnd))
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Load Windows HotKeys
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AMNESIA));

	// Load cart
	logdbg("Loading game cart...");
	Cart* cart = new Cart();
	cart->loadCartFromFile( romFilename );
	if (!cart->isLoaded()) {
		lognow("FATAL - failed to load cart '%s'... exiting", romFilename.c_str());
		return 1;
	}

	// if trainer is present, its 512 bytes preceed PRG-ROM
	int offset = cart->getRomHeader().hasTrainer ? 512 : 0;
	if (cart->getRomHeader().hasTrainer)
		logdbg("cart has trainer");

	// Load PRG-ROM data into CPU memory
	if( cart->getRomHeader().numRomBanks == 1 ){
		cart->readRom(offset, &Cpu6502::PrgRom[0x0000], 0x4000); // 16kb cart
		cart->readRom(offset, &Cpu6502::PrgRom[0x4000], 0x4000); // 16kb cart
		offset += 0x4000; // update offset to point at CHR-ROM
		logdbg("Loaded 16kb PRG ROM chunk into 0x8000 and 0xC000");
	}
	else{
		cart->readRom(offset, &Cpu6502::PrgRom[0x0000], 0x8000 ); // 32kb cart
		offset += 0x8000; // update offset to point at CHR-ROM
		logdbg("Loaded 32kb PRG ROM chunk into 0x8000");
	}

	// load CHR-ROM into PPU memory 
	cart->readRom(offset, &Ppu::PpuRam[0x0000], 0x2000); // hardcoded 8k TODO: unhardcode
	offset += 0x2000; // update offset to point at CHR-ROM

	logdbg("Resetting 6502...");
	Cpu6502::Reset();
	logdbg("Start pc: $%hX", Cpu6502::getM6502()->PC);

	// Main game loop
	for(;;)
	{
		/*gfx->drawPixel(10, 10, 0.9f, 0.8f, 0.7f);
		gfx->drawPixel(20,20,  0.4f, 0.5f, 0.6f);
		gfx->drawPixel(30,30, 0.5f, 0.9f, 0.9f);
		gfx->drawPixel(40,40,  0.5f, 0.8f, 0.7f);
		gfx->drawPixel(50,50,  0.9f, 0.3f, 0.7f);*/

		// Made up color table for lower four bits
		float color_lookup[4][3] = {
			{0.5f, 0.8f, 0.2f},
			{0.9f, 0.2f, 0.1f},
			{0.5f, 0.2f, 0.7f},
			{0.9f, 0.5f, 0.9f} };
		
		int bytes_per_tile = 16;
		
		// blit all the pattern table tiles in order to screen
		/*
		for (int tnum=0; tnum < 0x1000/bytes_per_tile; ++tnum)
		{
			int offs = tnum * bytes_per_tile;

			int base_x = (tnum%16)*8;
			int base_y = (tnum/16)*8;

			// DRAW TILE
			for (int y=0; y<8; y++) 
			{
				for (int x=0; x<8; x++)
				{
					int val_r = ((Ppu::PpuRam[y+offs]>>(7-x)) & 0x01) + ((Ppu::PpuRam[y+offs+8]>>(7-x)) & 0x01);
					float rgb[3] = { color_lookup[val_r][0],
						color_lookup[val_r][1],
						color_lookup[val_r][2] };
					gfx->drawPixel(base_x + x, base_y + y, rgb[0], rgb[1], rgb[2]);
					//lognow("x: %d, y: %d, VAL_R: %d r: %.1f, g: %.1f, b: %.1f", x, y, val_r, rgb[0], rgb[1], rgb[2]);
				}
			}
		}*/

		// blit all the pattern table tiles in order to screen, using function
		/*for (int tnum=0; tnum < 0x1000/bytes_per_tile; ++tnum)
		{
			int base_x = (tnum%16)*8;
			int base_y = (tnum/16)*8;
			drawTile(tnum, base_x, base_y);
		}*/


		// blit the first name table (NT0 at $2000 of size $3C0) to the screen
		int tilenum = 0;
		for (int tile_y = 0; tile_y < 30; ++tile_y)
		{
			for (int tile_x = 0; tile_x < 32; ++tile_x)
			{

				int base_x = tile_x * 8;
				int base_y = tile_y*8;

				drawTile(tilenum, base_x, base_y);
				++tilenum;
			}
		}


		//logdbg("pc: $%hX", Cpu6502::getM6502()->PC);
		//Cpu6502::Run(1);

		// Main message loop
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) {
				lognow("Received WM_QUIT, exiting...");
				break;
			}


			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				gfx->render(); // OPENGL
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	delete cart;
	delete gfx;

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
	//wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.hbrBackground	= NULL;
	//wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_AMNESIA);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex); // TODO check for zero return as failure
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, HWND* hwnd_out)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   // Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_AMNESIA, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// OPENGL  ////////
	DWORD   dwExStyle;              //window extended style
    DWORD   dwStyle;                //window style

	int		width = 800;
	int		height = 600;

	dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;	// window extended style
	dwStyle = WS_OVERLAPPEDWINDOW;					// windows style

	RECT    windowRect;
    windowRect.left =(long)0;						// set left value to 0
    windowRect.right =(long)width;					// set right value to requested width
    windowRect.top =(long)0;						// set top value to 0
    windowRect.bottom =(long)height;				// set bottom value to requested height	

	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);
	///////////////////////

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

   if (!hWnd)
   {
	   hwnd_out = NULL;
	   return FALSE;
   }

   *hwnd_out = hWnd;

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	UNREFERENCED_PARAMETER(ps);

	static HDC hDC = GetDC(hWnd); // get the device context for window

	switch (message)
	{
	case WM_CREATE:
		gfx = new Video(hDC);
		gfx->setupPixelFormat();	         		    // setup pixel format
		gfx->setHglrc(wglCreateContext(gfx->getHdc()));	// create rendering context
		wglMakeCurrent(gfx->getHdc(), gfx->getHglrc());	// make rendering context current
		return 0;

	case WM_SIZE:
		gfx->resizeWindow(HIWORD(lParam), LOWORD(lParam));
        return 0;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	/*case WM_PAINT:
		// hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;*/

	case WM_CLOSE:
		lognow("got WM_CLOSE");
		wglMakeCurrent(gfx->getHdc(), NULL);	// deselect rendering context
		wglDeleteContext(gfx->getHglrc());		// delete rendering context
		PostQuitMessage(0);			// send WM_QUIT
		return 0;

	case WM_DESTROY:
		lognow("got WM_DESTROY");
		if (gfx) {
			DELETE_SAFE(gfx);
		}
		/* wglMakeCurrent(hDC, NULL);	// deselect rendering context
		wglDeleteContext(hRC);		// delete rendering context
		PostQuitMessage(0);			// send WM_QUIT 		*/
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
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}