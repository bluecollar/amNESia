#include "amnesia.h"

#include <fstream>
#include "cart.h"


using namespace amnesia;
using namespace std;


Cart::Cart()
{
	_rom = NULL; 
	_romSize = 0;
	_hasSaveRam = false;
	_saveRam = NULL;
	_saveRamSize = 0; 
	_romfile = "";
	_isLoaded = false;	
}

Cart::~Cart() 
{
	if (_isLoaded) {
		if (_rom) {
			delete[] _rom;
			_rom = NULL;
		}
	}
}


bool Cart::isLoaded() 
{
	return _isLoaded; 
}

int Cart::loadCartFromFile(string filename) 
{ 
	ifstream infile(filename.c_str(), ios::binary);
	if (!infile.is_open())
		return -1;
    
	infile.seekg(0, ios_base::end);
	int cartsize = (int)infile.tellg() - 16;
	infile.seekg(0, ios_base::beg);
	
	// Read iNES header (TODO: these two lines need to be better coupled)
	infile.read(_header, 16);
	loadRomHeader();

	//
	_rom = new char[cartsize];
	if (!_rom)
		return -2;

	infile.read(_rom, cartsize);
	if (infile.fail())
		return -3;

	_isLoaded = true;
	_romfile = filename;
	_romSize = cartsize;

	return SUCCESS; 
}

Cart::romHeader Cart::getRomHeader()
{
	return _iNesHeader;
}

unsigned int Cart::getRomSize()
{
	return _romSize;
}

int Cart::readRom(int offset, byte *out_buffer, unsigned int numBytes)
{
	memcpy(out_buffer, &(_rom[offset]), numBytes);
	return SUCCESS;
}

int Cart::getSaveRamSize()
{
	return _saveRamSize;
}

int Cart::readSaveRam(byte *outBuf, unsigned int maxBytes)
{
	// TODO: protection
	int numBytes = (maxBytes < _saveRamSize ? maxBytes : _saveRamSize);
	memcpy(outBuf, _saveRam, numBytes);
	return SUCCESS;
}

int Cart::writeSaveRam(byte *inBuf, unsigned int numBytes)
{
	// TODO: protection
	memcpy(_saveRam, inBuf, numBytes);
	return SUCCESS;
}

int Cart::loadRomHeader()
{
	char* hdrData = _header;

	//0-3      String "NES^Z" used to recognize .NES files.
	if( hdrData[0] == 'N' && hdrData[1] == 'E' && hdrData[2] == 'S' && hdrData[3] == 0x1A ) {
		g_logger.logDebug("valid NES file");
		_iNesHeader.isNES = true;
	}
	else {
		g_logger.logDebug("invalid NES file!");
		return 1;
	}

	//4        Number of 16kB ROM banks.
	_iNesHeader.numRomBanks = hdrData[4];
	g_logger.logDebug("%d 16kB PRG-ROM banks", _iNesHeader.numRomBanks);

	//5        Number of 8kB VROM banks.
	_iNesHeader.numVRomBanks = hdrData[5];
	g_logger.logDebug("%d VROM banks", _iNesHeader.numVRomBanks);

	//6        bit 0     1 for vertical mirroring, 0 for horizontal mirroring.
	_iNesHeader.verticalMirroring = ( (hdrData[6] & 1 ) > 0 ); 
	g_logger.logDebug("%s mirroring", _iNesHeader.verticalMirroring ? "vertical":"horizontal");

	//         bit 1     1 for battery-backed RAM at $6000-$7FFF.
	_iNesHeader.hasBatteryBackedRAM  = ( (hdrData[6] & (1 << 2) ) > 0 );
	if (_iNesHeader.hasBatteryBackedRAM)
		g_logger.logDebug("SRAM (Save RAM) exists");

	//         bit 2     1 for a 512-byte trainer at $7000-$71FF.
	_iNesHeader.hasTrainer  = ( (hdrData[6] & (1 << 3) ) > 0 );
	if (_iNesHeader.hasTrainer)
		g_logger.logDebug("Trainer exists");

	//         bit 3     1 for a four-screen VRAM layout. 
	_iNesHeader.has4ScreenVRAM  = ( (hdrData[6] & (1<<4) ) > 0 );
	if (_iNesHeader.has4ScreenVRAM)
		g_logger.logDebug("Has 4 screen VRAM layout");

	//         bit 4-7   Four lower bits of ROM Mapper Type.
	_iNesHeader.mapperType = (0xF0 & hdrData[6]) >> 4;
	
	//7        bit 0     1 for VS-System cartridges.
	_iNesHeader.isVSSystemCart = ( (hdrData[7] & 1 ) > 0 ); 
	if (_iNesHeader.isVSSystemCart)
		g_logger.logDebug("id VS System Cart");

	//         bit 1-3   Reserved, must be zeroes!
	//         bit 4-7   Four higher bits of ROM Mapper Type.
	_iNesHeader.mapperType |= (0xF0 & hdrData[7]);
	g_logger.logDebug("mapper type: %d", _iNesHeader.mapperType);

	//8        Number of 8kB RAM banks. For compatibility with the previous
	//         versions of the .NES format, assume 1x8kB RAM page when this
	//         byte is zero.
	_iNesHeader.numRAMBanks = ( hdrData[8] > 0 ) ? hdrData[8] : 1;
	g_logger.logDebug("%d 8kB RAM banks", _iNesHeader.numRAMBanks);

	//9        bit 0     1 for PAL cartridges, otherwise assume NTSC.
	_iNesHeader.isNTSC = ( (hdrData[9] & 1 ) == 0 );
	g_logger.logDebug("is %s", _iNesHeader.isNTSC ? "NTSC":"PAL");

	//         bit 1-7   Reserved, must be zeroes!
	//10-15    Reserved, must be zeroes!

	return 0; // SUCCESS
}