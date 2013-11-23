#pragma once

#define PPU_RAM__SIZE 0x4000		// 16kb
#define PPU_SPR_RAM__SIZE 0x0100	// 256 bytes

class Ppu
{
public:
	typedef unsigned char byte;
	typedef unsigned short address;
	static void Write(register address Addr, register byte Value);
	static byte Read(register address Addr);

	static byte PpuRam[PPU_RAM__SIZE];
	static byte SprRam[PPU_SPR_RAM__SIZE];

private:
	Ppu(void);
	~Ppu(void);

};
