#pragma once

namespace amnesia
{

class Video
{
public:
	Video(HDC hdc);
	~Video();

	//
	void setupPixelFormat();
	void setHglrc(HGLRC hglrc) { _hglrc = hglrc; }
	HGLRC getHglrc() { return _hglrc; }
	HDC getHdc() { return _hdc; }

	// 
	void resizeWindow(int height, int width);
	void render();

	//
	//void drawPixel(float xPos, float yPos, unsigned int RGBa);
	void drawPixel(float xPos, float yPos, float R, float G, float B);
	void drawCube(float xPos, float yPos, float zPos);

private:
	HDC _hdc;      // hardware device context
	HGLRC _hglrc;  // rendering context

	int _width;
	int _height;
};

}// end namespace amnesia