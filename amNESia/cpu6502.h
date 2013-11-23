#pragma once

#ifdef _WIN32
#include "M6502\M6502.h"
#else
#include "M6502/M6502.h"
#endif

#define CPU_RAM__SIZE 0x0800    // 2kb (8kb mirrored four ways)
#define PRG_ROM__SIZE 0x8000	// 32kb

#define CPU_RAM__MASK 0x07FF	//
#define PRG_ROM__MASK 0x7FFF	//


class Cpu6502
{
public:
	typedef unsigned char byte;
	typedef unsigned short address;

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

private:
	Cpu6502(void) {};
	~Cpu6502(void) {};

	static M6502 _m6502;	
	static byte CpuRam[CPU_RAM__SIZE];
};
