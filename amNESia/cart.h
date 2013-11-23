#pragma once

#include <string>
#include "amnesia_util.h"

using std::string;

namespace amnesia {

class Cart
{
public:
	Cart();
	~Cart();

	typedef unsigned char byte;
	typedef unsigned short address;

	// iNES ROM header info
	struct romHeader
	{
		bool isNES;
		unsigned char numRomBanks;
		unsigned char numVRomBanks;
		bool verticalMirroring;
		bool hasBatteryBackedRAM;
		bool hasTrainer; 
		bool has4ScreenVRAM;
		unsigned char mapperType;
		bool isVSSystemCart;
		unsigned char numRAMBanks;
		bool isNTSC;
	};

	bool isLoaded();
	int loadCartFromFile(string filename);
	romHeader getRomHeader();

	unsigned int getRomSize();
	int readRom(int offset, byte *out_buffer, unsigned int numBytes);

	int getSaveRamSize();
	int readSaveRam(byte *outBuf, unsigned int maxBytes=0);
	int writeSaveRam(byte *inBuf, unsigned int numBytes);

private:

	// Actual cart program data
	char *_rom;
	unsigned int _romSize;

	// Save ram used for saving games to the cart
	bool _hasSaveRam;
	unsigned char *_saveRam;
	unsigned int _saveRamSize;
	// extensions

	// file we used to load cart
	string _romfile;
	bool _isLoaded;
	romHeader _iNesHeader;
	
	void loadRomHeader(char* hdrData);
};

}// end namespace amnesia
