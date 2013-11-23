#pragma once

#include "amNESia.h"

namespace amnesia {

class HID
{
public:
	HID(void);
	~HID(void);

	typedef BYTE button_mask_t;

	inline button_mask_t getButtonsPressed()  { return _buttons; }
	void processRawInput(LPARAM lParam);

	enum button_t
	{
		NO_BUTTON   =     0 ,
		A           = (1<<0),
		B           = (1<<1),
		SELECT      = (1<<2),
		START       = (1<<3),
		UP          = (1<<4),
		DOWN        = (1<<5),
		LEFT        = (1<<6),
		RIGHT       = (1<<7),
		BUTTON_MASK = ((1<<8)-1)
	};

	enum hid_raw_button_actions_t 
	{ 
		UNKNOWN  = 0x0000, 
		PRESSED  = 0x0100, 
		RELEASED = 0x0101	
	};

	struct keys_s {
		enum keys_t 
		{
			UP = 0x0026,
			DOWN = 0x0028,
			LEFT = 0x0025, 
			RIGHT = 0x0027,
			ENTER = 0x000D,
			TAB = 0x0009,
			D = 0x0044,
			F = 0x0046,
			F1 = 0x0070,
			F2 = 0x0071,
			F3 = 0x0072,
			F4 = 0x0073,
			F5 = 0x0074,
			F6 = 0x0075,
			F7 = 0x0076,
			F8 = 0x0077,
			F9 = 0x0078,
			F10 = 0x0079,
			F11 = 0x007A,
			//F12 = 0x0070, dont use for now, counts as break out running program in Visual Studio
			END_OF_KEYS = 0xFFFF
		};
	} keys;
		

private:
	vector<RAWINPUTDEVICE> _devices;
	button_mask_t          _buttons;
};

} // end namespace amnesia