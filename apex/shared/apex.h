// Guard to prevent multiple inclusions of this header file
#ifndef APEX_H
#define APEX_H

// Include necessary header files from the library directory
#include "../../library/vm.h"
#include "../../library/math.h"

// Check if not in kernel mode
#ifndef _KERNEL_MODE
#include <stdio.h>          // Include standard input-output header for user mode
#define DEBUG               // Define DEBUG macro for debugging purposes
#define LOG printf          // Define LOG macro as an alias for printf function for logging
#endif

// #define DEBUG
#define LOG(...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)

// Define custom data types for game elements
typedef QWORD C_Entity;      // Custom type for game entities
typedef QWORD C_Player;      // Custom type for players
typedef QWORD C_Weapon;      // Custom type for weapons

// Main namespace for Apex Legends related functions and utilities
namespace apex
{
    // Function to check if the game is running
	BOOL running(void);

    // Function to reset global variables or states
	void reset_globals(void);

    // Function to reset global variables or states
	namespace entity
	{
		// Function to retrieve an entity based on its index
		C_Entity get_entity(int index);
	}

    // Namespace for functions related to game teams
	namespace teams
	{
		C_Player get_local_player(void);
	}

    // Namespace for functions related to the game engine
	namespace engine
	{
		float get_sensitivity(void);
		DWORD get_current_tick(void);
	}

    // Namespace for functions related to game input
	namespace input
	{
		// Function to check the state of a button
		BOOL  get_button_state(DWORD button);

	    // Function to move the mouse to specified coordinates
		void  mouse_move(int x, int y);
	}

	//
	// use it with teams (Ekknod)
	// Namespace for functions related to players
	namespace player
	{
		// Various functions to retrieve or set player attributes
		BOOL         is_valid(C_Player player_address);
		float        get_visible_time(C_Player player_address);
		int          get_team_id(C_Player player_address);
		BOOL         get_muzzle(C_Player player_address, vec3 *vec_out);
		BOOL         get_bone_position(C_Player player_address, int index, vec3 *vec_out);
		BOOL         get_velocity(C_Player player_address, vec3 *vec_out);
		BOOL         get_viewangles(C_Player player_address, vec2 *vec_out);
		void         enable_glow(C_Player);
		C_Weapon     get_weapon(C_Player player_address);
	}

	// Namespace for functions related to weapons
	namespace weapon
	{
		// Functions to retrieve weapon attributes
		float        get_bullet_speed(C_Weapon weapon_address);
		float        get_bullet_gravity(C_Weapon weapon_address);
		float        get_zoom_fov(C_Weapon weapon_address);
	}
}

// End of include guard
#endif /* apex.h */

