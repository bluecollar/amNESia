#include "amNESia.h"

#include <math.h>
#include "ppu.h" // for PPU_* defines
#include "video_driver.h"

using namespace amnesia;

#include "nes_palette.inc"  // this is our bucket-to-RGB mappings


#define _PI       (3.14159265358979323846)
#define _2PI      (6.28318530717958647692)
#define _INV_2PI  (0.15915494309189533577)

// THIS IS UGLY AND REALLY BAD.
// A "NES" CONTAINER OBJECT OR OTHER LIGHTWEIGHT "GAME MANAGER" OBJECT 
// CONSTRUCTED IN THE FUTURE WILL SOLVE THIS, BUT I WANT TO GET THIS GOING RIGHT NOW.
// TODO.
extern Ppu* ppu;
// END OF TODO


/*
NOTE:
(1) glVertex2s (shorts) is faster than glVertex2i (ints) for drawing Quads, even if you are passing
      in four byte ints and not shorts

	//glOrtho(0, _width, _height, 0, 0.f, 0.00001f);  // TODO: unhardcode this
//vs
//glOrtho2d?

[Sun Dec 18 18:06:08 2011]  * Initializing sprites to use pattern table: 0
[Sun Dec 18 18:06:08 2011]  * Initializing background to us pattern table: 1
[Sun Dec 18 18:06:08 2011]  * In unknown strobe state
[Sun Dec 18 18:06:11 2011] [four pixels] time for 10000000 its: 3.428245s
[Sun Dec 18 18:06:13 2011] [draw4PixelRectFast] time for 10000000 its: 1.537469s
[Sun Dec 18 18:06:15 2011] [drawRectFast] time for 10000000 its: 1.720045s
[Sun Dec 18 18:06:16 2011] [drawRect] time for 10000000 its: 1.740435s
[Sun Dec 18 18:06:20 2011] [four pixels] time for 10000000 its: 3.477649s
[Sun Dec 18 18:06:21 2011] [draw4PixelRectFast] time for 10000000 its: 1.503180s
[Sun Dec 18 18:06:23 2011] [drawRectFast] time for 10000000 its: 1.671834s
[Sun Dec 18 18:06:25 2011] [drawRect] time for 10000000 its: 1.702404s
[Sun Dec 18 18:06:28 2011] [four pixels] time for 10000000 its: 3.440806s
[Sun Dec 18 18:06:30 2011] [draw4PixelRectFast] time for 10000000 its: 1.556411s
[Sun Dec 18 18:06:32 2011] [drawRectFast] time for 10000000 its: 1.716913s
*/


//ChangeDisplaySettings();
//ShowCursor();
//glOrtho(0, _width, _height, 0, 0.f, 0.1f);  // TODO: unhardcode this
// MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);


/*
SetForegroundWindow(hWnd);                      // Slightly Higher Priority
SetFocus(hWnd);                             // Sets Keyboard Focus To The Window
ReSizeGLScene(width, height);                       // Set Up Our Perspective GL Screen
*/

Video::Video(HDC hdc) : _hdc(hdc)
{
	// Get client window and save off width and height
	RECT rect;
	HWND hWnd = WindowFromDC(_hdc);
	GetClientRect(hWnd, &rect);
	_width = rect.right;
	_height = rect.bottom;

	// Setup pixel format (*before* creating rendering context)
	setupPixelFormat();	       

	// Create a rendering context
	setHglrc(wglCreateContext(hdc));

	// Make this rendering context the current thread's one
	int rc = wglMakeCurrent(hdc, getHglrc());	
	ASSERT( rc );

	// Configure OpenGL
	glShadeModel(GL_FLAT);              // Flat shading instead of GL_SMOOTH
	glClearDepth(1.0f);                 // Possibly unneeded, range: [0.0, 1.0]
	//glDepthFunc(GL_NEVER);				// For real 3d, consider GL_LEQUAL	
	//glDisable(GL_DEPTH_TEST);	        // Disable depth testing		
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, _width, _height);  // Reset the viewport to screen dimensions //TODO once?
	glClearColor(0, 0, 0, 0);           // Set screen clear color
	glDrawBuffer(GL_FRONT);           // For now, directly draw to the front buffer.
	//glDrawBuffer(GL_FRONT_AND_BACK);    // draw to both for now.

	//
	_frameCounter = 0;

	// Hints for OpenGL to make better decisions. GL_NICEST, GL_FASTEST, GL_DONTCARE
	// TODO: enable and compare
	/*glHint(GL_FOG_HINT, GL_FASTEST);
	glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
	glHint(GL_POLYGON_SMOOTH_HINT);*/

	// Set up font for string drawing
	_base = glGenLists(96);
	HFONT font = CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, 
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 
		FF_DONTCARE|DEFAULT_PITCH, TEXT("Courier New"));

	//oldFont = (HFONT)SelectObject(hdc, GetStockObject(SYSTEM_FONT));
	HFONT oldFont = (HFONT)SelectObject(hdc, font); // Point hdc to our font object while returning previous font hdc pointed to
	wglUseFontBitmaps(hdc, 32, 96, _base);    // build 96 chars from offset 32 in ANSI
	SelectObject(hdc, oldFont);               // Restore previous font
	DeleteObject(font);						  // Done with that
}

Video::~Video()
{
	 glDeleteLists(_base, 96);
}

void Video::drawText(const char *fmt, ...)
{
	char text[256];
	va_list ap;

	if (!fmt)
		return;

	va_start(ap, fmt);
		vsprintf_s(text, sizeof(text)-1, fmt, ap);
	va_end(ap);
 
	glPushAttrib( GL_LIST_BIT );  // prevent glListBase from affecting any other display lists in our program (?)
	glListBase( _base - 32 );     // Tell opengl we skipped the first 32 characters
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);  // Draw the display text
	glPopAttrib();                // Pop the GL_LIST_BIT
}


void Video::renderScene(unsigned char *buffer, RECT& texture, int dx, int dy)
{

	int zoom2x=ppu->debug().getZoom2x()? 1:0; // TODO - move in game container superobject once its made
 
	glClear(GL_COLOR_BUFFER_BIT);	// Clear screen (color buffer)
	glClear(GL_DEPTH_BUFFER_BIT);	// Clear z-buffer

	// Setup
	glLoadIdentity();  // Reset the view
	glTranslatef(0.0f, 0.0f, -10000.0f);

	//
	glLineWidth(1); 

	/* //  draw "here" text in space
	glLoadIdentity();
	glTranslatef(0.5f, 0.0f,-1.0f);   
	glColor3f(0.9f, 0.4f, 0.7f);
	glRasterPos2f(-0.2f, 0.15f);
	drawText("Here");*/

	// Green? line with slope=1
	/*glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -5.0f);
	glBegin(GL_LINES);
		glColor3ub(0, 230, 50);
		glVertex3i(0, 0, 0);
		glVertex3i(1, 1, 0);
	glEnd();*/


	// Bounding lines
	

	// END 3d RENDERING /////
	glPushMatrix();                 // Save off previous matrix (TODO: but what matrix is 
		glMatrixMode(GL_MODELVIEW);     // Select modelview matrix
		glLoadIdentity();               // ... and reset it
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
			glLoadIdentity();
			gluOrtho2D(0, 1, 0, 1); // TODO: vs glOrtho ?
			//render2d now/////////////

			glRasterPos2f(0.f, 0.f);

			// Bounding lines
			int w2 = PPU_SCREEN_WIDTH<<1;
			int u2 = PPU_NTSC_SCANLINES<<1;
			drawLineFast(dx,dy-2,       dx+w2+2,dy-2,    240,240,240);
			drawLineFast(dx,dy-2,       dx,dy+u2,        240,240,240);
			drawLineFast(dx,dy+u2,      dx+w2+2,dy+u2,   240,240,240);
			drawLineFast(dx+w2+2,dy,    dx+w2+2,dy+u2,   240,240,240);

			// Draw overlay stats
			glRasterPos2f(0.0f, 0.f);
	 		drawText("glps: %.1f", g_glps);
			glRasterPos2f(0.33f, 0.f);
	 		drawText("fps(est): %.1f", g_glps/300.0);
			glRasterPos2f(0.66f, 0.f);
	 		drawText("fps(actual): %.1f", g_fps);

			// Draw screenBuffer prepopulated by emulator
			glLoadIdentity();
			gluOrtho2D(0, 800, 600, 0);
			for (int y=0; y < texture.bottom; ++y) {
				for (int x=0; x < texture.right; ++x) {
					#ifdef BE_SAFE
						unsigned int iColorBucket = buffer[(y * texture.right) + x];
						ASSERT( iColorBucket >= 0 && iColorBucket < 64 );
						byte colorBucket = (byte)iColorBucket;
					#else
						byte colorBucket = buffer[(y * texture.right) + x];
					#endif

					// Transparency is determined in Ppu::DrawScene.
					// Not drawing the nes_palette's index 0 will result
					// in gray not being drawn for games.
					////// Transparent pixel - skip
					//////if (colorBucket == 0)
					//////	continue; 

					unsigned char *rgb = nes_palette[colorBucket];
					if (zoom2x) 
					{
						// VINTAGE MODE BELOW! :D
							//drawQuad((dx+(x<<1)),(dy+(y<<1)), (dx+(x<<1))+1, (dy+(y<<1))+1, rgb[0], rgb[1], rgb[2]);									
						// A SLIGHTLY LESS BUT STILL VINTAGE MODE
							//drawRect((dx+(x<<1)),(dy+(y<<1)), 1.5, 1.5, rgb[0], rgb[1], rgb[2]);
						// NORMAL MODE scaled (2x) game for 512x480. NOTE: I *should* be adding +1, not +2, but something about edges is weird. it works, but why? TODO
							drawRectFast((dx+(x<<1)),(dy+(y<<1)), 2, 2, rgb);
					}
					else {
						int xy[2] = {dx+x, dy+y};
						drawPixelFast(xy, rgb);
					}
				} 
			}

		//end render 2d ///////
		glPopMatrix();
	glPopMatrix();
	glFlush();
	_frameCounter++;

	glMatrixMode(GL_MODELVIEW);

	// Draw to actual screen by swapping intermediate buffer with screen buffer
	//SwapBuffers(_hdc);
}

//
void Video::setupPixelFormat()
{
	static PIXELFORMATDESCRIPTOR pfd = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),          //size of structure
		1,                                      //default version (ALWAYS 1)
		PFD_DRAW_TO_WINDOW |                    //window drawing support
		PFD_SUPPORT_OPENGL |                    //opengl support
		//PFD_GENERIC_ACCELERATED |             // TODO - experiment w/ performance here
		//PFD_DEPTH_DONTCARE      |
		//PFD_DOUBLEBUFFER_DONTCARE |
		PFD_DOUBLEBUFFER |                       //double buffering support
		0,                                      // -- merely an end options marker
        PFD_TYPE_RGBA,                          //RGBA color mode (else PFD_TYPE_COLORINDEX for buckets)
        24,                                     // num color bits (not counting alpha bits)
        0, 0, 0, 0, 0, 0,                       // (IGNORED) color bits
        0,                                      // num alpha bits (no alpha buffer)
        0,                                      // (IGNORED) alpha shift bit
        0,                                      // num accumulation buffer bits
        0, 0, 0, 0,                             // (IGNORED) accumulation bits
        16,                                   // num z-buffer bits
		//4,                                      //   We choose 4 bit z-buffer size
        0,                                      // num stencil buffer bits
        0,                                      // Always 0 (aux buffers not supported)
        PFD_MAIN_PLANE,                         // (IGNORED) main drawing plane
        0,                                      // (reserved) specifies number of underlay/overlay planes
        0, 0, 0                                 // (IGNORED) layer masks ignored
	};
	int nPixelFormat = ChoosePixelFormat(_hdc, &pfd);  // Choose best matching format
	ASSERT( nPixelFormat );
    int rc = SetPixelFormat(_hdc, nPixelFormat, &pfd);          // Set the pixel format to the device context*/
	ASSERT( rc );
}

void Video::drawPixelFast(int xy[2], u8 RGB[3])
{
	glBegin(GL_POINTS);
		glColor3ubv(RGB);
		glVertex2iv(xy); 
	glEnd();
}
void Video::drawPixel(float xPos, float yPos, unsigned char R, unsigned char G, unsigned char B)
{
	glBegin(GL_POINTS);
		glColor3ub(R, G, B);
		glVertex2f(xPos, yPos); // draw a point
	glEnd();
}
void Video::drawLine(f32 x1, f32 y1, f32 x2, f32 y2, f32 R, f32 G, f32 B)
{
	glBegin(GL_LINES);
		glColor3f(R, G, B);
		glVertex2f(x1, y1);
		glVertex2f(x2, y2);
	glEnd();
}
void Video::drawLineFast(int x1, int y1, int x2, int y2, u8 R, u8 G, u8 B)
{
	glBegin(GL_LINES);
		glColor3ub(R, G, B);
		glVertex2i(x1, y1);
		glVertex2i(x2, y2);
	glEnd();
}

void Video::drawRect(float x1, float y1, float w, float h, u8 R, u8 G, u8 B)
{
	float x2 = x1+w;
	float y2 = y1+h;
	glBegin(GL_QUADS); 
		glColor3ub(R, G, B);
		glVertex2f(x1, y1); 
		glVertex2f(x2, y1); 
		glVertex2f(x2, y2); 
		glVertex2f(x1, y2); 
	glEnd();
}



void Video::drawRectFast(register int x1, register int y1, int w, int h, u8 RGB[3])
{
	register int x2 = (GLshort)(x1+w);
	register int y2 = (GLshort)(y1+h);
	glBegin(GL_QUADS); 
		glColor3ubv(RGB);
		glVertex2s((GLshort)x1, (GLshort)y1); 
		glVertex2s((GLshort)x2, (GLshort)y1); 
		glVertex2s((GLshort)x2, (GLshort)y2); 
		glVertex2s((GLshort)x1, (GLshort)y2); 
	glEnd();
}


void Video::drawQuad(int x1, int y1, int x2, int y2, u8 R, u8 G, u8 B)
{
	glBegin(GL_QUADS); 
		glColor3ub(R, G, B);
		glVertex2i(x1, y1); 
		glVertex2i(x2, y1); 
		glVertex2i(x2, y2); 
		glVertex2i(x1, y2); 
	glEnd();
}

void Video::resizeWindow(int width, int height)
{
	// This is necessary - minimizing the window causes a divide by zero from height
	_width  = (width<1)  ? 1:width;
	_height = (height<1) ? 1:height;
    
    glViewport(0, 0, _width, _height);  // Reset the viewport to new screen dimensions dimensions
    glMatrixMode(GL_PROJECTION);        // Select projection matrix
    glLoadIdentity();                   // ... and reset it

	// Calculate the aspect ratio of the window
	gluPerspective(45.0f, (GLfloat)(_width/_height), 0.1f, 100000.0f);
	
	glMatrixMode(GL_MODELVIEW);			// Select the modelview matrix
	glLoadIdentity();					// ... and reset it

	//glTranslatef(0.375, 0.375, 0); // "displacement trick for exact pixelation"
}


/*
	HDC hdc = _hdc;
	HFONT prevFont = (HFONT)SelectObject(hdc, GetStockObject(SYSTEM_FONT));
	wglUseFontBitmaps(hdc, 0, 255, 1000); // glyphs 0-254, display list numbering arbitrarily started at 1000
	glListBase(1000); // indicate start of display lists
	glCallLists(24, GL_UNSIGNED_BYTE, "Hello Windows OpenGL World");

	//SwapBuffers(hdc);
	return;

	*/


	/*
	glLoadIdentity();  // Reset the view
	glTranslatef(-1.5f, 0.0f, -6.0f);
	glBegin(GL_TRIANGLES);                      // Drawing Using Triangles
		glVertex3f( 0.0f, 1.0f, 0.0f);              // Top
		glVertex3f(-1.0f,-1.0f, 0.0f);              // Bottom Left
	    glVertex3f( 1.0f,-1.0f, 0.0f);              // Bottom Right
	glEnd();
	*/
	
	// Awesome animation on perspective
	/*double t2 = g_globalClock.getSecondsElapsed();
	int end = 15 + 14*sin(t2 * 4 * _INV_2PI);
	for (int i=0; i<end; i++) 
	{
		double t = g_globalClock.getSecondsElapsed();
		double u =  -((500.0 * sin(t * 15f * _2PI) + 2550.0));

		
		int w2 = PPU_SCREEN_WIDTH<<1;
		int u2 = PPU_NTSC_SCANLINES<<1;
		
		double g = -200*sin(t * 0.75f * _2PI);
		double h = 50*sin(t * 0.25f * _2PI);

		glLoadIdentity();  // Reset the view
		glTranslatef(0.0f, 0.0f, (float)(i*u));
		/*drawLineFast(g+dx, g+dy-2, dx+w2+2, dy-2,   h*90, h*110, h*210);
		drawLineFast(g+dx, g+dy-2,       dx,dy+u2,  h*95, h*115, h*210);
		drawLineFast(dx,dy+u2,      dx+w2+2,dy+u2,  h*95, h*115, h*195);
		drawLineFast(dx+w2+2,dy,    dx+w2+2,dy+u2,  h*90, h*110, h*195);*/
		/*drawLineFast(g+dx, g+dy-2, dx+w2+2, dy-2,   90+h*1.4, 90+h, h/2+230);
		drawLineFast(g+dx, g+dy-2,       dx,dy+u2,  95+1.5*h, 95+h, h/2+230);
		drawLineFast(dx,dy+u2,      dx+w2+2,dy+u2,  95+1.6*h, 95+h, h/2+215);
    	drawLineFast(dx+w2+2,dy,    dx+w2+2,dy+u2,  95+h,     95+h, h/2+215);
		drawLineFast(g+dx, g+dy-2,     g+dx+w2+2, g+dy-2,  90+h*1.4, 90+h, h/2+230);
		drawLineFast(g+dx, g+dy-2,     g+dx,      g+dy+u2, 95+1.5*h, 95+h, h/2+230);
		drawLineFast(g+dx, g+dy+u2,    g+dx+w2+2, g+dy+u2, 95+1.6*h, 95+h, h/2+215);
		drawLineFast(g+dx+w2+2, g+dy,  g+dx+w2+2, g+dy+u2, 95+h,     95+h, h/2+215);*/

	// Experiments
		/*drawLineFast(g+dx, g+dy-2, dx+w2+2, dy-2,   h*90, h*110, h*210);
		drawLineFast(g+dx, g+dy-2,       dx, dy+u2,  h*95, h*115, h*210);
		drawLineFast(dx,   dy+u2,    dx+w2+2, dy+u2,  h*95, h*115, h*195);
		drawLineFast(dx+w2+2, dy,    dx+w2+2, dy+u2,  h*90, h*110, h*195);
	drawLineFast(g+dx, g+dy-2,     g+dx+w2+2, g+dy-2,  h*90, h*90, h*230);
		drawLineFast(g+dx, g+dy-2,     g+dx,      g+dy+u2, h*95, h*95, h*230);
		drawLineFast(g+dx, g+dy+u2,    g+dx+w2+2, g+dy+u2, h*95, h*95, h*215);
		drawLineFast(g+dx+w2+2, g+dy,  g+dx+w2+2, g+dy+u2, h*90, h*90, h*215);
	drawLineFast(g+dx, g+dy-2,     g+dx+w2+2, g+dy-2,  abs(h*90), abs(h*90), abs(h*230));
		drawLineFast(g+dx, g+dy-2,     g+dx,      g+dy+u2, abs(h*95), abs(h*95), abs(h*230));
		drawLineFast(g+dx, g+dy+u2,    g+dx+w2+2, g+dy+u2, abs(h*95), abs(h*95), abs(h*215));
		drawLineFast(g+dx+w2+2, g+dy,  g+dx+w2+2, g+dy+u2, abs(h*90), abs(h*90), abs(h*215));
	drawLineFast(g+dx, g+dy-2,        g+dx+w2+2, g+dy-2,  90+h*1.4, 90+h, h/2+230);
		drawLineFast(g+dx, g+dy-2,     g+dx,      g+dy+u2, 95+1.5*h, 95+h, h/2+230);
		drawLineFast(g+dx, g+dy+u2,    g+dx+w2+2, g+dy+u2, 95+1.6*h, 95+h, h/2+215);
		drawLineFast(g+dx+w2+2, g+dy,  g+dx+w2+2, g+dy+u2, 95+h,     95+h, h/2+215);*/
