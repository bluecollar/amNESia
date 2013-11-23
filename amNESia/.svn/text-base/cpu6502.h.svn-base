#pragma once
#include "amnesia.h"

#include "m6502\M6502.h"
#include "HID.h" // temp TODO remove via refactoring
#include "ppu.h" // only until we have a containing nes object

#define CPU_RAM__SIZE 0x0800    // 2kb (8kb mirrored four ways)
#define PRG_ROM__SIZE 0x8000	// 32kb

#define CPU_RAM__MASK 0x07FF	//
#define PRG_ROM__MASK 0x7FFF	//


namespace amnesia {


class Cpu6502
{
public:
	//static void Init(Ppu*); //temp
	static void AttachPpu(Ppu*); //temp
	static void Reset();

	static void Write(register address Addr, register byte Value);
	static byte Read(register address Addr);
	static byte ReadOpcodeOnly(register address Addr);
	static byte HandleBadOpcode(register byte op);
	static byte ServiceInterrupts();
	static byte PrgRom[PRG_ROM__SIZE];
	static void Run();
	static int Run(int runCycles);

	static M6502* getM6502() { return &_m6502; }

	static inline void incrementCyclesElapsed( unsigned int n ) { _numCycles += n; }
	static inline unsigned long getCyclesElapsed() { return _numCycles; }

	enum reg4016strobe_states { STROBED=0x00, LATCHED=0x01, UNKNOWN_STROBE_STATE=0x02 };
	static byte reg4016strobe;

	static byte reg4016readno;

	static void executeIRQ();
	static void executeNMI();

	enum nes_signatures_t {
		DISCONNECTED = 0x00,
		JOYPAD_1     = 0x01,
		JOYPAD_2     = 0x02
	};

	static HID::button_mask_t pad1_buttons;
	static HID::button_mask_t pad2_buttons;

	static HID::button_mask_t pad1_buttons_save;
	static HID::button_mask_t pad2_buttons_save;

private:
	Cpu6502() {};
	~Cpu6502() {};

	static Ppu*  _ppu;
	static M6502 _m6502;	
	static byte  _cpuRam[CPU_RAM__SIZE];
	static u32   _numCycles;

	//	static vector<byte>  _cpuRam[CPU_RAM__SIZE]; // TODO 
};

} // end namespace amnesia