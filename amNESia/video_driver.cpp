#include "StdAfx.h"
#include "video_driver.h"
#include <gl/gl.h> // OPENGL
#include <gl/glu.h>

using namespace amnesia;


Video::Video(HDC hdc) : _hdc(hdc)
{
	RECT rect;
	GetClientRect(WindowFromDC(_hdc), &rect);
	_width = rect.right;
	_height = rect.bottom;
}

Video::~Video()
{
}


void Video::setupPixelFormat()
{
        /*      Pixel format index
        */
        int nPixelFormat;

        static PIXELFORMATDESCRIPTOR pfd = {
                sizeof(PIXELFORMATDESCRIPTOR),          //size of structure
                1,                                      //default version
                PFD_DRAW_TO_WINDOW |                    //window drawing support
                PFD_SUPPORT_OPENGL |                    //opengl support
                PFD_DOUBLEBUFFER,                       //double buffering support
                PFD_TYPE_RGBA,                          //RGBA color mode
                32,                                     //32 bit color mode
                0, 0, 0, 0, 0, 0,                       //ignore color bits
                0,                                      //no alpha buffer
                0,                                      //ignore shift bit
                0,                                      //no accumulation buffer
                0, 0, 0, 0,                             //ignore accumulation bits
                16,                                     //16 bit z-buffer size
                0,                                      //no stencil buffer
                0,                                      //no aux buffer
                PFD_MAIN_PLANE,                         //main drawing plane
                0,                                      //reserved
                0, 0, 0 };                              //layer masks ignored

                /*      Choose best matching format*/
                nPixelFormat = ChoosePixelFormat(_hdc, &pfd);

                /*      Set the pixel format to the device context*/
                SetPixelFormat(_hdc, nPixelFormat, &pfd);
}

void Video::drawPixel(float xPos, float yPos, float R, float G, float B)
{
	glBegin(GL_POINTS);
	glColor3f(R, G, B);
	glVertex2f(xPos, yPos); // draw a point
	glEnd();
}

void Video::drawCube(float xPos, float yPos, float zPos)
{
	glPushMatrix();
	glTranslatef(xPos, yPos, zPos);
	glBegin(GL_POLYGON);
		// Top face
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, -1.0f);
		glVertex3f(-1.0f, 0.0f, -1.0f);
		glVertex3f(-1.0f, 0.0f, 0.0f);
		  /*      This is the front face*/
		glColor3f(1.0f, 0.0f, 1.0f);
			glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(-1.0f, 0.0f, 0.0f);
                glVertex3f(-1.0f, -1.0f, 0.0f);
                glVertex3f(0.0f, -1.0f, 0.0f);

                /*      This is the right face*/
		glColor3f(1.0f, 1.0f, 1.0f);
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(0.0f, -1.0f, 0.0f);
                glVertex3f(0.0f, -1.0f, -1.0f);
                glVertex3f(0.0f, 0.0f, -1.0f);

                /*      This is the left face*/
			glColor3f(1.0f, 0.5f, 1.0f);
                glVertex3f(-1.0f, 0.0f, 0.0f);
                glVertex3f(-1.0f, 0.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, 0.0f);

                /*      This is the bottom face*/
			glColor3f(1.0f, 1.0f, 0.5f);
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(0.0f, -1.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, 0.0f);

                /*      This is the back face*/
				glColor3f(0.5f, 1.0f, 1.0f);
                glVertex3f(0.0f, 0.0f, 0.0f);
                glVertex3f(-1.0f, 0.0f, -1.0f);
                glVertex3f(-1.0f, -1.0f, -1.0f);
                glVertex3f(0.0f, -1.0f, -1.0f);
	glEnd();
	glPopMatrix();
}


void Video::resizeWindow(int height, int width)
{
    /*      Don't want a divide by 0*/
    if (height == 0) {
            height = 1;
    }
	_width = width;
	_height = height;

    /*      Reset the viewport to new dimensions*/
    glViewport(0, 0, _width, _height);

    /*      Set current Matrix to projection*/
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity(); //reset projection matrix

    /*      Time to calculate aspect ratio of our window. */
    //gluPerspective(54.0f, (GLfloat)width/(GLfloat)height, 1.0f, 1000.0f);
	// instead of aspect ratio, this is 2d. set it flat
	glOrtho(0, _width, _height, 0, 0, 1);  // TODO: unhardcode this

    glMatrixMode(GL_MODELVIEW); //set modelview matrix

	glDisable(GL_DEPTH_TEST);				// Enable depth testing
	glLoadIdentity(); //reset modelview matrix

	glTranslatef(0.375, 0.375, 0); // "displacement trick for exact pixelation"
}

/*void Video::render()
{
	glEnable(GL_DEPTH_TEST);				// Enable depth testing
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	// Clear screen to black
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear color and depth buffers
	glLoadIdentity();						// Reset our modelview matrix

	// draw here //////
	glPushMatrix();
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, -30.0f);
		glRotatef(10.0f, 0.0f, 1.0f, 0.0f);
		glPushMatrix();
			glTranslatef(0.0f, 0.0f, 0.0f);
			glPushMatrix();
			glColor3f(1.0f, 1.0f, 1.0f);
			glTranslatef(1.0f, 2.0f, 0.0f);
			glScalef(2.0f, 2.0f, 2.0f);
			drawCube(0.0f, 0.0f, 0.0f);
		glPopMatrix();
	glPopMatrix();
	///////////////////

	glFlush();
	SwapBuffers(_hdc);					// Bring back buffer to foreground
}*/

void Video::render()
{
	SwapBuffers(_hdc);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	/*glBegin(GL_POINTS);
	glColor3f(100, 150, 200);
	glVertex2f(10, 10); // draw a point
	glEnd();

	glBegin(GL_LINES);
	glColor3f(200, 100, 150);
	glVertex2f(500, 500);
	glVertex2f(550, 550); // draw a line
	glEnd();

	SwapBuffers(_hdc);*/
}
