#include "amNESia.h"

#include <fstream>
#include <limits.h>
#include "cpu6502.h"
#include "HID.h"


using namespace amnesia;


// TODO: http://wiki.nesdev.com/w/index.php/Emulator_Tests
// TODO: Screen grabs/dumps, or even checksums of the grab/dumps
// TODO: Movie feature to record and replay entire user input
// TODO: Automated test harness for hands free REGRESSION testing
// TODO: Create AI computer players :)


namespace amnesia {
	extern HID* g_hid;
}


// statics
M6502 Cpu6502::_m6502 = {0};
byte Cpu6502::_cpuRam[CPU_RAM__SIZE] = {0};
byte Cpu6502::PrgRom[PRG_ROM__SIZE] = {0};
byte Cpu6502::reg4016strobe = UNKNOWN_STROBE_STATE;
byte Cpu6502::reg4016readno = 0;
Ppu* Cpu6502::_ppu = NULL;

HID::button_mask_t Cpu6502::pad1_buttons      = HID::NO_BUTTON;
HID::button_mask_t Cpu6502::pad2_buttons      = HID::NO_BUTTON;
HID::button_mask_t Cpu6502::pad1_buttons_save = HID::NO_BUTTON;
HID::button_mask_t Cpu6502::pad2_buttons_save = HID::NO_BUTTON;





unsigned long Cpu6502::_numCycles = 0;

// YUM YUM GIMME SOME
void Cpu6502::Run()
{
	Run6502( &_m6502 );
	incrementCyclesElapsed((_m6502.IPeriod<<1) - _m6502.ICount); // this may end up being oddly accurate
}

int Cpu6502::Run(int numCycles)
{
	int excessCycles = Exec6502( &_m6502, numCycles );
	int totalCycles = numCycles - excessCycles; // excessCycles is either 0 or wrapped past 0 and negative and thus needs to be negated
	incrementCyclesElapsed( totalCycles );

	return totalCycles;
}


byte Cpu6502::ServiceInterrupts()
{
	//static unsigned long nmi_counter = 0;
	//unsigned long currentElapsed = getCyclesElapsed();

	// ROUGH start at timing
	//g_logger.log("cpu_cyles: %d", currentElapsed);

/*	currentElapsed -= nmi_counter;
	if (currentElapsed > (98342L/3))
	{
		_ppu->startVBlank();

		nmi_counter = getCyclesElapsed();

		if (_ppu->generateNmiOnVBlank()){
			g_logger.logDebug("[NOTE] ppu is generating NMI");
			return INT_NMI;
		}
	}*/

	return INT_QUIT; //return INT_NONE; //return INT_IRQ; */
}

// Temporary
void Cpu6502::AttachPpu(Ppu *ppu)
{
	_ppu = ppu;
}

// HARD Reset of the cpu
void Cpu6502::Reset()
{
	g_logger.log("starting Cpu6502::Reset()");

	// PLEASE DO NOT FUCK WITH THE ORDER OF THIS STUFF UNLESS YOU KNOW 
	// WHY YOU ARE DOING IT
	// :D

	// Set the last number of cpu cycles Run() should go before calling ServiceInterrupts()
	_m6502.IPeriod = 1000; // TODO: calibrate around ppu

	// Reset m6502 internally
	Reset6502(&_m6502);

	// Ignore IRQ interrupts
	_m6502.P |= I_FLAG;

	// disable decimal mode here 
	// ?

	// 
	//reg4016strobe = UNKNOWN_STROBE_STATE; 
	reg4016strobe = LATCHED;

	// Disable APU frame IRQs
	Write(0x4017, 0x40);

	// Disable DMC IRQs
	Write(0x4010, 0x00);  

	// CPU POWER UP STATE
	// http://wiki.nesdev.com/w/index.php/CPU_ALL
	_m6502.P = 0x34;
	_m6502.A = 0x00;
	_m6502.X = 0x00;
	_m6502.Y = 0x00;
	_m6502.S = 0xFD;
	memset(_cpuRam, 0xFF, CPU_RAM__SIZE);
	_cpuRam[0x0008] = 0xF7;
	_cpuRam[0x0009] = 0xEF;
	_cpuRam[0x000A] = 0xDF;
	_cpuRam[0x000F] = 0xBF;

	// PPU POWER UP STATE - http://wiki.nesdev.com/w/index.php/PPU_power_up_state#Best_practice
	Write(0x2000, 0x00);  // Disable NMI
	Write(0x2001, 0x00);  // Disable Rendering
	Write(0x2003, 0x00);
	// Clear 0x2005 and 0x2006 latch first as well as write these zeros
	// ;
	Write(0x2005, 0x00);
	Write(0x2006, 0x00);
	Write(0x2007, rand()%0xFF);
	
	// APU AND CONTROLLER POWER UP STATE
	// http://wiki.nesdev.com/w/index.php/APU_basics
	// "Before using the APU, first initialize all the
	// registers to known values that silence all channels."
	Write(0x4015, 0x0F); 

	Write(0x4000, 0x30);
	Write(0x4001, 0x08);
	Write(0x4002, 0x00);
	Write(0x4003, 0x00);
	
	Write(0x4004, 0x30);
	Write(0x4005, 0x08);
	Write(0x4006, 0x00);
	Write(0x4007, 0x00);
	
	Write(0x4008, 0x80);
	Write(0x4009, 0x00);
	Write(0x400A, 0x00);
	Write(0x400B, 0x00);

	Write(0x400C, 0x30);
	//Write(0x400D, 0x00);
	Write(0x400E, 0x00);
	Write(0x400F, 0x00);

	Write(0x4010, 0x00); // disable DMC IRQs (again?)
	Write(0x4011, 0x00);
	Write(0x4012, 0x00);
	Write(0x4013, 0x00);

	Write(0x4014, 0x00);
	Write(0x4015, 0x0F);
	Write(0x4016, 0x00); // controller
	Write(0x4017, 0x40); // Disable APU frame IRQ (again?)

	g_logger.log("ending Cpu6502::Reset()");
}


//	
void Cpu6502::executeIRQ()
{
	Int6502(&_m6502, INT_IRQ);
}
void Cpu6502::executeNMI()
{
	g_logger.logDebug("Executing NMI");
	Int6502(&_m6502, INT_NMI);
}

//
void Cpu6502::Write(register address Addr, register byte Value)
{
	ASSERT( Addr >= 0x0000 && Addr <= 0xFFFF );
	g_logger.logTrace("Writing $%02hX to $%04hX", Value, Addr);

	// [0x0000, 0x2000) - CPU internal ram, 8kb mirror (0x0800 * 4 = 0x2000)
	if ( Addr < 0x2000 ) {
		// TODO - we need vector and BE_SAFE here.
		_cpuRam[Addr & CPU_RAM__MASK] = Value; 
	}
	
	// [0x2000, 0x4000) - PPU registers
	else if ( Addr < 0x4000 ) 
	{ 
		Addr &= 0x2007;  // 8 bytes of registers mirrored through 0x3FFF
		g_logger.logDebug("[PPU/Registers] Asked to write $%02X to $%04hX", (unsigned char)Value, Addr);
		_ppu->WriteReg( Addr, Value );
	}

	// [0x4000, 0x4017] APU and input registers  TODO: mirrored?
	else if ( Addr <= 0x4017 ) 
	{
		g_logger.logTrace("[APU/Input] Asked to write $%02X to $%04hX", (unsigned char)Value, Addr);

		switch( Addr ) // http://wiki.nesdev.com/w/index.php/APU_registers
		{
			case 0x4000: // APU Pulse 1 Control Register. (Duty, loop envelope/disable length counter, constant volume, envelope period/volume)
				break;
			case 0x4001: // APU Pulse 1 Ramp Control Register.
				break;
			case 0x4002: // APU Pulse 1 Fine Tune (FT) Register.
				break;
			case 0x4003: // APU Pulse 1 Coarse Tune (CT) Register.
				break;
			case 0x4004: // APU Pulse 2 Control Register.
				break;
			case 0x4005: // APU Pulse 2 Ramp Control Register.
				break;
			case 0x4006: // APU Pulse 2 Fine Tune Register.
				break;
			case 0x4007: // APU Pulse 2 Coarse Tune Register.
				break;
			case 0x4008: // APU Triangle Control Register 1.
				break;
			case 0x4009: // APU Triangle Control Register 2.
				break;
			case 0x400A: // APU Triangle Frequency Register 1.
				break;
			case 0x400B: // APU Triangle Frequency Register 2.
				break;
			case 0x400C: // APU Noise Control Register 1.
				break;
			case 0x400D: // Unused?
				//g_logger.log("is $400D unused?"); // Used by lunar pool
				//ASSERT( 0 );
				break;
			case 0x400E: // APU Noise Frequency Register 1.
				break;
			case 0x400F: // APU Noise Frequency Register 2.
				break;
			case 0x4010: // APU Delta Modulation Control Register.
				break;
			case 0x4011: // APU Delta Modulation D/A Register.
				break;
			case 0x4012: // APU Delta Modulation Address Register.
				break;
			case 0x4013: // APU Delta Modulation Data Length Register.
				break;
			case 0x4014: // Sprite DMA (W) - write 256 bytes from cpuRam[n*$100] to &sprRam[0]
				{
					g_logger.logDebug(" * 256 byte sprite DMA xfer from cpu[0x%04hX]", 0x100*Value); // TODO - vector besafe below?
					address curr2003 = _ppu->getAddr2003();
					if( curr2003 == 0 )
						memcpy(&_ppu->sprRam[0], &_cpuRam[0x100*Value], 0x100); // TODO - unroll this? :D
					else 
					{
						// need to start copy at $2003's addr and wrap
						int offset = 0x100 - curr2003;
						memcpy(&_ppu->sprRam[curr2003], &_cpuRam[0x100*Value], offset);
						memcpy(&_ppu->sprRam[0], &_cpuRam[(0x100*Value) + offset], curr2003);
					}
					Cpu6502::incrementCyclesElapsed(512); //  256 copies at 2 opcodes each = 512 (+1 which i think the m6502 already accounts for to write to this register)
				}
				break;
			case 0x4015: // Sound Channel Switch (W)
				break;
			case 0x4016: // Joystick1 + Strobe (RW)
				if (Value & 0x01) { // 0x01: record state of each button on the controller
					switch (reg4016strobe)
					{

						case LATCHED: // aka 'half-strobed'
							g_logger.log(" *!* Already latched. NO-OP?");
							ASSERT( 0 );
							break;

						case UNKNOWN_STROBE_STATE:	 g_logger.log(" * In unknown strobe state");
						case STROBED:
							reg4016strobe     = LATCHED;
							reg4016readno     = 0;
							pad1_buttons_save =	pad1_buttons & 0x0F; // allows auto-repeat of dpad but not buttons
							pad1_buttons      = g_hid->getButtonsPressed();
							/*{
								if (pad1_buttons & HID::A)      { g_logger.log("[PAD] A was hit"); }
								if (pad1_buttons & HID::B)      { g_logger.log("[PAD] B was hit"); }
								if (pad1_buttons & HID::SELECT) { g_logger.log("[PAD] SELECT was hit"); }
								if (pad1_buttons & HID::START)  { g_logger.log("[PAD] START was hit"); }
								if (pad1_buttons & HID::UP)     { g_logger.log("[PAD] UP was hit"); }
								if (pad1_buttons & HID::DOWN)   { g_logger.log("[PAD] DOWN was hit"); }
								if (pad1_buttons & HID::LEFT)   { g_logger.log("[PAD] LEFT was hit"); }
								if (pad1_buttons & HID::RIGHT)  { g_logger.log("[PAD] RIGHT was hit"); }
							}*/
							break;
						
						default:
							g_logger.log("Bad reg4016strobe state: %d", reg4016strobe);
							ASSERT( 0 );
							break;
					}
				}
				else      // 0x00: If now strobed, allow buttons to be read back one by one from d0
				{
					switch (reg4016strobe)
					{
						case UNKNOWN_STROBE_STATE:	
							g_logger.log(" *!* LATCH DID NOT HAPPEN BEFORE FAILED ATTEMPT TO STROBE!");
							//ASSERT( 0 );
							break;

						case LATCHED: // aka 'half-strobed' - Latched -> Strobed  (Allow controller bit reads now)
							reg4016strobe = STROBED;
							break;

						case STROBED:
							g_logger.log(" *!* Already strobed. NO-OP?");
							ASSERT(0);
							break;
			
						default:
							g_logger.log("Invalid reg4016strobe state");
							ASSERT(0);
					}
				}
								
			case 0x4017: // Joystick2 + Strobe (RW)

				// ALL WRITES WILL:
				// Reset frame counter
				// ;

				// Reset clock divider
				// ;

				/* "Writes to register $4017 control operation of both 
					the clock divider and the frame counter." */

				/*
				sound channels 
using the $4017.7=0 frequencies (60, 120, and 240 Hz). For $4017.7=1 
operation, replace those frequencies with 48, 96, and 192 Hz (respectively).*/

				// D7 - 0 has divider of /4, frame IRQ freq of 60
				//    - 1 has divider of /5, frame IRQ freq of 48
				if (Value & (1<<7)) {
					g_logger.logTrace("$4017.7=1 (W): Clock Audio counters now (and /4 clock divider)");
				}
				else {
					g_logger.logTrace("$4017.7=0 (W): (use /5 clock divider)");
				}
				if (Value & (1<<6)) {
					g_logger.logTrace("$4017.6=1 (W): Disable frame IRQ");
				}
				else {
					g_logger.logTrace("$4017.6=0 (W): Enable frame IRQ");
				}
				break;
			default:
				g_logger.log("Bad write to address: %04hX, value:%02hX ", Addr, Value);
				ASSERT( 0 );
		}
	}

	// don't write to PRG-ROM
	else if( Addr >= 0x8000 ) 
	{
		g_logger.log("ERROR: attempted to write $%04hX (Cart ROM area) with value $%02hX", Addr, Value);
		ASSERT( 0 );
	}

	// SHOULD NEVER GET HERE
	else 
	{
		g_logger.log("BAD!!! addr: %d", Addr);
		ASSERT( 0 ); // should never get here!
	}
}

byte Cpu6502::Read(register address Addr)
{	
	ASSERT( Addr >= 0x0000 && Addr <= 0xFFFF );
	g_logger.logTrace("Reading $%04hX", Addr);

	// [0x0000, 0x2000) - CPU internal ram, 8kb mirror (0x0800 * 4 = 0x2000)
	if (Addr < 0x2000) {
		return _cpuRam[Addr & CPU_RAM__MASK];   // TODO: vector and BE_SAFE
	}
	
	// [0x2000, 0x4000) - PPU registers
	else if ( Addr < 0x4000 ) 
	{ 
		return _ppu->ReadReg( Addr & 0x2007 );  // 8 bytes of registers mirrored through 0x3FFF
	}

	// [0x4000, 0x4017] APU and input registers  TODO: mirrored?
	else if ( Addr <= 0x4017 ) 
	{
		if (Addr != 0x4017 && Addr != 0x4016) {
			g_logger.log("[APU] Asked to read $%04hX", Addr);
		}
		else {
			g_logger.logTrace("[APU] Asked to read $%04hX", Addr);
		}

		switch( Addr ) // http://wiki.nesdev.com/w/index.php/APU_registers
		{
			case 0x4000: // APU Pulse 1 Control Register. (Duty, loop envelope/disable length counter, constant volume, envelope period/volume)
			case 0x4001: // APU Pulse 1 Ramp Control Register.
			case 0x4002: // APU Pulse 1 Fine Tune (FT) Register.
			case 0x4003: // APU Pulse 1 Coarse Tune (CT) Register.
			case 0x4004: // APU Pulse 2 Control Register.
			case 0x4005: // APU Pulse 2 Ramp Control Register.
			case 0x4006: // APU Pulse 2 Fine Tune Register.
			case 0x4007: // APU Pulse 2 Coarse Tune Register.
			case 0x4008: // APU Triangle Control Register 1.
			case 0x4009: // APU Triangle Control Register 2.
			case 0x400A: // APU Triangle Frequency Register 1.
			case 0x400B: // APU Triangle Frequency Register 2.
			case 0x400C: // APU Noise Control Register 1.
			case 0x400D: // Unused?
				//g_logger.log("is $400D unused?");
				//break;
			case 0x400E: // APU Noise Frequency Register 1.
			case 0x400F: // APU Noise Frequency Register 2.
			case 0x4010: // APU Delta Modulation Control Register.
			case 0x4011: // APU Delta Modulation D/A Register.
			case 0x4012: // APU Delta Modulation Address Register.
			case 0x4013: // APU Delta Modulation Data Length Register.
			case 0x4014: // *** DMA Access to the Sprite Memory (W)
			case 0x4015: // Sound Channel Switch (W)
				g_logger.log("ERROR: attempted to read write-only $%04hX", Addr);
				ASSERT( 0 );
				break;

			case 0x4016: // Joystick1 + Strobe (RW)
			{
				if (reg4016strobe != STROBED)
				{
					g_logger.log(" [0x4016]Trying to read from pad not in 'STROBED' state, returning 0");
					ASSERT( 0 );
				}

				byte ret = 0;

				ASSERT(reg4016readno >= 0 && reg4016readno < 24);
				if (reg4016readno < 8) {
					//ret = (pad1_buttons >> reg4016readno) & 0x01;
					ret = ((pad1_buttons & ~pad1_buttons_save) >> reg4016readno) & 0x01; // fixes rapid-fire issue with buttons while preserving behavor for dpad
				}
				else if (reg4016readno < 16) {
					ret = 0; // TODO: for now. i've read unused as 0 and 1
				}
				else if (reg4016readno < 24) {	
					static const byte p1padSignature[8] = {0, 0, 0, 1, 0, 0, 0, 0};
					ret = p1padSignature[reg4016readno];
				}
				else {
					g_logger.log(" ERROR: too many reads to pad.");
					ASSERT( 0 );
				}
				reg4016readno++;
				if (reg4016readno==24)
					reg4016readno = 0;

				return ret;
			}

			case 0x4017: // Joystick2 + Strobe (RW)
				//g_logger.log("Returning hardcoded 0 from read to $4017");
				//return pad2_buttons;
				return 0;
				break;
			default:
				g_logger.log("! Bad address: %04hX", Addr);
				ASSERT( 0 );
		}
	}

	// [0x8000, 0xFFFF] read from PRG-ROM
	else if (Addr >= 0x8000) // this can be removed for efficiency later
	{
		Addr &= PRG_ROM__MASK;
		return PrgRom[Addr];
	}

	// SHOULD NEVER GET HERE
	else 
	{
		g_logger.log("BAD!!! $%04hX / %d reached...", Addr, Addr);
		ASSERT( 0 ); // should never get here!
	}

	return 69;
}

byte Cpu6502::ReadOpcodeOnly(register address Addr)
{
	g_logger.log(" I NEVER KNEW ReadOpcodeOnly() got called! Addr: %04hX", Addr);
	ASSERT( 0 ); // WHEN IS THIS CALLED?
	return 0;
}
byte Cpu6502::HandleBadOpcode(register byte op)
{
	g_logger.log("ERROR: bad opcode %d", op);
	ASSERT( 0 );
	return 0;
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