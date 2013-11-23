#include "amNESia.h"

#include <Strsafe.h> // STRSAFE_MAX_CCH
#include "HID.h"
#include "ppu.h"    // TODO: TO BE REMOVED UN REFACTORING OF UGLAGE


using namespace amnesia;


// THIS IS UGLY AND REALLY BAD.
// A "NES" CONTAINER OBJECT OR OTHER LIGHTWEIGHT "GAME MANAGER" OBJECT 
// CONSTRUCTED IN THE FUTURE WILL SOLVE THIS, BUT I WANT TO GET THIS GOING RIGHT NOW.
// TODO.
extern Ppu* ppu;
// END OF TODO


HID::HID() : _buttons(0)
{
	//GetRawInputDeviceList();

	RAWINPUTDEVICE device;

	// Set up raw windows input
	device.usUsagePage = 0x01; // adds keyboard
	device.usUsage     = 0x06;  
	device.dwFlags     = 0x00; // TODO: consider RIDEV_LEGACY ?
	device.hwndTarget  = 0x00;

	_devices.push_back(device);

	vector<RAWINPUTDEVICE>::iterator it;
	for (it=_devices.begin(); it!=_devices.end(); ++it) {
		if (RegisterRawInputDevices(&(*it), 1, sizeof(RAWINPUTDEVICE)) == FALSE) {
			g_logger.log("[FATAL] RegisterRawInputDevices failed, error: %ld", GetLastError());
			ASSERT( 0 );
		}
	}
}
HID::~HID()
{
	_devices.clear();
}

void HID::processRawInput(LPARAM lParam)
{
	UINT dwSize;

	HRAWINPUT* rawInput = (HRAWINPUT *)&lParam;

	// get size of input data we need to pull
	GetRawInputData(*rawInput, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
	LPBYTE lpb = new BYTE[dwSize];
	ASSERT( lpb );

	// get the actual input data
	if (GetRawInputData(*rawInput, RID_INPUT, lpb, &dwSize, 
				sizeof(RAWINPUTHEADER)) != dwSize) {
		OutputDebugString(TEXT("GetRawInputData() returned mis-sized information!"));
		ASSERT( 0 );
		return;
	}

	// now interpret that data
	RAWINPUT* raw = (RAWINPUT*)lpb;
	if (raw->header.dwType == RIM_TYPEKEYBOARD) 
	{
		TCHAR szTempOutput[1024] = {0};
		HRESULT hResult = StringCchPrintf(szTempOutput, STRSAFE_MAX_CCH, 
			TEXT(" Kbd: make=%04x Flags:%04x Reserved:%04x ExtraInformation:%08x, msg=%04x VK=%04x \n"), 
				raw->data.keyboard.MakeCode, 
				raw->data.keyboard.Flags, 
				raw->data.keyboard.Reserved, 
				raw->data.keyboard.ExtraInformation, 
				raw->data.keyboard.Message, 
				raw->data.keyboard.VKey);
			
			if (FAILED(hResult))
			{
				OutputDebugString(L"Result indicated raw data input failure!");
				ASSERT( 0 );
			}

		// Determine new key codes by uncommented this! :D
		//OutputDebugString(szTempOutput);
		// :D :D :D
			
		button_t nesButton = NO_BUTTON;
		switch(raw->data.keyboard.VKey)
		{
			case keys.UP:    nesButton = UP;     break;
			case keys.DOWN:  nesButton = DOWN;   break;
			case keys.LEFT:  nesButton = LEFT;   break;
			case keys.RIGHT: nesButton = RIGHT;  break;
			case keys.ENTER: nesButton = START;  break;
			case keys.TAB:   nesButton = SELECT; break;
			case keys.D:     nesButton = B;      break;
			case keys.F:     nesButton = A;      break;
			
			default: 
				switch(raw->data.keyboard.VKey)
				{
					case keys.F1:  // F1  (toggle logging level)
						if (raw->data.keyboard.Message == 0x101) // 0x101 is released, 0x100 is depressed. do this so we only get one toggle per press, holding it down causes many 0x100 fires
							g_logger.shiftLevel();
						break;

					case keys.F4: // F5 (toggle scanline rendering)
						if (raw->data.keyboard.Message == 0x101) // 0x101 is released, 0x100 is depressed. do this so we only get one toggle per press, holding it down causes many 0x100 fires
							ppu->debug().toggleScanline();
						break;

					case keys.F5: // F5 (toggle VBlank flag reset)
						if (raw->data.keyboard.Message == 0x101) // 0x101 is released, 0x100 is depressed. do this so we only get one toggle per press, holding it down causes many 0x100 fires
							ppu->debug().toggleVBlankReset();
						break;	
					
					case keys.F6:  // F6  (toggle background layer)
						if (raw->data.keyboard.Message == 0x101) // 0x101 is released, 0x100 is depressed. do this so we only get one toggle per press, holding it down causes many 0x100 fires
							ppu->debug().toggleBackgroundLayer();
						break;
					
					case keys.F7:  // F7  (toggle sprite layer)
						if (raw->data.keyboard.Message == 0x101) // 0x101 is released, 0x100 is depressed. do this so we only get one toggle per press, holding it down causes many 0x100 fires
							ppu->debug().toggleSpriteLayer();
						break;

					case keys.F8: // F8 (toggle name table display)
						if (raw->data.keyboard.Message == 0x101) // 0x101 is released, 0x100 is depressed. do this so we only get one toggle per press, holding it down causes many 0x100 fires
							ppu->debug().toggleNameTables();
						break;

					case keys.F9: // F9 (toggle pattern table display)
						if (raw->data.keyboard.Message == 0x101) // 0x101 is released, 0x100 is depressed. do this so we only get one toggle per press, holding it down causes many 0x100 fires
							ppu->debug().togglePatternTables();
						break;

					case keys.F11: // F11
						break;

					default:
						break;
				}
		}

		if (raw->data.keyboard.Message == PRESSED)
			_buttons |=   nesButton;
		else if (raw->data.keyboard.Message == RELEASED)
			_buttons &=  ~nesButton;
	}
	// this has to go up to connect to the if
	/*else if (raw->header.dwType == RIM_TYPEMOUSE) 
	{
		HRESULT hResult = StringCchPrintf(szTempOutput, STRSAFE_MAX_CCH, TEXT("Mouse: usFlags=%04x ulButtons=%04x usButtonFlags=%04x usButtonData=%04x ulRawButtons=%04x lLastX=%04x lLastY=%04x ulExtraInformation=%04x\r\n"), 
		raw->data.mouse.usFlags, 
		raw->data.mouse.ulButtons, 
		raw->data.mouse.usButtonFlags, 
		raw->data.mouse.usButtonData, 
		raw->data.mouse.ulRawButtons, 
		raw->data.mouse.lLastX, 
		raw->data.mouse.lLastY, 
		raw->data.mouse.ulExtraInformation);
			if (FAILED(hResult))
			{
			// TODO: write error handler
			}
        OutputDebugString(szTempOutput);
	}*/

	delete[] lpb;
}
