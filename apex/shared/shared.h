/*
 * originally written in C (2019), ported to C++ (17/09/2022)
 * i think code is easier to understand when it's written with C++.
 * 
 * what EC-CSGO open-source version lacks of(?)
 * - security features
 *
 */

// Ensure that the contents of this header file are included only once in a compilation.
#ifndef SHARED_H
#define SHARED_H

// If the code is being compiled for kernel mode, include the external declaration for _fltused.
#ifdef _KERNEL_MODE
extern "C" int _fltused;
#endif

// Include the "apex.h" header file.
#include "apex.h"

//
// features.cpp
//
// Declare the "features" namespace which contains functions related to the features of the application.
namespace features
{
	// Function to run the features.
	void run(void);
	// Function to reset the features.
	void reset(void);
}


//
// implemented by application/driver
//
// Declare the "input" namespace which contains functions related to input handling.
// These functions are implemented by the application or driver.
namespace input
{
	// External function to move the mouse by the given x and y offsets.
	extern void mouse_move(int x, int y);
}

//
// implemented by application/driver
//
// Declare the "config" namespace which contains configuration variables.
// These variables are implemented by the application or driver.
namespace config
{
	// External variable representing the button used for the aimbot feature.
	extern DWORD aimbot_button;
	// External variable representing the field of view for the aimbot.
	extern float aimbot_fov;
	// External variable representing the smoothness of the aimbot's movement.
	extern float aimbot_smooth;
	// External variable indicating whether the aimbot should check for target visibility.
	extern BOOL  aimbot_visibility_check;
	// External variable indicating whether visuals are enabled.
	extern BOOL  visuals_enabled;
}

// Declare the "apex_legends" namespace which contains functions related to the Apex Legends game.
namespace apex_legends
{
	// Inline function to run the features if Apex Legends is running, or reset them if not.
	inline void run(void)
	{
		if (apex::running())
		{
			features::run();
		} else {
			features::reset();
		}
	}
}

// End the inclusion guard for this header file.
#endif // SHARED_H

