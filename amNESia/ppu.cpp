#include "Ppu.h"

Ppu::byte Ppu::PpuRam[PPU_RAM__SIZE];
Ppu::byte Ppu::SprRam[PPU_SPR_RAM__SIZE];

Ppu::Ppu(void)
{
}

Ppu::~Ppu(void)
{
}

void Ppu::Write(Ppu::address Addr, Ppu::byte Value)
{
	// 0x4000-0xFFFF mirrors 0x0000-0x3FFF
	Addr &= 0x3FFF;

	// Non-mirrored addresses from 0x0000-0x3000
	if( Addr < 0x3000 ) {
		PpuRam[ Addr ] = Value;
	}
	// 0x3000-0x3EFF mirrors 0x2000-0x2EFF
	else if( Addr > 0x2FFF && Addr < 0x3F00 ) {
		PpuRam[ Addr - 0x1000 ] = Value;
	}
	// 0x3F20-0x3FFF mirrors 0x3F00-0x3F1F
	else {
		PpuRam[ Addr & 0x3F1F ] = Value; // TODO: double check this math
	}
}

Ppu::byte Ppu::Read(Ppu::address Addr)
{
	// 0x4000-0xFFFF mirrors 0x0000-0x3FFF
	Addr &= 0x3FFF;

	// Non-mirrored addresses from 0x0000-0x3000
	if( Addr < 0x3000 ) {
		return PpuRam[ Addr ];
	}
	// 0x3000-0x3EFF mirrors 0x2000-0x2EFF
	else if( Addr > 0x2FFF && Addr < 0x3F00 ) {
		return PpuRam[ Addr - 0x1000 ];
	}
	// 0x3F20-0x3FFF mirrors 0x3F00-0x3F1F
	else { // if( Addr > 0x3F1F && Addr < 0x4000 )
		return PpuRam[ Addr & 0x3F1F ];
	}
}
