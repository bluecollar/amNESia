#include "StdAfx.h"

#include <fstream>
#include <limits.h>

#include "amNESia.h"
#include "cpu6502.h"
#include "Ppu.h"

using namespace amnesia;


// TODO: optimize bg tile draw routine

// TODO: Reading $2002 within a few PPU clocks of when VBL is set causes special case behavior which will need to be implemented.

/* "On an NTSC machine, the VBL flag is cleared 6820 PPU clocks, or exactly 
    20 scanlines, after it is set." TODO- confirm against our clock delta, so that we are getting 6820

// TAKE NOTE NIGGA
/*** 
What really happens in the NES PPU is conceptually more like this:

1. During sprite evaluation for each scanline (cycles 256 to 319), the eight frontmost sprites 
on this line get drawn front (lower index) to back (higher index) into a buffer, taking only 
the first opaque pixel that matches each X coordinate. Priority does not affect ordering in 
this buffer but is saved with each pixel.

2. The background gets drawn to a separate buffer.
For each pixel in the background buffer, the corresponding sprite pixel replaces it only if 
the sprite pixel is opaque and front priority.

3. The buffers don't actually exist as full-scanline buffers inside the PPU but instead as a set of counters and shift registers. 
The above logic is implemented a pixel at a time, as PPU rendering explains. 

***/


Ppu::Ppu(Video* video) : _video(video)
{
	// Allocate and zero out the offboard screen buffer
	_screenBuffer.reserve(PPU_NTSC_PIXEL_COUNT);
	patternTable0.reserve(PPU_PATTERN_TBL__SIZE);
	patternTable1.reserve(PPU_PATTERN_TBL__SIZE);
	nameTables.reserve(PPU_NAME_TBL__SIZE * 4);
	paletteRam.reserve(PPU_PALETTE_RAM__SIZE);
	sprRam.reserve(PPU_SPR_RAM__SIZE);

	this->Reset();
}

Ppu::~Ppu()
{
	nameTables.clear();
	patternTable1.clear();
	patternTable0.clear();
	paletteRam.clear();
	sprRam.clear();
	_screenBuffer.clear();
}

void Ppu::Reset()
{
	g_logger.log("starting Ppu::Reset()"); 

	_addrLatch = 0;
	_x2005 = 0;
	_y2005 = 0;

	pNameTable0 = NULL;
	pNameTable1 = NULL;
	pNameTable2 = NULL;
	pNameTable3 = NULL;
	baseNameTable = NULL;
	baseBGPatternTable = NULL;
	baseSpritePatternTable = NULL;

	_addr2003 = 0x0000;
	_addr2006 = 0x0000;
	_generateNMI = 0;
	_showBackground8pixels = 0;
	_showSprites8pixels = 0;
	_showBackground = 0; 
	_showSprites = 0;

	doubleWideSprites = 0;
	isOddFrame = 0;

	// $2002
	_inVBlank = 0;
	_sprite0hit = 0;
	_excessSprites = 0;
	_ignoreVRAMwrites = 0;
	_last2002 = 0; 
	_staged2006 = 0;

	state = PRERENDER;

	_numCycles = 0;

	paletteRam.assign(PPU_PALETTE_RAM__SIZE,    0x00);
	sprRam.assign(PPU_SPR_RAM__SIZE,            0xFF);	
	_screenBuffer.assign(PPU_NTSC_PIXEL_COUNT,   0x00);
	patternTable0.assign(PPU_PATTERN_TBL__SIZE, 0x00);
	patternTable1.assign(PPU_PATTERN_TBL__SIZE, 0x00);
	nameTables.assign(PPU_NAME_TBL__SIZE*4,     0x00);

	g_logger.log("ending Ppu::Reset()");
}

void Ppu::WriteRam( address Addr, byte Value)
{
	// Pattern Table 0
	if (Addr < 0x1000)
		patternTable0[Addr] = Value;

	// Pattern Table 1
	else if (Addr < 0x2000)
		patternTable1[Addr - 0x1000] = Value;

	// Name/Attribute Tables
	else if (Addr < 0x3F00) 
	{
		// 0x3000-0x3EFF mirrors 0x2000-0x2EFF
		if( Addr > 0x2FFF )
			Addr -= 0x1000;
		
		if( Addr < 0x2400 )
			pNameTable0[ Addr - 0x2000 ] = Value;
		else if ( Addr < 0x2800 )
			pNameTable1[ Addr - 0x2400 ] = Value;
		else if ( Addr < 0x2C00 )
			pNameTable2[ Addr - 0x2800 ] = Value;
		else if ( Addr < 0x3000 )
			pNameTable3[ Addr - 0x2C00 ] = Value;
		else {
			g_logger.log("Addr is busted: 0x04hX", Addr);
			ASSERT( 0 );
		}
	}

	// Sprite/Image Palettes
	else if( Addr < 0x4000 ){
		// D7-D6 of bytes written to $3F00-3FFF are ignored;
		Value &= 0x3F;

		// Mirror 0x3F00-0x3F1F through 0x3FFF
		Addr &= 0x001F;

		// Mirroring occurs between the Image Palette and the Sprite Palette.
		// Any data which is written to $3F00 is mirrored to $3F10. Any data
		// written to $3F04 is mirrored to $3F14, etc. etc...
		if( Addr < 0x0010 && (Addr % 4) == 0 )
			paletteRam[ Addr + 0x0010 ] = Value;
		else if( Addr > 0x000F && (Addr % 4) == 0 )
			paletteRam[ Addr - 0x0010 ] = Value;
		//////if ((Addr & 0xEF) == 0x00)
		//////	paletteRam[(Addr ^ 0x10) & 0x1F] = Value;

		paletteRam[ Addr ] = Value;
	}

	// Very bad!!
	else {
		g_logger.log(" BUSTED ppu ram write");
		ASSERT( 0 );
	}
}

byte Ppu::ReadRam(address Addr)
{
	// 0x4000-0xFFFF mirrors 0x0000-0x3FFF (PPU_ADDRESS_MASK)
	Addr &= PPU_ADDRESS_MASK;

	// Pattern Table 0
	if (Addr < 0x1000)
		return patternTable0[Addr];

	// Pattern Table 1
	else if (Addr < 0x2000)
		return patternTable1[Addr - 0x1000];

	// Name/Attribute Tables
	else if( Addr < 0x3F00 )
	{
		// 0x3000-0x3EFF mirrors 0x2000-0x2EFF
		if( Addr > 0x2FFF )
			Addr -= 0x1000;
		
		if( Addr < 0x2400 )
			return pNameTable0[ Addr - 0x2000 ];
		else if ( Addr < 0x2800 ) 
			return pNameTable1[ Addr - 0x2400 ];
		else if ( Addr < 0x2C00 )
			return pNameTable2[ Addr - 0x2800 ];
		else if ( Addr < 0x3000 )
			return pNameTable3[ Addr - 0x2C00 ];
		else {
			g_logger.log("Addr is busted: 0x04hX", Addr);
			ASSERT( 0 );
		}
	}

	// Sprite/Image Palettes
	else if( Addr < 0x4000 ){
		return paletteRam[ Addr & 0x001F ];
	}

	// Should not get here!!!
	else {
		g_logger.log(" BUSTED ppu ram write");
		ASSERT( 0 );
	}

	return 0;
}

void Ppu::WriteReg( address Addr, byte Value )
{
	switch( Addr ) 
	{
		// $2000: PPU Control Register 1 (W)
		case PPUCTRL: 
		{
			// First 30k cycles of ppu power-up ignore writes to $2000
			if (_numCycles < 30000)
				break;

			// $2000.1 and $2000.0: base nametable ad dress
			static  byte* nameTableLookup[] = {pNameTable0, pNameTable1, pNameTable2, pNameTable3};
			if (baseNameTable != nameTableLookup[ Value & 0x03 ]) {
				g_logger.logDebug(" * baseNameTable%d assigned now", Value & 0x03);
			}

			baseNameTable = nameTableLookup[ Value & 0x03 ];

			// $2000.2: VRAM address increment (0: increment 1 (across), 1: inc 32 (down)
			/*if (    (addr2006inc != 0x20  &&  (Value & 0x04)) ||
			    	(addr2006inc != 0x01  && !(Value & 0x04)) ) {
				g_logger.log("$2006 increment is now: %d", addr2006inc);
			}*/
			_addr2006inc = (Value & 0x04) ? 0x20:0x01;

			// $2000.3: Sprite pattern table address for 8x8 sprites (ignored for 8x16)
			if (!baseSpritePatternTable) {
				g_logger.log(" * Initializing sprites to use pattern table: %d", (Value & 0x08) ? 1:0);
			}
			else if ((baseSpritePatternTable == &patternTable0[0]) && (Value & 0x08)) {
				g_logger.logDebug("NOW Using pattern table 1 for sprites");
			}
			else if ((baseSpritePatternTable == &patternTable1[0]) && !(Value & 0x08)) {
				g_logger.logDebug("NOW Using pattern table 0 for sprites");
			}
			baseSpritePatternTable = ((Value & 0x08) ? &patternTable1[0]:&patternTable0[0]);

			// $2000.4: Background pattern table address (0: 0x0000, 1: 0x1000)
			if (!baseBGPatternTable) {
				g_logger.log(" * Initializing background to us pattern table: %d", (Value & 0x10) ? 1:0);
			}
			else if ((baseBGPatternTable == &patternTable0[0]) && (Value & 0x10)) {
				g_logger.logDebug("NOW Using pattern table 1 for background");
			}
			else if ((baseBGPatternTable == &patternTable1[0]) && !(Value & 0x10)) {
				g_logger.logDebug("NOW Using pattern table 0 for background");
			}
			baseBGPatternTable = ((Value & 0x10) ? &patternTable1[0]:&patternTable0[0]);
			
			// $2000.5: Sprite size (0: 8x8, 1: 8x16)
			if (Value & (1<<5)) {
				g_logger.log("$2000.5=1 (W) ENABLE 8x16 pixel sprites");
				//ASSERT( 0 );
			}
			else {
				g_logger.logTrace("$2000.5=0 (W) RESET TO 8x8 pixel sprites");
			}
			doubleWideSprites = (Value & (1<<5));
			
			// $2000.6: master/slave ppu select bit
			// UNUSED

			// $2000.7: Generate NMI at start of VBlank (0: off, 1: on)
			if (!_generateNMI && (Value & 0x80)) {
				g_logger.logTrace("Now allowing NMI");
			}
			else if (_generateNMI && !(Value & 0x80)) {
				g_logger.logTrace("Now disallowing NMI");
			}
			_generateNMI = (Value & 0x80) ? 1:0;
			break;
		}

		// $2001: PPU Control Register 2 (W)
		case PPUMASK:
			//[$2001.0] 0: Color, 1: Grayscale
			if (Value & (1<<0)) {
				g_logger.log("$2001: Grayscale!");
				ASSERT( 0 );
			}
			//[$2001.1] 1: Show background in leftmost 8 pixels of screen; 0: Hide
			if (_showBackground8pixels && !(Value & (1<<1))) {
				g_logger.logDebug(" ~ Now SHOWING 8 left BACKGROUND pixels");
			}
			else if (!_showBackground8pixels && (Value & (1<<1))) {
				g_logger.logDebug(" ~ Now HIDING 8 left BACKGROUND pixels");
			}
			_showBackground8pixels = (Value & (1<<1)) ? 1:0;

			//[$2001.2] 1: Show sprites in leftmost 8 pixels of screen; 0: Hide
			if (_showSprites8pixels && !(Value & (1<<2))) {
				g_logger.logDebug(" ~ Now SHOWING 8 left SPRITE pixels");
			}
			else if (!_showSprites8pixels && (Value & (1<<2))) {
				g_logger.logDebug(" ~ Now HIDING 8 left SPRITE pixels");
			}
			_showSprites8pixels = (Value & (1<<2)) ? 1:0;

			//[$2001.3] 1/0: Show/hide background
			if (!_showBackground && (Value & (1<<3))) {
				g_logger.logDebug(" ~ Now SHOWING BACKGROUND");
			}
			else if (_showBackground && !(Value & (1<<3))) {
				g_logger.logDebug(" ~ Now HIDING BACKGROUND");
			}
			_showBackground = (Value & (1<<3)) ? 1:0;

			//[$2001.4] 1/0: Show/hide sprites
			if (!_showSprites && (Value & (1<<4))) {
				g_logger.logDebug(" ~ Now SHOWING SPRITES");
			}
			else if (_showSprites && !(Value & (1<<4))) {
				g_logger.logDebug(" ~ Now HIDING SPRITES");
			}
			_showSprites = (Value & (1<<4)) ? 1:0;

			//[$2001.5] Intensify reds (and darken other colors)
			if (Value & (1<<5)) {
				g_logger.log("Intensify reds: HOW??");	
				ASSERT( 0 );
			}
			//[$2001.6] Intensify greens (and darken other colors)
			if (Value & (1<<6)) {
				g_logger.log("Intensify greens: HOW??");	
				ASSERT( 0 );
			}
			//[$2001.7] Intensify blues (and darken other colors)
			if (Value & (1<<7)) {
				g_logger.log("Intensify blues: HOW??");	
				ASSERT( 0 );
			}
			break;

		// $2003: Sprite Memory Address (W)
		case OAMADDR:
			if (Value != 0)
				g_logger.log(" * OAMADDR $2003: %02hX", Value);

			_addr2003 = Value;
			break;

		// $2004 Sprite Memory Data (RW) - Write value to the SprRam address stored in $2003
		case OAMDATA:    
			sprRam[_addr2003] = Value; // writes, but not reads, increment offset
			_addr2003 = (_addr2003 + 1) & 0xFF; // wrap 2003
			break;

		// $2005: Screen Scroll Offsets (W) (write twice)
		case PPUSCROLL:

			// TODO: Should x2005 be written when flipFlop is 1, y2005 when flipFlop is 0?
			if (!_addrLatch){
				_x2005 = Value;
				if (_x2005 != 0)
					g_logger.log("Setting x: %d", _x2005);

				_addrLatch = 1;
			}
			else{
				// TODO: Should be >=, or > 240?
				_y2005 = (Value >= 240) ? 256 - Value : Value;

				if (_y2005 != 0)
					g_logger.logDebug("Setting y: %d", _y2005);

				_addrLatch = 0;
			}
			break;

		// PPU Memory Address (W)
		case PPUADDR:    // $2006  >> write twice
			if( _addrLatch ){
				_addr2006 = (_addr2006 & 0xFF00) | Value;
				_addrLatch = 0;
			}
			else{
				// Ignore top two bits since PPU's external address bus is 14-bit
				_addr2006 = (_addr2006 & 0x00FF) | ((Value & 0x3F) << 8);
				g_logger.logDebug("Set PPU i/o address to [effective] 0x%04hX", _addr2006 & PPU_ADDRESS_MASK);
				_addrLatch = 1;
			}
			break;

		// #2007: PPU Memory Data (RW)
		case PPUDATA:
			if (!_ignoreVRAMwrites) {
				unsigned short ad = _addr2006 & PPU_ADDRESS_MASK;
				WriteRam( ad, Value );
				g_logger.logDebug("$2007 (W): ppuRam[0x%04hX] = 0x%02hX", ad, Value);
				_addr2006 += _addr2006inc;
			}
			else {
				g_logger.logDebug("ignoring write to $2007 because $2002.4 is set.");
				g_logger.logTrace("ignoring write to $2007 because $2002.4 is set."); // eventually
			}
			break;
		default:
			g_logger.log("Attempted to WritePpuReg() to bad address: 0x04hX", Addr);
			ASSERT( 0 );
	}
}

byte Ppu::ReadReg( address Addr )
{
	switch ( Addr ) 
	{
		// $2002: PPU Status Register (R)
		case PPUSTATUS:
		{
			unsigned int blockflag =  ((_inVBlank) || !(_showBackground || _showSprites)) ? 0:1;
			
			if (_ignoreVRAMwrites && !blockflag) {
			  g_logger.logTrace("No longer ignoring VRAM writes");
			}
			else if (!_ignoreVRAMwrites && blockflag) {
			  g_logger.logTrace("Ignoring VRAM writes");
			}
			_ignoreVRAMwrites = (byte)blockflag;

			int data = 
			   (int)((_inVBlank        << 7) |
			         (_sprite0hit       << 6) |
			         (_excessSprites    << 5) |
					 (_ignoreVRAMwrites << 4));

			ASSERT((data >= 0) && (data <= 255));

			//if (data)  { g_logger.logDebug("reading $2002 returned: $%02hX", data); }

			_x2005 = _y2005 = 0; // reset 2005, next write should be horizontal
			_addr2006 = 0;	   // reset 2006, next write should be high byte
			_addrLatch = 0;	   // Reset $2005/$2006 flip-flop (commonly referred to as "latch")

			// http://nesdev.parodius.com/bbs/viewtopic.php?t=7647&highlight=donkey+kong
			if( !_su.disabledVBlankReset() )
				_inVBlank = 0;

			// TODO: Should we be saving last2002 into temp and returning it 
			//       instead of current last2002 value? I think this may be 
			//		 need to be buffered.
			_last2002 = (byte)data;
			return _last2002;
		}
		// Sprite Memory Data (RW)
		case OAMDATA:    // $2004  <> read/write
			// reading should not increment the address!
			g_logger.logDebug("reading $2004 returned: %02hX", sprRam[_addr2003]);
			return sprRam[_addr2003];
			break;

		// PPU Memory Data (RW)
		case PPUDATA:    // $2007  <> read/write
		{
			unsigned short ad = _addr2006 & PPU_ADDRESS_MASK;
			byte temp;

			// Reading $2007 is buffered. We return the *previous* byte read 
			// EXCEPT on palette reads
			if( ad > 0x3EFF )
			{	
				temp = ReadRam( ad ); // palette reads aren't buffered
				_lastRead = ReadRam( ad - 0x1000 ); // buffer VRAM read ($2F00-$2FFF)
				g_logger.logDebug("[$2007] read %02hX from palette[0x%04hX], returning", temp, ad);
				_addr2006 += _addr2006inc;
			}
			else
			{
				temp = _lastRead; // use previously buffered value for non-palette reads
				_lastRead = ReadRam( ad ); // buffer VRAM read
				g_logger.logDebug("[$2007] read %02hX from ppuRam[0x%04hX], returning", _lastRead, ad);
				_addr2006 += _addr2006inc;
			}
			return temp; 
		}
		break;
	default:
		g_logger.log("ERROR: attempted to read write-only (or invalid) $%04hX", Addr);
		ASSERT( 0 );
	}

	return 0;
}


// VBLANK ROUTINES
void Ppu::startVBlank()
{
	/*if (inVBlank()) {
		g_logger.log(" ***** Warning: starting vblank while in vblank???");
//		ASSERT( 0 ); 
	}*/

	_inVBlank = 1;   // Vertical blank has started

	if (generateNmiOnVBlank()) {
		Cpu6502::executeNMI();   
	}
}
void Ppu::endVBlank()
{
	/*if (!_inVBlank) {
		g_logger.log(" Warning - ending vblank when not in vblank???");
		ASSERT( 0 );
	}*/

	_inVBlank = 0;   // Vertical blank has started flag is now reset (still could be in VBlank)
}

//
int Ppu::generateNmiOnVBlank() // TODO inline
{
	return _generateNMI;
}

void Ppu::Run(unsigned const int cycles)
{
	// ONE DOT(PIXEL) PER PPU CYCLE. THREE PPU CYCLES PER CPU CYCLE
	ASSERT( cycles >= 0 );

	static const unsigned int cyclesPerScanline    = PPU_SCREEN_WIDTH + HBLANK_CYCLES;
	static const unsigned int preRenderCycles      =   1 * cyclesPerScanline;
	static const unsigned int renderingCycles      = 240 * cyclesPerScanline;
	static const unsigned int postRenderingCycles  =   1 * cyclesPerScanline;
	static const unsigned int vblankCycles         =  20 * cyclesPerScanline;

	static int stateCounter = 0; // statics are bad for this :(
	static int renderedYet = 0;
	static int pixelNumber = 0;

	int leftoverCycles = 0;

	switch(state) 
	{
		case PRERENDER:   // lines between VBlank and next picture (1) - not sure if there is any actual work to do here.
			g_logger.logTrace("PRERENDER");
			stateCounter += cycles;
			leftoverCycles = stateCounter - preRenderCycles;

			// With BG enabled, each odd PPU frame is one PPU clock shorter than normal. (gain 1 cycle)
			if( showBackground() && isOddFrame ){
				leftoverCycles++;
			}

			// Even/Odd flag so frames with background that are ODD skip one cycle in pre-render, around clock 
			// 328 on this pre-render scanline
			isOddFrame = (isOddFrame) ? 0 : 1;

			if (leftoverCycles >= 0) {
				state = RENDERING;
				stateCounter = 0;
				if (leftoverCycles > 0) {
					Run(leftoverCycles);
				}
			}
			break;

		case RENDERING:   // 240 lines (NTSC) - electron gun is actually going over screen now
			// line drawing SHOULD be happening here
			g_logger.logTrace("RENDERING");
			stateCounter += cycles;
			leftoverCycles = stateCounter - renderingCycles;
			if (leftoverCycles >= 0) {
				state = POSTRENDER;
				stateCounter = 0;
				if (leftoverCycles > 0) {
					Run(leftoverCycles);
				}
			}
			break;

		case POSTRENDER:  // blanking lines between end of picture and NMI (1) - no work to do, just burn 1 cycle
			g_logger.logTrace("POSTRENDERING");
			stateCounter += cycles;
			leftoverCycles = stateCounter - postRenderingCycles;

			if (leftoverCycles >= 0)  
			{
				DrawScene();
				
				//
				memset(&Ppu::sprRam[0], 0xFF, PPU_SPR_RAM__SIZE);
				stateCounter = 0;
				state = VBLANK;
				if (_ignoreVRAMwrites) { g_logger.logDebug("No longer ignoring VRAM writes"); }
				_ignoreVRAMwrites = 0;
				startVBlank();

				if (leftoverCycles > 0) {
					Run(leftoverCycles);
				}
			}
			break; 

		case VBLANK:      // NMI lines (20) - electron gun is reseting to upper-left now
			g_logger.logTrace("VBLANKING");
			stateCounter += cycles;
			leftoverCycles = stateCounter - vblankCycles;
			if (leftoverCycles >= 0) {
				endVBlank();
		        _sprite0hit = 0;    //  should these be moved above to where clearing sprRam happens?
			    _excessSprites = 0; // "

				if (_showBackground || _showSprites)
					_addr2006 = _staged2006;

				state = PRERENDER;
				stateCounter = 0;
				
				if (leftoverCycles > 0) {
					Run(leftoverCycles);
				}
			}
			break;

		default:
			g_logger.log("invalid ppu state");
			ASSERT( 0 );
	}

	_numCycles += cycles;
}

void Ppu::setMirroring(nametable_mirroring t) 
{ 
	_mirroring = t; 
	if (t == NS_SINGLE) {
		pNameTable0 = pNameTable1 = pNameTable2 = pNameTable3 = &nameTables[0x0000];
	}
	else if (t == NS_HORIZONTAL) {
		pNameTable0 = pNameTable1 = &nameTables[0x0000];
		pNameTable2 = pNameTable3 = &nameTables[0x0400];
	}
	else if (t == NS_VERTICAL) {
		pNameTable0 = pNameTable2 = &nameTables[0x0000];
		pNameTable1 = pNameTable3 = &nameTables[0x0400];
	}
	else if (t == NS_FOURWAY) {
		pNameTable0 = &nameTables[0x0000];
		pNameTable1 = &nameTables[0x0400];
		pNameTable2 = &nameTables[0x0800];
		pNameTable3 = &nameTables[0x0C00];
	}
	else {
		g_logger.log("Busted nametable mirroring");
		ASSERT( 0 );
	}
}

void Ppu::drawSprites( int priority )
{
	// draw these backwards from 63 to 0 to preserve priority ordering
	for (int i=63; i>=0; --i)  // 256 bytes of sprite ram, 4 bytes per sprite
	{
		//ASSERT( !doubleWideSprites ); // TODO: gotta be implemented later

		byte* sprite     = &sprRam[i<<2];
		int lowPriority  = (sprite[2] & (1<<5)) ? 1:0; // (0: in front of background; 1: behind background)

		if( lowPriority != priority )
			continue;

		int y            =  sprite[0];   // Y position of top of sprite - 1 
		int tile         =  sprite[1];  // byte[1] - Tile index number for 8x8 sprites.
		int hicolor      =  sprite[2] & 0x03; // Palette (4 to 7 [?]) of sprite
		int hFlip        = (sprite[2] & (1<<6)) ? 1:0; // Flip sprite horizontally
		int vFlip        = (sprite[2] & (1<<7)) ? 1:0; // Flip sprite vertically
		int x            =  sprite[3];
		
		#ifdef BE_SAFE
		assert (x >= 0  && x < PPU_SCREEN_WIDTH);
		assert (y >= 0  && y < 256);
		assert (tile >= 0  && tile < 256);
		assert (hicolor >= 0  &&  hicolor < 4);
		#endif

		// TODO: Drop these for now until I properly clip (and not wrap) them against x=0xFF
		if (x > 0xF8) {
			continue;
		}

		// " Hide a sprite by writing any values in $EF-$FF here. "
		if ( y >= 0xEF ) { // possibly should be 0xF0, from my math
			continue; 
		}

		g_logger.logDebug("sprite tile: %d, x/y: (%d,%d)", tile, x, y);
		
		// TODO: This is not correct, but it allows us to limp along for now.
		if( i == 0 ) _sprite0hit = 1;
		drawSpriteTileSafe(tile, x, y, (byte)hicolor, vFlip, hFlip);
	}
}

void Ppu::drawPatternTables()
{
	// Clear screen to enforce new redraw each frame.
	// Needs to use bg color for clear screen
	_screenBuffer.assign(PPU_NTSC_PIXEL_COUNT, paletteRam[0]); 

	static  byte* ptLookup[] = {&patternTable0[0], &patternTable1[0]};
	for( int k = 0; k < 2; k++ ){
		for( int i = 0; i < 16; i++){
			for( int j = 0; j < 16; j++){
				int base_x = (k&0x01)?(i<<3)+128:(i<<3);
				int base_y = (j<<3);
				int tilenum = ((j<<4) + i);

				int offs = tilenum * 16;
				
				// DRAW TILE
				for (int y=0; y<8; y++) {
					int pixelOffset = ((y+base_y) * PPU_SCREEN_WIDTH) + base_x;
					for (int x=0; x<8; x++){
						int val_r = ((ptLookup[ k ][y+offs]>>(7-x)) & 0x01) | 
								 (((ptLookup[ k ][y+offs+8]>>(7-x)) & 0x01)<<1);

						if( val_r != 0 )
							_screenBuffer.at(pixelOffset + x) = paletteRam[val_r];
					} // end for-x
				} // end for-y
			} // end for-j
		} //end for-i
	} // end for-k

	for(int i = 0; i < 32; i++ ){

		int base_x = ((i%16)<<3)+64;
		int base_y = ((i/16)<<3)+140;

		// DRAW TILE
		for (int y=0; y<8; y++) {
			int pixelOffset = ((y+base_y) * PPU_SCREEN_WIDTH) + base_x;
			for (int x=0; x<8; x++) {
				_screenBuffer.at(pixelOffset + x) = paletteRam[i];
			} // end for-x
		} // end for-y
	} // end for-i

	// Actually render to the video card here
	RECT rect;
	rect.top = 0;
	rect.left = 0;
	rect.right = PPU_SCREEN_WIDTH;
	rect.bottom = PPU_NTSC_SCANLINES;
	int destX = 20;
	int destY = 60;

	_video->renderScene((byte *)&_screenBuffer[0], rect, destX, destY);
}

//
void Ppu::drawSpriteRam()
{
	_screenBuffer.assign(PPU_NTSC_PIXEL_COUNT, paletteRam[0]); 

	// draw these backwards from 63 to 0 to preserve priority ordering
	for (int i=0; i<64; ++i) { // 256 bytes of sprite ram, 4 bytes per sprite
		byte* sprite     = &sprRam[i<<2];
		int tilenum      =  sprite[1];  // byte[1] - Tile index number for 8x8 sprites.
		int hicolor      =  sprite[2] & 0x03; // Palette (4 to 7 [?]) of sprite
		int hflip        = (sprite[2] & (1<<6)) ? 1:0; // Flip sprite horizontally
		int vflip        = (sprite[2] & (1<<7)) ? 1:0; // Flip sprite vertically

		// Sprites which are 8x16 in size function a little bit differently. A
		// 8x16 sprite which has an even-numbered Tile Index # use the Pattern
		// Table at $0000 in VRAM; odd-numbered Tile Index #s use $1000.
		// *NOTE*: Register $2000 has no effect on 8x16 sprites.
		static const int bytes_per_tile = 16;
		byte* spritePatternTable = baseSpritePatternTable;
		int offs = tilenum * bytes_per_tile;
		int base_x = (i%8)*8;
		int base_y = (i/8)*8;

		// DRAW TILE
		for (int y=0; y<8; y++) {
			for (int x=0; x<8; x++) {
				int val_r = ((spritePatternTable[y+offs]>>(7-x)) & 0x01)       +
						  (((spritePatternTable[y+offs+8]>>(7-x)) & 0x01) << 1);

				if (!val_r)
					continue;
				    		                                           
				val_r |= (hicolor << 2);
				 
				ASSERT( val_r >= 0 && val_r < 16 );

				byte xx = (byte) (hflip ? (-(x-7)):(x));
				byte yy = (byte) (vflip ? (-(y-7)):(y));

				_screenBuffer[((base_y+yy) * PPU_SCREEN_WIDTH) + (base_x+xx)] = paletteRam[val_r + PPU_SPR_PALETTE__OFFSET];
			}
		}
	}

	// Actually render to the video card here
	RECT rect;
	rect.top = 0;
	rect.left = 0;
	rect.right = PPU_SCREEN_WIDTH;
	rect.bottom = PPU_NTSC_SCANLINES;
	int destX = 20;
	int destY = 60;

	_video->renderScene((byte *)&_screenBuffer[0], rect, destX, destY);
}



void Ppu::drawNameTables()
{
	// Clear screen to enforce new redraw each frame using bg color for clear screen
	int numPixels = PPU_NTSC_PIXEL_COUNT*4;
	int transBgColor = paletteRam[0];

	_screenBuffer.reserve(numPixels);
	_screenBuffer.assign(numPixels, transBgColor); 

	static  byte* nameTableLookup[] = {pNameTable0, pNameTable1, pNameTable2, pNameTable3};
	for( int nt = 0; nt < 4; nt++ ){

		g_logger.logDebug( "Name Table %d Tile Numbers", nt );
		g_logger.logDebug( "y\\x|00|01|02|03|04|05|06|07|08|09|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|" );
		g_logger.logDebug( "---|-----------------------------------------------------------------------------------------------|" );

		for( int yy = 0; yy < 30; yy++){
			int yy5 = (yy<<5);

			g_logger.logDebug( 
				"%02d |%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|%02hX|",
				yy, nameTableLookup[nt][yy5], nameTableLookup[nt][yy5+1], nameTableLookup[nt][yy5+2], nameTableLookup[nt][yy5+3], nameTableLookup[nt][yy5+4], nameTableLookup[nt][yy5+5], nameTableLookup[nt][yy5+6], nameTableLookup[nt][yy5+7], nameTableLookup[nt][yy5+8], nameTableLookup[nt][yy5+9], nameTableLookup[nt][yy5+10], nameTableLookup[nt][yy5+11], nameTableLookup[nt][yy5+12], nameTableLookup[nt][yy5+13], nameTableLookup[nt][yy5+14], nameTableLookup[nt][yy5+15], nameTableLookup[nt][yy5+16], nameTableLookup[nt][yy5+17], nameTableLookup[nt][yy5+18], nameTableLookup[nt][yy5+19], nameTableLookup[nt][yy5+20], nameTableLookup[nt][yy5+21], nameTableLookup[nt][yy5+22], nameTableLookup[nt][yy5+23], nameTableLookup[nt][yy5+24], nameTableLookup[nt][yy5+25], nameTableLookup[nt][yy5+26], nameTableLookup[nt][yy5+27], nameTableLookup[nt][yy5+28], nameTableLookup[nt][yy5+29], nameTableLookup[nt][yy5+30], nameTableLookup[nt][yy5+31]);
			
			for( int xx = 0; xx < 32; xx++){
				int base_y = (yy<<3);
				int base_x = (xx<<3);
				int tilenum = nameTableLookup[nt][yy5+xx];
				int offs = tilenum * 16;

				byte attribute = nameTableLookup[ nt ][PPU_ATTR_TBL__OFFSET + ((base_y>>5)<<3) + (base_x>>5)];
				byte shift = ((base_x % 32) < 16 ? 0:2) + ((base_y % 32) < 16 ? 0:4);
				int highbits = ((attribute >> shift) & 0x03) << 2;

				base_x += ((nt&0x01) ? 256:0);
				base_y += ((nt&0x02) ? 240:0);
			
				// DRAW TILE
				for (int y=0; y<8; y++) {
					int pixelOffset = ((y+base_y) * PPU_SCREEN_WIDTH*2) + base_x;
					for (int x=0; x<8; x++){
						int val_r = ((baseBGPatternTable[y+offs]>>(7-x)) & 0x01) | 
								 (((baseBGPatternTable[y+offs+8]>>(7-x)) & 0x01)<<1);

						val_r = ((val_r|highbits) % 4) ? val_r|highbits :  0;

						_screenBuffer.at(pixelOffset + x) = paletteRam[val_r];

					} // end for-x
				} // end for-y				
			} // end for-j
		} // end for-i
	} //end for-k

	// Actually render to the video card here
	RECT rect;
	rect.top = 0;
	rect.left = 0;
	rect.right = PPU_SCREEN_WIDTH*2;
	rect.bottom = PPU_NTSC_SCANLINES*2;
	int destX = 5;
	int destY = 5;
	
	bool currentZoom = _su.getZoom2x();
	_su.setZoom2x( false );
	_video->renderScene((byte *)&_screenBuffer[0], rect, destX, destY);
	_su.setZoom2x( currentZoom );
}



void Ppu::DrawScene()
{
	// Clear screen to enforce new redraw each frame.
	// Needs to use bg color for clear screen
	_screenBuffer.assign(PPU_NTSC_PIXEL_COUNT, paletteRam[0]); 

	if( _su.showNameTables() ){
		drawNameTables();
		return;
	}
	else if( _su.showPatternTables() ){
		drawPatternTables();
		return;
	}

	// draw low priority sprites
	if (showSprites())
		drawSprites( 1 );

	// Pre render background and sprites to off-screen buffer
	if (showBackground()) { // TODO: this is probably flawed, not checked often enough. should be per scanline, not per frame

		if( !_su.disabledScanline() ){
			for( int scanlineNum = 0; scanlineNum < PPU_NTSC_SCANLINES; scanlineNum++ ){
				drawScanline(scanlineNum);
			}
		}
		else{
			for (int j=0; j < 30; ++j) {		
				for (int i=0; i < 32; ++i) {
					#define x         (i<<3)        // i*8
					#define y         (j<<3)        // j*8
					#define index     ((j<<5) + i)  // j*32 + i
					#define tilenum   (baseNameTable[index])
					drawBackgroundTile(tilenum, x, y);
					#undef tilenum
					#undef index
					#undef y
					#undef x
				} // end for-i
			} // end for-j
		} // end else
	} // end if showBackground

	// draw high priority sprites
	if (showSprites())	
		drawSprites( 0 );

	// Actually render to the video card here
	RECT rect;
	rect.top = 0;
	rect.left = 0;
	rect.right = PPU_SCREEN_WIDTH;
	rect.bottom = PPU_NTSC_SCANLINES;
	int destX = 20;
	int destY = 60;

	_video->renderScene((byte *)&_screenBuffer[0], rect, destX, destY);
}	


void Ppu::drawScanline(int scanlineNumber)
{
	static const int BYTES_PER_TILE = 16;

	int bgTileOffsetY = (scanlineNumber>>3)<<5;
	int ptrnTblOffsetY = scanlineNumber%8;
	int attrTableOffsetY = ((scanlineNumber>>5)<<3);
	int shiftOffsetY = ((scanlineNumber % 32) < 16 ? 0:4);

	for( int horPos = 0; horPos < PPU_SCREEN_WIDTH; horPos+=8 ){

		int x = horPos>>3;	// horizontal position divided by 8
		int bgTileIdx = bgTileOffsetY + x;
		byte bgTileNum = baseNameTable[bgTileIdx];

		// TODO: There may be a problem here. SMB is the only rom that has a problem
		//	     using this routine.
		byte attribute = baseNameTable[(horPos>>5) + attrTableOffsetY + PPU_ATTR_TBL__OFFSET];
		byte shift = ((horPos % 32) < 16 ? 0:2) + shiftOffsetY;
		int highbits = ((attribute >> shift) & 0x03) << 2;

		// Each tile is represented by 2 sets of 8 bytes (16 bytes per tile)
		// The tile number (0-255) is retrieved from the nametable and multiplied
		// by 16 to get the pattern table offset
		int ptrnTblOffset = (bgTileNum * BYTES_PER_TILE) + ptrnTblOffsetY;
		byte ptrnTblByte1 = baseBGPatternTable[ptrnTblOffset];
		byte ptrnTblByte2 = baseBGPatternTable[ptrnTblOffset+8];

		int pixelOffset = (scanlineNumber * PPU_SCREEN_WIDTH) + horPos;

		// blit one row from the background pattern table tile
		// TODO: Unroll this.
		for( int bitIdx = 0; bitIdx < 8; bitIdx++ ){

			int val_r = ((ptrnTblByte1>>bitIdx) & 0x01) | 
					    (((ptrnTblByte2>>bitIdx) & 0x01)<<1);

			val_r = ((val_r|highbits) % 4) ? val_r|highbits :  0;

			if( val_r != 0 )
				_screenBuffer.at(pixelOffset + (7-bitIdx)) = paletteRam[val_r];
		}
	}
}



// 
void Ppu::drawBackgroundTile(int tilenum, int base_x, int base_y)
{
	static const int BYTES_PER_TILE = 16;
	int offs = tilenum * BYTES_PER_TILE;

	// Pixels x = 0-31 && y = 0-31 use attribute byte 0
	// pixels x = 32-63 && y = 0-31 use attribute byte 1
	// ...
	// pixels x = 224-255 && y = 0-31 use attribute byte 7
	// Pixels x = 0-31 && y = 32-63 use attribute byte 8
	// Pixels x = 32-63 && y = 32-63 use attribute byte 9
	byte attribute = baseNameTable[PPU_ATTR_TBL__OFFSET + ((base_y>>5)<<3) + (base_x>>5)];
	byte shift = ((base_x % 32) < 16 ? 0:2) + ((base_y % 32) < 16 ? 0:4);
	int highbits = ((attribute >> shift) & 0x03) << 2;
	
	// DRAW TILE
	for (int y=0; y<8; y++) {
		int pixelOffset = ((y+base_y) * PPU_SCREEN_WIDTH) + base_x;
		for (int x=0; x<8; x++) {
			int val_r = ((baseBGPatternTable[y+offs]>>x) & 0x01) | 
				     (((baseBGPatternTable[y+offs+8]>>x) & 0x01)<<1);

			#ifdef BE_SAFE
			assert(val_r >= 0 && val_r < 4);
			#endif

			val_r = ((val_r|highbits) % 4) ? val_r|highbits :  0;

			#ifdef BE_SAFE
			assert(val_r >= 0 && val_r < 16);
			#endif

			#ifdef BE_SAFE
			if( val_r != 0) // TODO: optimization if screen buffer cleared to bg color?
							// Doing this check was wrong before because bg color wasn't drawn (screenbuffer now cleared to bg color)
				_screenBuffer.at(pixelOffset + (7-x)) = paletteRam[val_r];
			#else
			if( val_r != 0 ) // TODO: optimization if screen buffer cleared to bg color?
							 // Doing this check was wrong before because bg color wasn't drawn
				_screenBuffer[pixelOffset + x] = paletteRam[val_r];
			#endif	
		} // end for-x
	} // end for-y
}

//
void Ppu::drawSpriteTileSafe(int tilenum, int base_x, int base_y, byte hicolor, int vflip, int hflip)
{
	// Sprites which are 8x16 in size function a little bit differently. A
	// 8x16 sprite which has an even-numbered Tile Index # use the Pattern
	// Table at $0000 in VRAM; odd-numbered Tile Index #s use $1000.
	// *NOTE*: Register $2000 has no effect on 8x16 sprites.
	static const int bytes_per_tile = 16;
	byte* spritePatternTable;
	
	int tileHeight = 8;
	if(doubleWideSprites == 0){
		spritePatternTable = baseSpritePatternTable;
	}
	else if( tilenum % 2 == 0 ){
		tileHeight = 16;
		spritePatternTable = &patternTable0[0];
	}
	else{
		tileHeight = 16;
		spritePatternTable = &patternTable1[0];
	}

	int offs = tilenum * bytes_per_tile;

	// DRAW TILE
	for (int y=0; y<tileHeight; y++)  {
		for (int x=0; x<8; x++){

			int val_r = ((spritePatternTable[y+offs]>>(7-x)) & 0x01)       +
				      (((spritePatternTable[y+offs+8]>>(7-x)) & 0x01) << 1);

			if (!val_r)
				continue;
			    		                                           
			val_r |= (hicolor << 2);
			 
			ASSERT( val_r >= 0 && val_r < 16 );

			byte xx = (byte) (hflip ? (-(x-7)):(x));
			byte yy = (byte) (vflip ? (-(y-7)):(y));

#ifdef BE_SAFE
			_screenBuffer.at(((base_y+yy) * PPU_SCREEN_WIDTH) + (base_x+xx)) = paletteRam[val_r + PPU_SPR_PALETTE__OFFSET];
#else
			_screenBuffer[((base_y+yy) * PPU_SCREEN_WIDTH) + (base_x+xx)] = paletteRam[val_r + PPU_SPR_PALETTE__OFFSET];
#endif
		}
	}
}

void Ppu::drawSpriteTileRealFast(int tilenum, int base_x, int base_y, byte hicolor, int vflip, int hflip)
{
	byte* tile = baseSpritePatternTable + (tilenum << 4);  // << 4 is same as * 16 (there are 16 bytes_per_tile)
	hicolor <<= 2;  // hicolor comes in as binary xxxxxxAA but needs to be xxxxAAxx, so shift it twice to the lef

	// DRAW TILE
	if (!vflip){
		for (int y=0; y<8; y++) {
			register byte byteA = tile[y];
			register byte byteB = tile[y+8]; // *8  // TODO is tile+8 faster?

			byte* pPixel = &_screenBuffer[((base_y + y)<<8) + base_x]; // PPU_SCREEN_WIDTH is 256, so shift left 8 instead

			if (!hflip) {
				// This may be wrong, we need to know if transparent means lower two bits==0 or if the whole color ==0 
				// Below doesn't compensate for eiter...
				*pPixel++ = ((byteA >> 7) & 0x01) | ((byteB >> 6) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 6) & 0x01) | ((byteB >> 5) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 5) & 0x01) | ((byteB >> 4) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 4) & 0x01) | ((byteB >> 3) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 3) & 0x01) | ((byteB >> 2) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 2) & 0x01) | ((byteB >> 1) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 1) & 0x01) | ((byteB     ) & 0x02) | hicolor;
				*pPixel++ = ((byteA     ) & 0x01) | ((byteB << 1) & 0x02) | hicolor;

				// TODO - copy those to a string of 8 chars above
				// then do two long int assignments instead of eight byte assignments
				// this WILL be faster for nontransparent segments
				// TODO - larger mask, across 4 bytes not 1 byte for 'hicolor' optimization
			}
			else {
				*pPixel++ = ((byteA     ) & 0x01) | ((byteB << 1) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 1) & 0x01) | ((byteB     ) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 2) & 0x01) | ((byteB >> 1) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 3) & 0x01) | ((byteB >> 2) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 4) & 0x01) | ((byteB >> 3) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 5) & 0x01) | ((byteB >> 4) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 6) & 0x01) | ((byteB >> 5) & 0x02) | hicolor;
				*pPixel++ = ((byteA >> 7) & 0x01) | ((byteB >> 6) & 0x02) | hicolor;

			}
		}
	}
	else {
		// TODO: still gotta handle vflip!
	}
}

#pragma warning(disable:4706) // (assignment within conditional expression)
void Ppu::drawSpriteTileExperimental(int tilenum, int base_x, int base_y, byte hicolor, int vflip, int hflip)
{
	hflip;vflip;

	byte* tile = baseSpritePatternTable + (tilenum << 4);  // << 4 is same as * 16 (there are 16 bytes_per_tile)
	hicolor <<= 2;  // hicolor comes in as binary xxxxxxAA but needs to be xxxxAAxx, so shift it twice to the lef

	// DRAW TILE
	for (int y=0; y<8; y++)  
	{
		byte* pPixel = &_screenBuffer[((base_y + y)<<8) + base_x]; // PPU_SCREEN_WIDTH is 256, so shift left 8 instead
		register byte byteA = tile[y];
		register byte byteB = tile[y+8];

		byte color;
		if (color = ((byteA >> 7) & 0x01) | ((byteB >> 6) & 0x02) | hicolor) { *pPixel = color; } pPixel++;
		if (color = ((byteA >> 7) & 0x01) | ((byteB >> 6) & 0x02) | hicolor) { *pPixel = color; } pPixel++; 
		if (color = ((byteA >> 6) & 0x01) | ((byteB >> 5) & 0x02) | hicolor) { *pPixel = color; } pPixel++;
		if (color = ((byteA >> 5) & 0x01) | ((byteB >> 4) & 0x02) | hicolor) { *pPixel = color; } pPixel++;
		if (color = ((byteA >> 4) & 0x01) | ((byteB >> 3) & 0x02) | hicolor) { *pPixel = color; } pPixel++;
		if (color = ((byteA >> 3) & 0x01) | ((byteB >> 2) & 0x02) | hicolor) { *pPixel = color; } pPixel++;
		if (color = ((byteA >> 2) & 0x01) | ((byteB >> 1) & 0x02) | hicolor) { *pPixel = color; } pPixel++;
		if (color = ((byteA >> 1) & 0x01) | ((byteB     ) & 0x02) | hicolor) { *pPixel = color; } pPixel++;
		if (color = ((byteA     ) & 0x01) | ((byteB << 1) & 0x02) | hicolor) { *pPixel = color; } pPixel++;

	}
}

/*void Ppu::drawBackgroundTileFast(int tilenum, int base_x, int base_y)
{
	byte highbits; byte* tile = baseBGPatternTable + (tilenum << 4);  // << 4 is same as * 16 (there are 16 bytes_per_tile)
	// Pixels x = 32-63 && y = 32-63 use attribute byte 9
		int attrByteOffs = (base_y/32)*8 + (base_x/32);
		byte attribute = baseNameTable[PPU_ATTR_TBL__OFFSET + attrByteOffs];
		byte left = (base_x % 32) < 16 ? 0:2;
		byte top  = (base_y % 32) < 16 ? 0:4;

		#ifndef GO_APE_SHIT
			if (top && left)
				highbits = attribute;
			else if (top && !left)
				highbits = (attribute >> 2);
			else if (!top && left)
				highbits = (attribute >> 4);
			else if (!top && !left)
				highbits = (attribute >> 6);

			highbits &= 0x03;
			highbits <<= 2;
		#else
			highbits = (((attribute >> top) >> left) & 0x03) << 2;
		#endif

	//highbits=0;
	// DRAW TILE
	for (int y=0; y<8; y++)  
	{
		byte* pPixel = &screenBuffer[((base_y + y)<<8) + base_x]; // PPU_SCREEN_WIDTH is 256, so shift left 8 instead
		register byte byteA = tile[y];
		register byte byteB = tile[y+8]; // *8  // TODO is tile+8 faster?

		// This may be wrong, we need to know if transparent means lower two bits==0 or if the whole color ==0 
		// Below doesn't compensate for eiter...
		*pPixel++ = ((byteA >> 7) & 0x01) | ((byteB >> 6) & 0x02) | highbits;
		*pPixel++ = ((byteA >> 6) & 0x01) | ((byteB >> 5) & 0x02) | highbits;
		*pPixel++ = ((byteA >> 5) & 0x01) | ((byteB >> 4) & 0x02) | highbits;
		*pPixel++ = ((byteA >> 4) & 0x01) | ((byteB >> 3) & 0x02) | highbits;
		*pPixel++ = ((byteA >> 3) & 0x01) | ((byteB >> 2) & 0x02) | highbits;
		*pPixel++ = ((byteA >> 2) & 0x01) | ((byteB >> 1) & 0x02) | highbits;
		*pPixel++ = ((byteA >> 1) & 0x01) | ((byteB     ) & 0x02) | highbits;
		*pPixel++ = ((byteA     ) & 0x01) | ((byteB << 1) & 0x02) | highbits;

		// TODO - copy those to a string of 8 chars above
		// then do two long int assignments instead of eight byte assignments
		// this WILL be faster for nontransparent segments
	}
}*/

#pragma warning(default:4706)