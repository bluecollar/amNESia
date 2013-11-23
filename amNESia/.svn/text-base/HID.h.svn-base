#pragma once

class HID
{
public:
	HID(void);
	~HID(void);

	typedef BYTE button_mask_t;

	enum buttons_t
	{
		UP = 1,
		DOWN = 2,
		LEFT = 4,
		RIGHT = 8,
		SELECT = 16,
		START = 32,
		B = 64,
		A = 128
	};

	button_mask_t getButtonsPressed();
};
