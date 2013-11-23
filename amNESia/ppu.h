#pragma once

#include "amNESia.h"
#include "video_driver.h"

namespace amnesia 
{
#define PPU_ADDRESS_MASK  (0x3FFF)	

#define PPU_SPR_RAM__SIZE       (0x0100)  // 256 bytes
#define PPU_PATTERN_TBL__SIZE   (0x1000)  // 4kb
#define PPU_NAME_TBL__SIZE      (0x0400)  // 1kb
#define PPU_ATTR_TBL__OFFSET	  (0x03C0)  // offset of attribute table in name table
#define PPU_PALETTE_RAM__SIZE   (0x0020)
#define PPU_SPR_PALETTE__OFFSET (0x0010)

#define HBLANK_CYCLES                (85)
#define PPU_LINES                   (262)
#define PPU_NTSC_SCANLINES_VISIBLE  (224)
#define PPU_NTSC_SCANLINES          (240)
#define PPU_SCREEN_WIDTH            (256)
#define PPU_NTSC_PIXEL_COUNT        ((PPU_SCREEN_WIDTH) * (PPU_NTSC_SCANLINES))

// http://wiki.nesdev.com/w/index.php/PPU_frame_timing
#define PPU_FRAME_CLOCK_CYCLES_BG_OFF      (HBLANK_CYCLES * PPU_LINES)     //341 * 262
#define PPU_FRAME_CLOCK_CYCLES_BG_ON_EVEN  (HBLANK_CYCLES * PPU_LINES)     //
#define PPU_FRAME_CLOCK_CYCLES_BG_ON_ODD   (HBLANK_CYCLES * PPU_LINES - 1) //

// Registers the 6502 uses to talk to the ppu
#define PPUCTRL    (0x2000) // (Ppu::ppuRegs[0x00])  // $2000  > write
#define PPUMASK    (0x2001) // (Ppu::ppuRegs[0x01])  // $2001  > write
#define PPUSTATUS  (0x2002) // (Ppu::ppuRegs[0x02])  // $2002  < read
#define OAMADDR    (0x2003) // (Ppu::ppuRegs[0x03])  // $2003  > write
#define OAMDATA    (0x2004) // (Ppu::ppuRegs[0x04])  // $2004  <> read/write
#define PPUSCROLL  (0x2005) // (Ppu::ppuRegs[0x05])  // $2005  >> write twice
#define PPUADDR    (0x2006) // (Ppu::ppuRegs[0x06])  // $2006  >> write twice
#define PPUDATA    (0x2007) // (Ppu::ppuRegs[0x07])  // $2007  <> read/write



// 341 ppu cc's = 1 scanline = 341/3 cpu cc's

// one frame consists of 262 scanlines 
//     (341*262 cc's as seen in PPU_FRAME_CLOCK_CYCLES_BG_OFF)

/* " These cycle values appear to correspond to numbers of scanlines. 
	Specifically, 27384 is almost 241 * 341/3 (241 scanlines), 29658 is 
	9 clocks less than 261 scanlines, and 57165 is 29781 clocks (one normal frame) 
	after 27384." */


class Ppu
{
public:

	Ppu(Video *);
	~Ppu();	

	void Run(unsigned int numCycles);
	void Reset();

	byte Read(address Addr); // TODO - use this to filter down to readram/reg
	byte ReadRam(address Addr);
	byte ReadReg(address Addr);
	void Write(address Addr, byte Value); // TODO as above
	void WriteRam(address Addr, byte Value);
	void WriteReg(address Addr, byte Value);

	byte* baseBGPatternTable;
	byte* baseSpritePatternTable;
	buffer_t patternTable0;
	buffer_t patternTable1;

	byte* pNameTable0;
	byte* pNameTable1;
	byte* pNameTable2;
	byte* pNameTable3;
	byte* baseNameTable;
	buffer_t nameTables;

	buffer_t sprRam;
	buffer_t paletteRam;

	enum nametable_mirroring {
		NS_UNKNOWN,
		NS_SINGLE,
		NS_HORIZONTAL,
		NS_VERTICAL,
		NS_FOURWAY
	} _mirroring;
	void setMirroring(nametable_mirroring t);

	void startVBlank(); 
	void endVBlank(); 
	int generateNmiOnVBlank();

	inline void incrementCyclesElapsed( unsigned int n ) { _numCycles += n; }
	inline unsigned long getCyclesElapsed()              { return _numCycles; }

	void drawBackgroundTile(int tilenum, int base_x, int base_y);
	void drawSpriteTileSafe(int tilenum, int base_x, int base_y, byte hicolor, int vflip=0, int hflip=0);
	void drawSpriteTileRealFast(int tilenum, int base_x, int base_y, byte hicolor, int vflip=0, int hflip=0);
	void drawSpriteTileExperimental(int tilenum, int base_x, int base_y, byte hicolor, int vflip=0, int hflip=0);
	void drawSprites(int priority);

	void DrawScene();
	void drawScanline(int scanlineNumber);
	void drawPatternTables();
	void drawSpriteRam();
	void drawNameTables();
	address getAddr2003() { return _addr2003; };

	inline bool showBackground() { return (_showBackground && !_su.disabledBG()); }
	inline bool showSprites()    { return (_showSprites    && !_su.disabledSprites()); }

	enum states_t { UNKNOWN, RENDERING, POSTRENDER, VBLANK, NMI, PRERENDER };
	states_t state; // TODO make private
	int doubleWideSprites;
	int isOddFrame;

private:
	class Superuser;

public:
	Superuser& debug()  { return _su; }

private:
	unsigned long _numCycles;

	int _addrLatch; // to toggle upper and lower 8-bit of 16-bit $2005/$2006 "magic"
	short _x2005;
	short _y2005;

	address _addr2003;

	address _staged2006; // TODO: What is this for?
	address _addr2006;
	byte _addr2006inc;

	// $2002
	byte _inVBlank;
	byte _sprite0hit;
	byte _excessSprites;
	byte _ignoreVRAMwrites;

	byte _last2002;

	byte _generateNMI; // NOTE: test performance against int
	byte _showBackground8pixels;
	byte _showSprites8pixels;
	byte _showBackground;
	byte _showSprites;
	
	byte _lastRead; // buffered output required for $2007

	vector<byte> _screenBuffer;//[PPU_NTSC_SCANLINES * PPU_SCREEN_WIDTH];

	//
	Video*    _video;
	
	//
	class Superuser // Superuser privs for debugging
	{
	public:
		Superuser() : _disabledBG(false), _disabledSprites(false), _zoom2x(true),
					 _showNameTables(false), _showPatternTables(false), _disableVBlankReset(false),
					 _disabledScanline(false)
		{}
		~Superuser() {}

		// This turns the layers off for the CLIENT only, does not affect the internal emulator state
		inline void toggleBackgroundLayer()  { _disabledBG = !_disabledBG; }
		void toggleSpriteLayer()             { _disabledSprites = !_disabledSprites; }
		void setZoom2x(bool zoom2x)			 { _zoom2x = zoom2x; }
		void toggleNameTables()				 { _showNameTables = !_showNameTables; if( _showNameTables ) _showPatternTables = false; }
		void togglePatternTables()			 { _showPatternTables = !_showPatternTables; if( _showPatternTables ) _showNameTables = false;}
		void toggleVBlankReset()			 { _disableVBlankReset = !_disableVBlankReset; }
		void toggleScanline()				 { _disabledScanline = !_disabledScanline; }

		// These tell you if the internal emulator state thinks the layers are off at the ppu level
		inline bool disabledBG()		{ return _disabledBG; }
		inline bool disabledSprites()	{ return _disabledSprites; }
		inline bool getZoom2x()			{ return _zoom2x; }
		inline bool showNameTables()	{ return _showNameTables; }
		inline bool showPatternTables()  { return _showPatternTables; }
		inline bool disabledVBlankReset() { return _disableVBlankReset; }
		inline bool disabledScanline()	{ return _disabledScanline; }

	private:
		bool _disabledBG;
		bool _disabledSprites;
		bool _zoom2x;
		bool _showNameTables;
		bool _showPatternTables;
		bool _disableVBlankReset;
		bool _disabledScanline;
	};
	Superuser _su;
};

} // end namespace amnesia