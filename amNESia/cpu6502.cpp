#include "cpu6502.h"
#include "ppu.h"
#include "logger.h"

#include <fstream>

using namespace amnesia;
using namespace std;

// statics
M6502 Cpu6502::_m6502;
byte Cpu6502::PrgRom[PRG_ROM__SIZE];
byte Cpu6502::CpuRam[CPU_RAM__SIZE];


void Cpu6502::Reset()
{
	Reset6502(&Cpu6502::_m6502);
	memset(&Cpu6502::CpuRam, 0, CPU_RAM__SIZE);
}

void Cpu6502::Write(register address Addr, register byte Value)
{
	logdbg("Writing $%02hX to $%04hX", Value, Addr);

	// [0x0000, 0x2000) - CPU internal ram, 8kb mirror (2kb * 4)
	if (Addr < 0x2000) {
		CpuRam[Addr & CPU_RAM__MASK] = Value; 
	}
	// [0x2000, 0x2007) - PPU controls
	else if ( Addr > 0x1FFF && Addr < 0x4000 ) { 

		// 8 bytes mirrored * 1024
		Addr &= 0x2007;

		// PPU Control Register 1 (W)
		if( Addr == 0x2000 ) {
		}
		// PPU Control Register 2 (W)
		else if( Addr == 0x2001 ) {
		}
		// PPU Status Register (R)
		else if( Addr == 0x2002 ) {
			lognow("WARN: attempted to write $%04hX (PPU Status Register) with value $%02hX", Addr, Value);
		}
		// Sprite Memory Address (W)
		else if( Addr == 0x2003 ) {
			;
		}
		// Sprite Memory Data (RW)
		else if( Addr == 0x2004 ) {
		}	
		// Screen Scroll Offsets (W)
		else if( Addr == 0x2005 ) {
			;
		}
		// PPU Memory Address (W)
		else if( Addr == 0x2006 ) {
			;
		}
		// PPU Memory Data (RW)
		else if( Addr == 0x2007 ) {
			;
		}
	}
	// pAPU Pulse 1 Control Register.
	else if( Addr == 0x4000 ){
		;
	}
	// pAPU Pulse 1 Ramp Control Register.
	else if( Addr == 0x4001 ){
		;
	}
	// pAPU Pulse 1 Fine Tune (FT) Register.
	else if( Addr == 0x4002 ){
		;
	}
	// pAPU Pulse 1 Coarse Tune (CT) Register.
	else if( Addr == 0x4003 ){
		;
	}
	// pAPU Pulse 2 Control Register.
	else if( Addr == 0x4004 ){
		;
	}
	// pAPU Pulse 2 Ramp Control Register.
	else if( Addr == 0x4005 ){
		;
	}
	// pAPU Pulse 2 Fine Tune Register.
	else if( Addr == 0x4006 ){
		;
	}
	// pAPU Pulse 2 Coarse Tune Register.
	else if( Addr == 0x4007 ){
		;
	}
	// pAPU Triangle Control Register 1.
	else if( Addr == 0x4008 ){
		;
	}
	// pAPU Triangle Control Register 2.
	else if( Addr == 0x4009 ){
		;
	}
	// pAPU Triangle Frequency Register 1.
	else if( Addr == 0x400A ){
		;
	}
	// pAPU Triangle Frequency Register 2.
	else if( Addr == 0x400B ){
		;
	}
	// pAPU Noise Control Register 1.
	else if( Addr == 0x400C ){
		;
	}
	// pAPU Noise Frequency Register 1.
	else if( Addr == 0x400E ){
		;
	}
	// pAPU Noise Frequency Register 2.
	else if( Addr == 0x400F ){
		;
	}
	// pAPU Delta Modulation Control Register.
	else if( Addr == 0x4010 ){
		;
	}
	// pAPU Delta Modulation D/A Register.
	else if( Addr == 0x4011 ){
		;
	}
	// pAPU Delta Modulation Address Register.
	else if( Addr == 0x4012 ){
		;
	}
	// pAPU Delta Modulation Data Length Register.
	else if( Addr == 0x4013 ){
		;
	}
	// DMA Access to the Sprite Memory (W)
	else if( Addr == 0x4014 ) {
		// Writes cause a DMA transfer to occur from CPU memory at 
		// address $100 x n, where n is the value written, to SPR-RAM.
		memcpy(&Ppu::SprRam[0x0000], &(Cpu6502::CpuRam[0x100*Value]), 0x100);
	}
	// Sound Channel Switch (W)
	else if( Addr == 0x4015 ) {
		;
	}
	// Joystick1 + Strobe (RW)
	else if( Addr == 0x4016 ) {
		;
	}
	// Joystick2 + Strobe (RW)
	else if( Addr == 0x4017 ) {
		;
	}
	// don't write to PRG-ROM
	else if( Addr > 0x7FFF ) {
		lognow("WARN: attempted to write $%04hX (Cart ROM area) with value $%02hX", Addr, Value);
	}
}

byte Cpu6502::Read(register address Addr)
{	
	logdbg("Reading $%04hX", Addr);

	// [0x0000, 0x2000) - CPU internal ram, 8kb mirror (2kb * 4)
	if (Addr < 0x2000) {
		return CpuRam[Addr & CPU_RAM__MASK];
	}
	// [0x2000, 0x2007) - PPU controls
	else if ( Addr > 0x1FFF && Addr < 0x4000 ) { 

		// 8 bytes mirrored * 1024
		Addr &= 0x2007;

		// PPU Control Register 1 (W)
		if( Addr == 0x2000 ) {
			lognow("WARN: attempted to read $%04hX (PPU Control Register 1)", Addr);
		}
		// PPU Control Register 2 (W)
		else if( Addr == 0x2001 ) {
			lognow("WARN: attempted to read $%04hX (PPU Control Register 2)", Addr);
		}
		// PPU Status Register (R)
		else if( Addr == 0x2002 ) {
			;
		}
		// Sprite Memory Address (W)
		else if( Addr == 0x2003 ) {
			lognow("WARN: attempted to read $%04hX (Sprite Memory Address)", Addr);
		}
		// Sprite Memory Data (RW)
		else if( Addr == 0x2004 ) {
		}	
		// Screen Scroll Offsets (W)
		else if( Addr == 0x2005 ) {
			lognow("WARN: attempted to read $%04hX (Screen Scroll Offsets)", Addr);
		}
		// PPU Memory Address (W)
		else if( Addr == 0x2006 ) {
			lognow("WARN: attempted to read $%04hX (PPU Memory Address)", Addr);
		}
		// PPU Memory Data (RW)
		else if( Addr == 0x2007 ) {
			;
		}
	}
	// pAPU Pulse 1 Control Register.
	else if( Addr == 0x4000 ){
		lognow("WARN: attempted to read $%04hX (pAPU Pulse 1 Control Register)", Addr);
	}
	// pAPU Pulse 1 Ramp Control Register.
	else if( Addr == 0x4001 ){
		lognow("WARN: attempted to read $%04hX (pAPU Pulse 1 Ramp Control Register)", Addr);
	}
	// pAPU Pulse 1 Fine Tune (FT) Register.
	else if( Addr == 0x4002 ){
		lognow("WARN: attempted to read $%04hX (pAPU Pulse 1 Fine Tune (FT) Register)", Addr);
	}
	// pAPU Pulse 1 Coarse Tune (CT) Register.
	else if( Addr == 0x4003 ){
		lognow("WARN: attempted to read $%04hX (pAPU Pulse 1 Coarse Tune (CT) Register)", Addr);
	}
	// pAPU Pulse 2 Control Register.
	else if( Addr == 0x4004 ){
		lognow("WARN: attempted to read $%04hX (pAPU Pulse 2 Control Register)", Addr);
	}
	// pAPU Pulse 2 Ramp Control Register.
	else if( Addr == 0x4005 ){
		lognow("WARN: attempted to read $%04hX (pAPU Pulse 2 Ramp Control Register)", Addr);
	}
	// pAPU Pulse 2 Fine Tune Register.
	else if( Addr == 0x4006 ){
		lognow("WARN: attempted to read $%04hX (pAPU Pulse 2 Fine Tune Register)", Addr);
	}
	// pAPU Pulse 2 Coarse Tune Register.
	else if( Addr == 0x4007 ){
		lognow("WARN: attempted to read $%04hX (pAPU Pulse 2 Coarse Tune Register)", Addr);
	}
	// pAPU Triangle Control Register 1.
	else if( Addr == 0x4008 ){
		lognow("WARN: attempted to read $%04hX (pAPU Triangle Control Register 1)", Addr);
	}
	// pAPU Triangle Control Register 2.
	else if( Addr == 0x4009 ){
		lognow("WARN: attempted to read $%04hX (pAPU Triangle Control Register 2)", Addr);
	}
	// pAPU Triangle Frequency Register 1.
	else if( Addr == 0x400A ){
		lognow("WARN: attempted to read $%04hX (pAPU Triangle Frequency Register 1)", Addr);
	}
	// pAPU Triangle Frequency Register 2.
	else if( Addr == 0x400B ){
		lognow("WARN: attempted to read $%04hX (pAPU Triangle Frequency Register 2)", Addr);
	}
	// pAPU Noise Control Register 1.
	else if( Addr == 0x400C ){
		lognow("WARN: attempted to read $%04hX (pAPU Noise Control Register 1)", Addr);
	}
	// pAPU Noise Frequency Register 1.
	else if( Addr == 0x400E ){
		lognow("WARN: attempted to read $%04hX (pAPU Noise Frequency Register 1)", Addr);
	}
	// pAPU Noise Frequency Register 2.
	else if( Addr == 0x400F ){
		lognow("WARN: attempted to read $%04hX (pAPU Noise Frequency Register 2)", Addr);
	}
	// pAPU Delta Modulation Control Register.
	else if( Addr == 0x4010 ){
		lognow("WARN: attempted to read $%04hX (pAPU Delta Modulation Control Register)", Addr);
	}
	// pAPU Delta Modulation D/A Register.
	else if( Addr == 0x4011 ){
		lognow("WARN: attempted to read $%04hX (pAPU Delta Modulation D/A Register)", Addr);
	}
	// pAPU Delta Modulation Address Register.
	else if( Addr == 0x4012 ){
		lognow("WARN: attempted to read $%04hX (pAPU Delta Modulation Address Register)", Addr);
	}
	// pAPU Delta Modulation Data Length Register.
	else if( Addr == 0x4013 ){
		lognow("WARN: attempted to read $%04hX (pAPU Delta Modulation Data Length Register)", Addr);
	}
	// DMA Access to the Sprite Memory (W)
	if( Addr == 0x4014 ) {
		lognow("WARN: attempted to read $%04hX (DMA Access to the Sprite Memory)", Addr);
	}
	// pAPU Sound / Vertical Clock Signal Register (RW)
	else if( Addr == 0x4015 ) {
		;
	}
	// Joystick1 + Strobe (RW)
	else if( Addr == 0x4016 ) {
		;
	}
	// Joystick2 + Strobe (RW)
	else if( Addr == 0x4017 ) {
		;
	}
	// read from PRG-ROM
	else if (Addr >= 0x8000) {
		return PrgRom[Addr - 0x8000]; // might be able to do & 0x7FFF faster
	}

	return 0;
}

byte Cpu6502::ReadOpcodeOnly(register address Addr)
{
	Addr;
	return 0;
}
byte Cpu6502::HandleBadOpcode(register byte op)
{
	lognow("warning: bad opcode %d", op);
	return 0;
}
byte Cpu6502::ServiceInterrupts()
{
	return INT_NONE;
	//return INT_IRQ;
	//return INT_NMI;
	//return INT_QUIT;
}

void Cpu6502::Run()
{
	Run6502( &Cpu6502::_m6502 );
}

int Cpu6502::Run(int numCycles)
{
	return Exec6502( &Cpu6502::_m6502, numCycles );
}

//////////////////////////////////////////////////////////////////////
//
// For Marat's crap ///
//
/** Rd6502()/Wr6502/Op6502() *********************************/
/** These functions are called when access to RAM occurs. **/
/** They allow to control memory access. Op6502 is the same **/
/** as Rd6502, but used to read opcodes only, when many **/
/** checks can be skipped to make it fast. It is only **/
/** required if there is a #define FAST_RDOP. **/
/** Patch6502() **********************************************/
/** Emulation calls this function when it encounters an **/
/** unknown opcode. This can be used to patch the code to **/
/** emulate BIOS calls, such as disk and tape access. The **/
/** function should return 1 if the exception was handled, **/
/** or 0 if the opcode was truly illegal. **/
/************************************ TO BE WRITTEN BY USER **/
/** Loop6502() ***********************************************/
/** 6502 emulation calls this function periodically to **/
/** check if the system hardware requires any interrupts. **/
/** This function must return one of following values: **/
/** INT_NONE, INT_IRQ, INT_NMI, or INT_QUIT to exit the **/
/** emulation loop. **/
/************************************ TO BE WRITTEN BY USER **/

void Wr6502(register word Addr, register byte Value) {    Cpu6502::Write(Addr, Value); }
byte Rd6502(register word Addr)                      {    return Cpu6502::Read(Addr); }
byte Op6502(register word Addr)                      {    return Cpu6502::ReadOpcodeOnly(Addr); }
byte Patch6502(register byte Op, register M6502 *R)  { R; return Cpu6502::HandleBadOpcode(Op); }
byte Loop6502(register M6502 *R)                     { R; return Cpu6502::ServiceInterrupts(); }
