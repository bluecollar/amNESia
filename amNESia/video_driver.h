#pragma once

#include <gl/gl.h>   // OpenGL32 Library
#include <gl/glu.h>  // GLu32 Library
#include <stdarg.h>  // for varg list

namespace amnesia
{

class Video
{
public:
	Video(HDC hdc);
	~Video();

	typedef unsigned char  byte;
	typedef unsigned char  u8;
	typedef unsigned short u16;
	typedef unsigned int   uint;
	typedef unsigned long  u32;
	typedef   signed char  s8;
	typedef   signed short s16;
	typedef   signed int   sint;
	typedef   signed long  s32;
	typedef float          f32;
	typedef double         f64;
	typedef double         d64;


	void setHglrc(HGLRC hglrc) { _hglrc = hglrc; }
	HGLRC getHglrc()           { return _hglrc; }
	HDC getHdc()               { return _hdc; }

	void setupPixelFormat();
	void resizeWindow(int width, int height);

	void renderScene(u8 *buffer, RECT& rect, int dx=0, int dy=0);

	//
	void drawPixel(float x, float y, u8 R, u8 G, u8 B);
	void drawPixelFast(int xy[2], u8 RGB[3]);
	void drawLine(f32 x1, f32 y1, f32 x2, f32 y2, f32 R, f32 G, f32 B);
	void drawLineFast(int x1, int y1, int x2, int y2, u8 R, u8 G, u8 B);
	void drawRect(f32 x, f32 y, f32 w, f32 h, u8 R, u8 G, u8 B);
	void drawRectFast(int x1, int y1, int w, int h, u8 RGB[3]);
	void drawQuad(int x1, int y1, int x2, int y2, u8 R, u8 G, u8 B);


	void drawText(const char *fmt, ...);
	//void drawCube(float xPos, float yPos, float zPos);

	inline long getFrameCount()   { return _frameCounter; }
	inline void resetFrameCount() { _frameCounter = 0; }

private:
	HDC _hdc;      // hardware device context
	HGLRC _hglrc;  // rendering context

	int _width;
	int _height;

	long _frameCounter;

	GLuint _base;  // hold the number of the first display list (for font/string drawing)

	static u8 nes_palette[64][3];
};

}// end namespace amnesia