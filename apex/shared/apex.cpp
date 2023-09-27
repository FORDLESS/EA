#include "apex.h"

//
// #define DIRECT_PATTERNS uses scan_pattern_direct
// pros: no need system memory allocation
// cons: performance is worse + function might fail in some cases.
//
#define DIRECT_PATTERNS

// Namespace for all apex-related functions and variables.
namespace apex
{
	// Static variables for various game-related handles and offsets.
	static vm_handle apex_handle       = 0;
	static BOOL      netvar_status     = 0;

	static QWORD     IClientEntityList = 0;
	static QWORD     C_BasePlayer      = 0;
	static QWORD     IInputSystem      = 0;
	static QWORD     m_GetAllClasses   = 0;
	static QWORD     sensitivity       = 0;

	static DWORD     m_dwBulletSpeed   = 0;
	static DWORD     m_dwBulletGravity = 0;

	static DWORD     m_dwMuzzle        = 0;
	static DWORD     m_dwVisibleTime   = 0;

	static int       m_iHealth         = 0;
	static int       m_iViewAngles     = 0;
	static int       m_bZooming        = 0;
	static int       m_lifeState       = 0;
	static int       m_iCameraAngles   = 0;
	static int       m_iTeamNum        = 0;
	static int       m_iName           = 0;
	static int       m_vecAbsOrigin    = 0;
	static int       m_iWeapon         = 0;
	static int       m_iBoneMatrix     = 0;
	static int       m_playerData      = 0;

    	// Forward declarations of internal functions.
	static BOOL initialize(void);
	static BOOL dump_netvars(QWORD GetAllClassesAddress);
	static int  dump_table(QWORD table, const char *name);
}

// Function to check if apex is running.
BOOL apex::running(void)
{
	return apex::initialize();
}

// Function to reset all global variables to their default values.
void apex::reset_globals(void)
{
	apex_handle       = 0;
	netvar_status     = 0;
	IClientEntityList = 0;
	C_BasePlayer      = 0;
	IInputSystem      = 0;
	m_GetAllClasses   = 0;
	sensitivity       = 0;
	m_dwBulletSpeed   = 0;
	m_dwBulletGravity = 0;
	m_dwMuzzle        = 0;
	m_dwVisibleTime   = 0;
	m_iHealth         = 0;
	m_iViewAngles     = 0;
	m_bZooming        = 0;
	m_lifeState       = 0;
	m_iCameraAngles   = 0;
	m_iTeamNum        = 0;
	m_iName           = 0;
	m_vecAbsOrigin    = 0;
	m_iWeapon         = 0;
	m_iBoneMatrix     = 0;
	m_playerData      = 0;
}

// Function to get an entity based on its index.
C_Entity apex::entity::get_entity(int index)
{
	index = index + 1;
	index = index << 0x5;
	return vm::read_i64(apex_handle, (index + IClientEntityList) - 0x280050);
}

// Function to get the local player.
C_Player apex::teams::get_local_player(void)
{
	return vm::read_i64(apex_handle, C_BasePlayer);
}

// Function to get the game's sensitivity setting.
float apex::engine::get_sensitivity(void)
{
	//
	// return vm::read_float(apex_handle, sensitivity);
	//

	//
	// I don't have apex installed to make proper fix
	//
	return 2.5f;
}

// Function to get the current game tick.
DWORD apex::engine::get_current_tick(void)
{
	return vm::read_i32(apex_handle, IInputSystem + 0xcd8);
}

// Function to get the state of a button.
BOOL apex::input::get_button_state(DWORD button)
{
	button = button + 1;
	DWORD a0 = vm::read_i32(apex_handle, IInputSystem + ((button >> 5) * 4) + 0xb0);
	return (a0 >> (button & 31)) & 1;
}

// Function to simulate mouse movement.
void apex::input::mouse_move(int x, int y)
{
	typedef struct
	{
		int x, y;
	} mouse_data;
	mouse_data data;

	data.x = (int)x;
	data.y = (int)y;
	vm::write(apex_handle, IInputSystem + 0x1DB0, &data, sizeof(data));
}

// Function to check if a player is valid.
BOOL apex::player::is_valid(C_Player player_address)
{
	if (player_address == 0)
	{
		return 0;
	}

	if (vm::read_i64(apex_handle, player_address + m_iName) != 125780153691248)
	{
		return 0;
	}

	if (vm::read_i32(apex_handle, player_address + m_iHealth) < 1)
	{
		return 0;
	}

	return 1;
}

// Function to retrieve the time a player has been visible.
float apex::player::get_visible_time(C_Player player_address)
{
	return vm::read_float(apex_handle, player_address + m_dwVisibleTime);
}

// Function to retrieve the team ID of a player.
int apex::player::get_team_id(C_Player player_address)
{
	return vm::read_i32(apex_handle, player_address + m_iTeamNum);
}

// Function to retrieve the muzzle position of a player.
BOOL apex::player::get_muzzle(C_Player player_address, vec3 *vec_out)
{
	return vm::read(apex_handle, player_address + m_dwMuzzle, vec_out, sizeof(vec3));
}

// Structure representing a 3x4 matrix.
typedef struct
{
	unsigned char pad1[0xCC];
	float x;
	unsigned char pad2[0xC];
	float y;
	unsigned char pad3[0xC];
	float z;
} matrix3x4;

// Function to retrieve the position of a specific bone in a player's skeleton.
BOOL apex::player::get_bone_position(C_Player player_address, int index, vec3 *vec_out)
{
	QWORD bonematrix = vm::read_i64(apex_handle, player_address + m_iBoneMatrix);
	if (bonematrix == 0)
	{
		return 0;
	}

	vec3 position;
	if (!vm::read(apex_handle, player_address + m_vecAbsOrigin, &position, sizeof(position)))
	{
		return 0;
	}

	matrix3x4 matrix;
	if (!vm::read(apex_handle, bonematrix + (0x30 * index), &matrix, sizeof(matrix3x4)))
	{
		return 0;
	}

	vec_out->x = matrix.x + position.x;
	vec_out->y = matrix.y + position.y;
	vec_out->z = matrix.z + position.z;
	return 1;
}

// Function to retrieve the velocity of a player.
BOOL apex::player::get_velocity(C_Player player_address, vec3 *vec_out)
{
	return vm::read(apex_handle, player_address + m_vecAbsOrigin - 0xC, vec_out, sizeof(vec3));
}

// Function to retrieve the view angles of a player.
BOOL apex::player::get_viewangles(C_Player player_address, vec2 *vec_out)
{
	return vm::read(apex_handle, player_address + m_iViewAngles - 0x10, vec_out, sizeof(vec2));
}

// Function to enable a glow effect on a player.
void apex::player::enable_glow(C_Player player_address)
{
	vm::write_i32(apex_handle, player_address + 0x2C4, 1512990053);
	vm::write_i32(apex_handle, player_address + 0x3c8, 1);
	vm::write_i32(apex_handle, player_address + 0x3d0, 2);
	vm::write_float(apex_handle, player_address + 0x1D0, 70.000f);
	vm::write_float(apex_handle, player_address + 0x1D4, 0.000f);
	vm::write_float(apex_handle, player_address + 0x1D8, 0.000f);
}

// Function to retrieve the weapon a player is holding.
C_Weapon apex::player::get_weapon(C_Player player_address)
{
	DWORD weapon_id = vm::read_i32(apex_handle, player_address + m_iWeapon) & 0xFFFF;
	return entity::get_entity(weapon_id - 1);
}

// Function to retrieve the bullet speed of a weapon.
float apex::weapon::get_bullet_speed(C_Weapon weapon_address)
{
	return vm::read_float(apex_handle, weapon_address + m_dwBulletSpeed);
}

// Function to retrieve the bullet gravity of a weapon.
float apex::weapon::get_bullet_gravity(C_Weapon weapon_address)
{
	return vm::read_float(apex_handle, weapon_address + m_dwBulletGravity);
}

// Function to retrieve the zoom field of view of a weapon.
float apex::weapon::get_zoom_fov(C_Weapon weapon_address)
{
	return vm::read_float(apex_handle, weapon_address + m_playerData + 0xb8);
}

#ifdef DIRECT_PATTERNS
// Initialization function for the apex module.
static BOOL apex::initialize(void)
{
	// Declare and initialize variables to store base addresses and temporary addresses.
	QWORD apex_base = 0;
	QWORD temp_address = 0;

    	// Check if the apex_handle is already initialized and if the VM is running.	
	if (apex_handle)
	{
		if (vm::running(apex_handle))
		{
			return 1;
		}
		apex_handle = 0;
	}
	
   	 // Open the "r5apex.exe" process and get its handle.
	apex_handle = vm::open_process("r5apex.exe");
	if (!apex_handle)
	{
#ifdef DEBUG
		LOG("[-] r5apex.exe process not found\n");
#endif
		return 0;
	}
	// Get the base address of the "r5apex.exe" module.
	apex_base = vm::get_module(apex_handle, 0);
	if (apex_base == 0)
	{
#ifdef DEBUG
		LOG("[-] r5apex.exe base address not found\n");
#endif
		goto cleanup;
	}
	
	// Scan for the IClientEntityList pattern and retrieve its address.
	IClientEntityList = vm::scan_pattern_direct(apex_handle, apex_base, "\x4C\x8B\x15\x00\x00\x00\x00\x33\xF6", "xxx????xx", 9);
	if (IClientEntityList == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find IClientEntityList\n");
#endif
		goto cleanup;
	}

	{
		temp_address = vm::get_relative_address(apex_handle, IClientEntityList, 3, 7);
		if (temp_address == (IClientEntityList + 0x07))
		{
	#ifdef DEBUG
			LOG("[-] failed to find IClientEntityList\n");
	#endif
			goto cleanup;
		}
		IClientEntityList = temp_address + 0x08;
	}
	
	// Scan for the C_BasePlayer pattern and retrieve its address.
	C_BasePlayer = vm::scan_pattern_direct(apex_handle, apex_base, "\x89\x41\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0", "xxxxxx????xxx", 13);
	if (C_BasePlayer == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find dwLocalPlayer\n");
#endif
		goto cleanup;
	}

	C_BasePlayer = C_BasePlayer + 0x03;


	{
		temp_address = vm::get_relative_address(apex_handle, C_BasePlayer, 3, 7);
		if (temp_address == (C_BasePlayer + 0x07))
		{
	#ifdef DEBUG
			LOG("[-] failed to find dwLocalPlayer\n");
	#endif
			goto cleanup;
		}
		C_BasePlayer = temp_address;
	}
	
	// Scan for the IInputSystem pattern and retrieve its address.
	IInputSystem = vm::scan_pattern_direct(apex_handle, apex_base,
		"\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x4C\x24\x20\xBA\x01\x00\x00\x00\xC7", "xxx????xxxxxxxxxxx", 18);

	if (IInputSystem == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find IInputSystem\n");
#endif
		goto cleanup;
	}

	IInputSystem = vm::get_relative_address(apex_handle, IInputSystem, 3, 7);
	if (IInputSystem == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find IInputSystem\n");
#endif
		goto cleanup;
	}

	IInputSystem = IInputSystem - 0x10;
	
	// Scan for the m_GetAllClasses pattern and retrieve its address.
	m_GetAllClasses = vm::scan_pattern_direct(apex_handle, apex_base,
		"\x48\x8B\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x74\x24\x20", "xxx????xxxxxxxxxxxxxx", 21);

	if (m_GetAllClasses == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find m_GetAllClasses\n");
#endif
		goto cleanup;
	}

	m_GetAllClasses = vm::get_relative_address(apex_handle, m_GetAllClasses, 3, 7);
	m_GetAllClasses = vm::read_i64(apex_handle, m_GetAllClasses);
	if (m_GetAllClasses == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find m_GetAllClasses\n");
#endif
		goto cleanup;
	}

	// Commented out code related to scanning for the sensitivity pattern.
	/*
	sensitivity = vm::scan_pattern_direct(apex_handle, apex_base,
		"\x48\x8B\x05\x00\x00\x00\x00\xF3\x0F\x10\x3D\x00\x00\x00\x00\xF3\x0F\x10\x70\x68", "xxx????xxxx????xxxxx", 20);
	
	if (sensitivity == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find sensitivity\n");
#endif
		goto cleanup;
	}

	sensitivity = vm::get_relative_address(apex_handle, sensitivity, 3, 7);
	sensitivity = vm::read_i64(apex_handle, sensitivity);
	if (sensitivity == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find sensitivity\n");
#endif
		goto cleanup;
	}
	*/


	if (netvar_status == 0)
	{
		// Scan the memory pattern to find the bullet speed and gravity.
		temp_address = vm::scan_pattern_direct(apex_handle, apex_base, "\x75\x0F\xF3\x44\x0F\x10\xBF", "xxxxxxx", 7);
		if (temp_address == 0)
		{
	#ifdef DEBUG
			// Log an error message if the bullet speed/gravity pattern is not found.
			LOG("[-] failed to find bullet speed / gravity\n");
	#endif
			goto cleanup;
		}

		// Read bullet gravity and speed values from the memory.
		m_dwBulletGravity = vm::read_i32(apex_handle, temp_address + 0x02 + 0x05);
		m_dwBulletSpeed = vm::read_i32(apex_handle, temp_address - 0x6D + 0x04);

		// Check if the bullet gravity and speed values are valid.
		if (m_dwBulletGravity == 0 || m_dwBulletSpeed == 0)
		{
	#ifdef DEBUG
			// Log an error message if the bullet gravity/speed values are invalid.
			LOG("[-] failed to find bullet m_dwBulletGravity/m_dwBulletSpeed\n");
	#endif
			goto cleanup;
		}

		// Scan the memory pattern to find the bullet muzzle.
		temp_address = vm::scan_pattern_direct(apex_handle, apex_base,
			"\xF3\x0F\x10\x91\x00\x00\x00\x00\x48\x8D\x04\x40", "xxxx????xxxx", 12);

		if (temp_address == 0)
		{
	#ifdef DEBUG
			// Log an error message if the bullet muzzle pattern is not found.
			LOG("[-] failed to find bullet dwMuzzle\n");
	#endif
			goto cleanup;
		}

		// Read the muzzle value from the memory.
		temp_address = temp_address + 0x04;
		m_dwMuzzle = vm::read_i32(apex_handle, temp_address) - 0x4;

		// Check if the muzzle value is valid.
		if (m_dwMuzzle == (DWORD)-0x4)
		{
	#ifdef DEBUG
			// Log an error message if the muzzle value is invalid.
			LOG("[-] failed to find bullet dwMuzzle\n");
	#endif
			goto cleanup;
		}

		// Scan the memory pattern to find the visible time.
		temp_address = vm::scan_pattern_direct(apex_handle, apex_base,
			"\x48\x8B\xCE\x00\x00\x00\x00\x00\x84\xC0\x0F\x84\xBA\x00\x00\x00", "xxx?????xxxxxxxx", 16);
		
		if (temp_address == 0)
		{
	#ifdef DEBUG
			// Log an error message if the visible time pattern is not found.
			LOG("[-] failed to find dwVisibleTime\n");
	#endif
			goto cleanup;
		}

		// Read the visible time value from the memory.
		temp_address = temp_address + 0x10;
		m_dwVisibleTime = vm::read_i32(apex_handle, temp_address + 0x4);
		if (m_dwVisibleTime == 0)
		{
	#ifdef DEBUG
			// Log an error message if the visible time value is invalid.
			LOG("[-] failed to find m_dwVisibleTime\n");
	#endif
			goto cleanup;
		}
	}


	// Check if netvar_status is initialized. If not, dump the netvars.
	if (netvar_status == 0)
	{
		netvar_status = dump_netvars(m_GetAllClasses);

		if (netvar_status == 0)
		{
	#ifdef DEBUG
		        // Log an error message if netvars dumping fails.
			LOG("[-] failed to get netvars\n");
	#endif
			goto cleanup;
		}
	}

#ifdef DEBUG
	// Log various debug information.
	LOG("[+] IClientEntityList: %lx\n", IClientEntityList - apex_base);
	LOG("[+] dwLocalPlayer: %lx\n", C_BasePlayer - apex_base);
	LOG("[+] IInputSystem: %lx\n", IInputSystem - apex_base);
	LOG("[+] m_GetAllClasses: %lx\n", m_GetAllClasses - apex_base);
	LOG("[+] sensitivity: %lx\n", sensitivity - apex_base);
	LOG("[+] dwBulletSpeed: %x\n", (DWORD)m_dwBulletSpeed);
	LOG("[+] dwBulletGravity: %x\n", (DWORD)m_dwBulletGravity);
	LOG("[+] dwMuzzle: %x\n", m_dwMuzzle);
	LOG("[+] dwVisibleTime: %x\n", m_dwVisibleTime);
	LOG("[+] m_iHealth: %x\n", m_iHealth);
	LOG("[+] m_iViewAngles: %x\n", m_iViewAngles);
	LOG("[+] m_bZooming: %x\n", m_bZooming);
	LOG("[+] m_lifeState: %x\n", m_lifeState);
	LOG("[+] m_iCameraAngles: %x\n", m_iCameraAngles);
	LOG("[+] m_iTeamNum: %x\n", m_iTeamNum);
	LOG("[+] m_iName: %x\n", m_iName);
	LOG("[+] m_vecAbsOrigin: %x\n", m_vecAbsOrigin);
	LOG("[+] m_iWeapon: %x\n", m_iWeapon);
	LOG("[+] m_iBoneMatrix: %x\n", m_iBoneMatrix);
	LOG("[+] r5apex.exe is running\n");
#endif

	return 1;
cleanup:
	// Cleanup process if any of the initialization steps fail.
	if (apex_handle)
	{
		vm::close(apex_handle);
		apex_handle = 0;
	}
	return 0;
}

// This section is an alternative initialization method for the Apex cheat.
#else
static BOOL apex::initialize(void)
{
	// Initialize base addresses and temporary address variables.
	QWORD apex_base = 0;
	PVOID apex_base_dump = 0;
	QWORD temp_address = 0;

	// Check if the Apex game process is already running.
	if (apex_handle)
	{
		if (vm::running(apex_handle))
		{
			return 1;
		}
		apex_handle = 0;
	}

	// Open the Apex game process.
	apex_handle = vm::open_process("r5apex.exe");
	if (!apex_handle)
	{
#ifdef DEBUG
		// Log an error message if the Apex game process is not found.
		LOG("[-] r5apex.exe process not found\n");
#endif
		return 0;
	}

	// Get the base address of the Apex game process.
	apex_base = vm::get_module(apex_handle, 0);
	if (apex_base == 0)
	{
#ifdef DEBUG
		// Log an error message if the base address is not found.
		LOG("[-] r5apex.exe base address not found\n");
#endif
		goto cleanup;
	}

    	// Dump the module (code sections only) of the Apex game process.
	apex_base_dump = vm::dump_module(apex_handle, apex_base, VM_MODULE_TYPE::CodeSectionsOnly);
	if (apex_base_dump == 0)
	{
#ifdef DEBUG
		// Log an error message if the module dump fails.
		LOG("[-] r5apex.exe base dump failed\n");
#endif
		goto cleanup;
	}

	// ... (similar pattern scanning and address retrieval processes for various game parameters)
	IClientEntityList = vm::scan_pattern(apex_base_dump, "\x4C\x8B\x15\x00\x00\x00\x00\x33\xF6", "xxx????xx", 9);
	if (IClientEntityList == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find IClientEntityList\n");
#endif
		goto cleanup2;
	}

	IClientEntityList = vm::get_relative_address(apex_handle, IClientEntityList, 3, 7);
	IClientEntityList = IClientEntityList + 0x08;
	if (IClientEntityList == (QWORD)0x08)
	{
#ifdef DEBUG
		LOG("[-] failed to find IClientEntityList\n");
#endif
		goto cleanup2;
	}

	C_BasePlayer = vm::scan_pattern(apex_base_dump, "\x89\x41\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0", "xxxxxx????xxx", 13);
	if (C_BasePlayer == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find dwLocalPlayer\n");
#endif
		goto cleanup2;
	}

	C_BasePlayer = C_BasePlayer + 0x03;
	C_BasePlayer = vm::get_relative_address(apex_handle, C_BasePlayer, 3, 7);
	if (C_BasePlayer == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find dwLocalPlayer\n");
#endif
		goto cleanup2;
	}

	IInputSystem = vm::scan_pattern(apex_base_dump,
		"\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x4C\x24\x20\xBA\x01\x00\x00\x00\xC7", "xxx????xxxxxxxxxxx", 18);

	if (IInputSystem == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find IInputSystem\n");
#endif
		goto cleanup2;
	}

	IInputSystem = vm::get_relative_address(apex_handle, IInputSystem, 3, 7);
	if (IInputSystem == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find IInputSystem\n");
#endif
		goto cleanup2;
	}

	IInputSystem = IInputSystem - 0x10;

	m_GetAllClasses = vm::scan_pattern(apex_base_dump,
		"\x48\x8B\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x74\x24\x20", "xxx????xxxxxxxxxxxxxx", 21);

	if (m_GetAllClasses == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find m_GetAllClasses\n");
#endif
		goto cleanup2;
	}

	m_GetAllClasses = vm::get_relative_address(apex_handle, m_GetAllClasses, 3, 7);
	m_GetAllClasses = vm::read_i64(apex_handle, m_GetAllClasses);
	if (m_GetAllClasses == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find m_GetAllClasses\n");
#endif
		goto cleanup2;
	}

	sensitivity = vm::scan_pattern(apex_base_dump,
		"\x48\x8B\x05\x00\x00\x00\x00\xF3\x0F\x10\x3D\x00\x00\x00\x00\xF3\x0F\x10\x70\x68", "xxx????xxxx????xxxxx", 20);
	
	if (sensitivity == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find sensitivity\n");
#endif
		goto cleanup2;
	}

	sensitivity = vm::get_relative_address(apex_handle, sensitivity, 3, 7);
	sensitivity = vm::read_i64(apex_handle, sensitivity);
	if (sensitivity == 0)
	{
#ifdef DEBUG
		LOG("[-] failed to find sensitivity\n");
#endif
		goto cleanup2;
	}

    	// Check if netvar_status is initialized. If not, proceed with the initialization process.
	if (netvar_status == 0)
	{
		// ... (similar pattern scanning and address retrieval processes for bullet speed, gravity, muzzle, etc.)
		temp_address = vm::scan_pattern(apex_base_dump, "\x75\x0F\xF3\x44\x0F\x10\xBF", "xxxxxxx", 7);
		if (temp_address == 0)
		{
	#ifdef DEBUG
			LOG("[-] failed to find bullet speed / gravity\n");
	#endif
			goto cleanup2;
		}

		m_dwBulletGravity = vm::read_i32(apex_handle, temp_address + 0x02 + 0x05);
		m_dwBulletSpeed = vm::read_i32(apex_handle, temp_address - 0x6D + 0x04);

		if (m_dwBulletGravity == 0 || m_dwBulletSpeed == 0)
		{
	#ifdef DEBUG
			LOG("[-] failed to find bullet m_dwBulletGravity/m_dwBulletSpeed\n");
	#endif
			goto cleanup2;
		}


		temp_address = vm::scan_pattern(apex_base_dump,
			"\xF3\x0F\x10\x91\x00\x00\x00\x00\x48\x8D\x04\x40", "xxxx????xxxx", 12);

		if (temp_address == 0)
		{
	#ifdef DEBUG
			LOG("[-] failed to find bullet dwMuzzle\n");
	#endif
			goto cleanup2;
		}

		temp_address = temp_address + 0x04;
		m_dwMuzzle = vm::read_i32(apex_handle, temp_address) - 0x4;

		if (m_dwMuzzle == (DWORD)-0x4)
		{
	#ifdef DEBUG
			LOG("[-] failed to find bullet dwMuzzle\n");
	#endif
			goto cleanup2;
		}
        	// Scan the memory pattern to find the visible time.
		temp_address = vm::scan_pattern(apex_base_dump,
			"\x48\x8B\xCE\x00\x00\x00\x00\x00\x84\xC0\x0F\x84\xBA\x00\x00\x00", "xxx?????xxxxxxxx", 16);
		
		if (temp_address == 0)
		{
	#ifdef DEBUG
			// Log an error message if the visible time pattern is not found.
			LOG("[-] failed to find dwVisibleTime\n");
	#endif
			goto cleanup2;
		}

		// Read the visible time value from the memory.
		temp_address = temp_address + 0x10;
		m_dwVisibleTime = vm::read_i32(apex_handle, temp_address + 0x4);
		if (m_dwVisibleTime == 0)
		{
	#ifdef DEBUG
			// Log an error message if the visible time value is invalid.
			LOG("[-] failed to find m_dwVisibleTime\n");
	#endif
			goto cleanup2;
		}
	}


	// Free the dumped module and reset the pointer.
	vm::free_module(apex_base_dump);
	apex_base_dump = 0;

	// Check if netvar_status is initialized. If not, proceed with the initialization process.
	if (netvar_status == 0)
	{
		netvar_status = dump_netvars(m_GetAllClasses);

		if (netvar_status == 0)
		{
	#ifdef DEBUG
		        // Log an error message if netvars cannot be retrieved.
			LOG("[-] failed to get netvars\n");
	#endif
			goto cleanup;
		}
	}
	
// Log various game parameters for debugging purposes.
#ifdef DEBUG
	LOG("[+] IClientEntityList: %lx\n", IClientEntityList - apex_base);
	LOG("[+] dwLocalPlayer: %lx\n", C_BasePlayer - apex_base);
	LOG("[+] IInputSystem: %lx\n", IInputSystem - apex_base);
	LOG("[+] m_GetAllClasses: %lx\n", m_GetAllClasses - apex_base);
	LOG("[+] sensitivity: %lx\n", sensitivity - apex_base);
	LOG("[+] dwBulletSpeed: %x\n", (DWORD)m_dwBulletSpeed);
	LOG("[+] dwBulletGravity: %x\n", (DWORD)m_dwBulletGravity);
	LOG("[+] dwMuzzle: %x\n", m_dwMuzzle);
	LOG("[+] dwVisibleTime: %x\n", m_dwVisibleTime);
	LOG("[+] m_iHealth: %x\n", m_iHealth);
	LOG("[+] m_iViewAngles: %x\n", m_iViewAngles);
	LOG("[+] m_bZooming: %x\n", m_bZooming);
	LOG("[+] m_lifeState: %x\n", m_lifeState);
	LOG("[+] m_iCameraAngles: %x\n", m_iCameraAngles);
	LOG("[+] m_iTeamNum: %x\n", m_iTeamNum);
	LOG("[+] m_iName: %x\n", m_iName);
	LOG("[+] m_vecAbsOrigin: %x\n", m_vecAbsOrigin);
	LOG("[+] m_iWeapon: %x\n", m_iWeapon);
	LOG("[+] m_iBoneMatrix: %x\n", m_iBoneMatrix);
	LOG("[+] r5apex.exe is running\n");
#endif

	return 1;

// Cleanup sections to free resources and handle errors.
cleanup2:
	if (apex_base_dump)
	{
		vm::free_module(apex_base_dump);
		apex_base_dump = 0;
	}

cleanup:
	if (apex_handle)
	{
		vm::close(apex_handle);
		apex_handle = 0;
	}
	return 0;
}
#endif

// Function to dump a specific table from the game's memory.
static int apex::dump_table(QWORD table, const char *name)
{
	for (DWORD i = 0; i < vm::read_i32(apex_handle, table + 0x10); i++) {
		

		QWORD recv_prop = vm::read_i64(apex_handle, table + 0x8);
		if (!recv_prop) {
			continue;
		}

		recv_prop = vm::read_i64(apex_handle, recv_prop + 0x8 * i);
		char recv_prop_name[260];
		{
			QWORD name_ptr = vm::read_i64(apex_handle, recv_prop + 0x28);
			vm::read(apex_handle, name_ptr, recv_prop_name, 260);
		}

		if (!strcmpi_imp(recv_prop_name, name)) {
			return vm::read_i32(apex_handle, recv_prop + 0x4);
		}
	}
	return 0;
}

// Function to dump netvars (network variables) from the game's memory.
static BOOL apex::dump_netvars(QWORD GetAllClassesAddress)
{
	int counter = 0;
	QWORD entry = GetAllClassesAddress;
	while (entry) {
		// ... (process to read and retrieve various netvars from the game's memory)
		QWORD recv_table = vm::read_i64(apex_handle, entry + 0x18);
		QWORD recv_name  = vm::read_i64(apex_handle, recv_table + 0x4C8);

		char name[260];
		vm::read( apex_handle, recv_name, name, 260 );
		
		if (!strcmpi_imp(name, "DT_Player")) {
			m_iHealth = dump_table(recv_table, "m_iHealth");
			if (m_iHealth == 0)
			{
				break;
			}

			m_iViewAngles = dump_table(recv_table, "m_ammoPoolCapacity");
			if (m_iViewAngles == 0)
			{
				break;
			}
			m_iViewAngles -= 0x14;

			m_bZooming = dump_table(recv_table, "m_bZooming");
			if (m_bZooming == 0)
			{
				break;
			}

			m_lifeState = dump_table(recv_table, "m_lifeState");
			if (m_lifeState == 0)
			{
				break;
			}

			m_iCameraAngles = dump_table(recv_table, "m_zoomFullStartTime");
			if (m_iCameraAngles == 0)
			{
				break;
			}
			m_iCameraAngles += 0x2EC;
			counter++;
		}

		if (!strcmpi_imp(name, "DT_BaseEntity")) {
			m_iTeamNum = dump_table(recv_table, "m_iTeamNum");
			if (m_iTeamNum == 0)
			{
				break;
			}
			m_iName = dump_table(recv_table, "m_iName");
			if (m_iName == 0)
			{
				break;
			}

			m_vecAbsOrigin = 0x014c;
			counter++;
		}

		if (!strcmpi_imp(name, "DT_BaseCombatCharacter")) {
			m_iWeapon = dump_table(recv_table, "m_latestPrimaryWeapons");
			if (m_iWeapon == 0)
			{
				break;
			}
			counter++;
		}

		if (!strcmpi_imp(name, "DT_BaseAnimating")) {
			m_iBoneMatrix = dump_table(recv_table, "m_nForceBone");
			if (m_iBoneMatrix == 0)
			{
				break;
			}
			m_iBoneMatrix = m_iBoneMatrix + 0x50 - 0x8;
			counter++;
		}

		if (!strcmpi_imp(name, "DT_WeaponX")) {
			m_playerData = dump_table(recv_table, "m_playerData");
			if (m_playerData == 0)
			{
				break;
			}
			counter++;
		}

		entry = vm::read_i64(apex_handle, entry + 0x20);
	}
	return counter == 5;
}

