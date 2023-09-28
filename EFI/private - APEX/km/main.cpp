// Include the shared header file from the apex directory.
#include "../../../apex/shared/shared.h"


//
// disables warnings for unused functions
//
#pragma warning (disable : 4505)



//
// float support for drivers
//
// Global variable to support floating-point operations for drivers.
int _fltused;

//
// used by vm.cpp
//
// Global variables used by vm.cpp to define memory range boundaries.
QWORD g_memory_range_low  = 0;
QWORD g_memory_range_high = 0;

//
// dynamic imports, used globally
//
// External function declarations (dynamic imports) for interacting with the Windows Kernel.
// These functions are likely imported from the Windows Kernel or other system libraries.
extern "C" {
	// ULONG (__cdecl *_DbgPrintEx)(ULONG, ULONG, PCSTR, ...);
	QWORD _KeAcquireSpinLockAtDpcLevel;
	QWORD _KeReleaseSpinLockFromDpcLevel;
	QWORD _IofCompleteRequest;
	QWORD _IoReleaseRemoveLockEx;
	PCSTR ( *_PsGetProcessImageFileName)(PEPROCESS);
	BOOLEAN ( *_PsGetProcessExitProcessCalled)(PEPROCESS);
	PVOID ( *_PsGetProcessPeb)(PEPROCESS);
	PVOID ( *_PsGetProcessWow64Process)(PEPROCESS);
	HANDLE ( *_PsGetProcessId)(_In_ PEPROCESS);
	PVOID ( *_MmMapIoSpace)(PHYSICAL_ADDRESS, SIZE_T, MEMORY_CACHING_TYPE);
	VOID ( *_MmUnmapIoSpace)(PVOID, SIZE_T);
	PPHYSICAL_MEMORY_RANGE (*_MmGetPhysicalMemoryRanges)(void);
	PVOID (*_ExAllocatePoolWithTag)(POOL_TYPE, SIZE_T, ULONG);
	VOID ( *_ExFreePoolWithTag)( PVOID, ULONG );
	NTSTATUS ( *_KeDelayExecutionThread)(KPROCESSOR_MODE, BOOLEAN, PLARGE_INTEGER);
	VOID ( *_RtlInitUnicodeString)(PUNICODE_STRING, PCWSTR);
	NTSTATUS ( *_ObReferenceObjectByName)(PUNICODE_STRING, ULONG, PACCESS_STATE, ACCESS_MASK,
		POBJECT_TYPE, KPROCESSOR_MODE, PVOID, PVOID*);
	LONG_PTR ( *_ObfDereferenceObject)(PVOID);
	POBJECT_TYPE* _IoDriverObjectType;
	PEPROCESS _PsInitialSystemProcess;

	QWORD g_system_previous_ms;

	__int64 (__fastcall *MiGetPteAddress)(unsigned __int64 a1);

	//
	// tested and stable approach, we can now fix IAT for them
	//
	
	BOOLEAN (*_KeInsertQueueDpc)(
	    _Inout_ PRKDPC Dpc,
	    _In_opt_ PVOID SystemArgument1,
	    _In_opt_ __drv_aliasesMem PVOID SystemArgument2
	    );

	VOID (*_KeSetTargetProcessorDpc)(
	    _Inout_ PRKDPC Dpc,
	    _In_ CCHAR Number
	    );

	VOID (*_KeInitializeDpc)(
	    _Out_ __drv_aliasesMem PRKDPC Dpc,
	    _In_ PKDEFERRED_ROUTINE DeferredRoutine,
	    _In_opt_ __drv_aliasesMem PVOID DeferredContext
	    );

	PHYSICAL_ADDRESS
	(*_MmGetPhysicalAddress)(
	    _In_ PVOID BaseAddress
	    );

	BOOLEAN
	(*_MmIsAddressValid)(
	    _In_ PVOID VirtualAddress
	    );

	ULONG
	(*_KeQueryTimeIncrement)(
	    VOID
	    );

	CCHAR _KeNumberProcessors;
};

// Data structures related to the PE (Portable Executable) format.
// These structures are used to parse and manipulate PE files in memory.

#ifdef _KERNEL_MODE
// DOS .EXE header structure.
typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
    WORD   e_magic;                     // Magic number
    WORD   e_cblp;                      // Bytes on last page of file
    WORD   e_cp;                        // Pages in file
    WORD   e_crlc;                      // Relocations
    WORD   e_cparhdr;                   // Size of header in paragraphs
    WORD   e_minalloc;                  // Minimum extra paragraphs needed
    WORD   e_maxalloc;                  // Maximum extra paragraphs needed
    WORD   e_ss;                        // Initial (relative) SS value
    WORD   e_sp;                        // Initial SP value
    WORD   e_csum;                      // Checksum
    WORD   e_ip;                        // Initial IP value
    WORD   e_cs;                        // Initial (relative) CS value
    WORD   e_lfarlc;                    // File address of relocation table
    WORD   e_ovno;                      // Overlay number
    WORD   e_res[4];                    // Reserved words
    WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
    WORD   e_oeminfo;                   // OEM information; e_oemid specific
    WORD   e_res2[10];                  // Reserved words
    LONG   e_lfanew;                    // File address of new exe header
  } IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

// PE file header structure.
typedef struct _IMAGE_FILE_HEADER {
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

// Data directory structure within the PE format.
typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;
    DWORD   Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

// Optional header structure for 64-bit PE format.
typedef struct _IMAGE_OPTIONAL_HEADER64 {
    WORD        Magic;
    BYTE        MajorLinkerVersion;
    BYTE        MinorLinkerVersion;
    DWORD       SizeOfCode;
    DWORD       SizeOfInitializedData;
    DWORD       SizeOfUninitializedData;
    DWORD       AddressOfEntryPoint;
    DWORD       BaseOfCode;
    ULONGLONG   ImageBase;
    DWORD       SectionAlignment;
    DWORD       FileAlignment;
    WORD        MajorOperatingSystemVersion;
    WORD        MinorOperatingSystemVersion;
    WORD        MajorImageVersion;
    WORD        MinorImageVersion;
    WORD        MajorSubsystemVersion;
    WORD        MinorSubsystemVersion;
    DWORD       Win32VersionValue;
    DWORD       SizeOfImage;
    DWORD       SizeOfHeaders;
    DWORD       CheckSum;
    WORD        Subsystem;
    WORD        DllCharacteristics;
    ULONGLONG   SizeOfStackReserve;
    ULONGLONG   SizeOfStackCommit;
    ULONGLONG   SizeOfHeapReserve;
    ULONGLONG   SizeOfHeapCommit;
    DWORD       LoaderFlags;
    DWORD       NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[16];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

// NT headers structure for 64-bit PE format.
typedef struct _IMAGE_NT_HEADERS64 {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

// Section header structure within the PE format.
typedef struct _IMAGE_SECTION_HEADER {
    BYTE    Name[8];
    union {
            DWORD   PhysicalAddress;
            DWORD   VirtualSize;
    } Misc;
    DWORD   VirtualAddress;
    DWORD   SizeOfRawData;
    DWORD   PointerToRawData;
    DWORD   PointerToRelocations;
    DWORD   PointerToLinenumbers;
    WORD    NumberOfRelocations;
    WORD    NumberOfLinenumbers;
    DWORD   Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;
#endif

// Disable compiler warning for nameless struct/union.
#pragma warning(disable : 4201)

// Structure to represent mouse input data.
typedef struct _MOUSE_INPUT_DATA {
	USHORT UnitId;
	USHORT Flags;
	union {
	ULONG Buttons;
	struct {
	USHORT ButtonFlags;
	USHORT ButtonData;
	};
	};
	ULONG  RawButtons;
	LONG   LastX;
	LONG   LastY;
	ULONG  ExtraInformation;
} MOUSE_INPUT_DATA, *PMOUSE_INPUT_DATA;

// Callback function type for mouse class service.
typedef VOID
(*MouseClassServiceCallbackFn)(
	PDEVICE_OBJECT DeviceObject,
	PMOUSE_INPUT_DATA InputDataStart,
	PMOUSE_INPUT_DATA InputDataEnd,
	PULONG InputDataConsumed
);

// Structure to represent a mouse object.
typedef struct _MOUSE_OBJECT
{
	PDEVICE_OBJECT              mouse_device;
	MouseClassServiceCallbackFn service_callback;
	BOOL                        use_mouse;
	QWORD                       target_routine;
} MOUSE_OBJECT, * PMOUSE_OBJECT;

// Global mouse object instance.
MOUSE_OBJECT gMouseObject = {0,0,0,0};

// Function to make the current thread sleep for a specified duration.
static void NtSleep(DWORD milliseconds);

// Namespace for mouse-related functions.
namespace mouse
{
	static void move(long x, long y, unsigned short button_flags);
	static BOOL open(void);
}

// Namespace for input-related functions.
namespace input
{
	void mouse_move(int x, int y)
	{
		mouse::move(x, y, 0);
		// apex::input::mouse_move(x, y);
	}
	/*
	void mouse1_down(void)
	{
		// mouse::move(0, 0, 0x01);
	}

	void mouse1_up(void)
	{
		// mouse::move(0, 0, 0x02);
	}
	*/
}

namespace config
{
	DWORD aimbot_button           = 111;
	float aimbot_fov              = 10.0f;
	float aimbot_smooth           = 24.0f;
	BOOL  aimbot_visibility_check = 1;
	BOOL  visuals_enabled         = 1;

	//
	// time_begin/time_end/licence_type used before in EC/W3
	//
	DWORD time_begin   = 0;
	DWORD time_end     = 0;
	DWORD licence_type = 0;
	DWORD invalid_hwid = 0;
}

// Various utility functions.
static QWORD get_ntoskrnl_data_location(QWORD ntoskrnl);
static void  clear_image_traces(QWORD efi_image_base);
static void  get_physical_memory_ranges(QWORD *low, QWORD *high);
static QWORD FindPattern(QWORD module, BYTE *bMask, CHAR *szMask, QWORD len, int counter);
static QWORD GetSystemBaseAddress(PDRIVER_OBJECT DriverObject, const unsigned short* driver_name);
static QWORD GetProcAddressQ(QWORD base, PCSTR export_name);

//
// it never should return zero, in case it does because of potential typo, loop forever
//

// Macro to ensure that the address of an exported function is valid.
#define EXPORT_ADDRESS2(var, name) \
	*(QWORD*)&var = GetProcAddressQ((QWORD)ntoskrnl, name); \
	if (var == 0) while (1) ; \

// System thread function (commented out).
/*
NTSTATUS system_thread(void)
{
	//
	// wait until mouse is available
	//
	while (mouse::open() == 0)
	{
		NtSleep(1);
	}

	features::reset();

	//
	// loop forever
	//
	while (1)
	{
		NtSleep(1);
		apex_legends::run();
	}

	return STATUS_SUCCESS;
}*/

// DPC (Deferred Procedure Call) object.
PKDPC dpc_object;

// Include intrinsic functions header.
#include <intrin.h>

// Enumeration for kernel objects.
enum _KOBJECTS
{
	// Various kernel object types.
    EventNotificationObject = 0,
    EventSynchronizationObject = 1,
    MutantObject = 2,
    ProcessObject = 3,
    QueueObject = 4,
    SemaphoreObject = 5,
    ThreadObject = 6,
    GateObject = 7,
    TimerNotificationObject = 8,
    TimerSynchronizationObject = 9,
    Spare2Object = 10,
    Spare3Object = 11,
    Spare4Object = 12,
    Spare5Object = 13,
    Spare6Object = 14,
    Spare7Object = 15,
    Spare8Object = 16,
    ProfileCallbackObject = 17,
    ApcObject = 18,
    DpcObject = 19,
    DeviceQueueObject = 20,
    PriQueueObject = 21,
    InterruptObject = 22,
    ProfileObject = 23,
    Timer2NotificationObject = 24,
    Timer2SynchronizationObject = 25,
    ThreadedDpcObject = 26,
    MaximumKernelObject = 27
}; 

// DPC routine function.
void DpcRoutine(void)
{
	//
	// if we are explorer.exe, lets find mouse
	//
	if (gMouseObject.use_mouse == 0)
	{
		PEPROCESS process = *(PEPROCESS*)(__readgsqword(0x188) + 0xB8);
		PCSTR image_name = _PsGetProcessImageFileName(process);
		if (image_name && *(QWORD*)(image_name) == 0x7265726f6c707865)
		{
			mouse::open();
		}
	}

	if (gMouseObject.use_mouse)
	{
		PEPROCESS process = *(PEPROCESS*)(__readgsqword(0x188) + 0xB8);
		PCSTR image_name = _PsGetProcessImageFileName(process);
		QWORD cr3 = *(QWORD*)((QWORD)process + 0x28);

		if (cr3 == __readcr3())
		{
			//
			// if we are Apex Legends
			//
			if (image_name && *(QWORD*)image_name == 0x652e786570613572)
			{
				apex_legends::run();
			}
		}
	}

	ULONG num = KeGetCurrentProcessorNumber();
	UCHAR next_num = 0;

	for (CCHAR i = 0; i < _KeNumberProcessors; i++)
	{
		if ((CCHAR)num != i)
		{
			next_num = i;
			break;
		}
	}

	_KeSetTargetProcessorDpc(dpc_object, ((CHAR)next_num));
	dpc_object->Importance = LowImportance;
	dpc_object->Type = 19;
	_KeInsertQueueDpc(dpc_object, 0, 0);
}

// Entry point for the driver.
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, void *efi_loader_base, void *efi_cfg)
{
	// Cast the efi_cfg pointer to an integer pointer and read configuration values.
	int *cfg = (int*)efi_cfg;
	config::aimbot_button = cfg[1];
	config::aimbot_fov = (float)cfg[2];
	config::aimbot_smooth = (float)cfg[3];
	config::aimbot_visibility_check = cfg[4];
	config::visuals_enabled = cfg[5];
	config::time_begin = cfg[7];
	config::time_end = cfg[8];
	config::licence_type = cfg[9];
	config::invalid_hwid = cfg[10];
    // ... (other configuration values are set similarly)

	// Get the base address of the ntoskrnl.exe module.
	QWORD ntoskrnl = GetSystemBaseAddress(DriverObject, L"ntoskrnl.exe");
	if (ntoskrnl == 0)
	{
		//
		// crash (PAGE_FAULT), ntoskrnl.exe not found
		//
		// Crash the system if ntoskrnl.exe is not found.

		*(int*)(0x10A0) = 0;
	}

	QWORD JMP_QWORD_PTR_RDX = FindPattern(ntoskrnl, (BYTE*)"\xFF\x22", (CHAR*)"xx", 2, 1);
	if (JMP_QWORD_PTR_RDX == 0)
	{
		//
		// crash (PAGE_FAULT), JMP_QWORD_PTR_RCX not found
		//
		*(int*)(0x10A0) = 0;
	}

	QWORD KiGlobalBuffer = get_ntoskrnl_data_location(ntoskrnl);
	if (KiGlobalBuffer == 0)
	{
		//
		// crash (PAGE_FAULT), KiGlobalBuffer not found
		//
		*(int*)(0x10A0) = 0;
	}

	// Initialize global variables and structures.
	QWORD MmUnlockPreChargedPagedPoolAddress = 0;
	QWORD processor_count=0;

	// EXPORT_ADDRESS2(_DbgPrintEx, "DbgPrintEx");
	EXPORT_ADDRESS2(MmUnlockPreChargedPagedPoolAddress, "MmUnlockPreChargedPagedPool");
	EXPORT_ADDRESS2(_KeAcquireSpinLockAtDpcLevel, "KeAcquireSpinLockAtDpcLevel");
	EXPORT_ADDRESS2(_KeReleaseSpinLockFromDpcLevel, "KeReleaseSpinLockFromDpcLevel");
	EXPORT_ADDRESS2(_IofCompleteRequest, "IofCompleteRequest");
	EXPORT_ADDRESS2(_IoReleaseRemoveLockEx, "IoReleaseRemoveLockEx");
	EXPORT_ADDRESS2(_PsGetProcessImageFileName, "PsGetProcessImageFileName");
	EXPORT_ADDRESS2(_PsGetProcessExitProcessCalled, "PsGetProcessExitProcessCalled");
	EXPORT_ADDRESS2(_PsGetProcessPeb, "PsGetProcessPeb");
	EXPORT_ADDRESS2(_PsGetProcessWow64Process, "PsGetProcessWow64Process");
	EXPORT_ADDRESS2(_PsGetProcessId, "PsGetProcessId");
	EXPORT_ADDRESS2(_MmMapIoSpace, "MmMapIoSpace");
	EXPORT_ADDRESS2(_MmUnmapIoSpace, "MmUnmapIoSpace");
	EXPORT_ADDRESS2(_MmGetPhysicalMemoryRanges, "MmGetPhysicalMemoryRanges");
	EXPORT_ADDRESS2(_ExAllocatePoolWithTag, "ExAllocatePoolWithTag");
	EXPORT_ADDRESS2(_ExFreePoolWithTag, "ExFreePoolWithTag");
	EXPORT_ADDRESS2(_KeDelayExecutionThread, "KeDelayExecutionThread");
	EXPORT_ADDRESS2(_RtlInitUnicodeString, "RtlInitUnicodeString");
	EXPORT_ADDRESS2(_ObReferenceObjectByName, "ObReferenceObjectByName");
	EXPORT_ADDRESS2(_ObfDereferenceObject, "ObfDereferenceObject");
	EXPORT_ADDRESS2(_IoDriverObjectType, "IoDriverObjectType");
	EXPORT_ADDRESS2(_PsInitialSystemProcess, "PsInitialSystemProcess");
	EXPORT_ADDRESS2(_KeInsertQueueDpc, "KeInsertQueueDpc");
	EXPORT_ADDRESS2(_KeSetTargetProcessorDpc, "KeSetTargetProcessorDpc");
	EXPORT_ADDRESS2(_KeInitializeDpc, "KeInitializeDpc");
	EXPORT_ADDRESS2(_MmGetPhysicalAddress, "MmGetPhysicalAddress");
	EXPORT_ADDRESS2(_MmIsAddressValid, "MmIsAddressValid");
	EXPORT_ADDRESS2(_KeQueryTimeIncrement, "KeQueryTimeIncrement");
	EXPORT_ADDRESS2(processor_count, "KeNumberProcessors");
	_KeNumberProcessors = *(CCHAR*)processor_count;

	_PsInitialSystemProcess = (PEPROCESS)*(QWORD*)_PsInitialSystemProcess;

	g_system_previous_ms = 0;

	*(QWORD*)&MiGetPteAddress = (QWORD)(*(int*)(MmUnlockPreChargedPagedPoolAddress + 8) + MmUnlockPreChargedPagedPoolAddress + 12);


	QWORD km_base = 0;
	{
		IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)efi_loader_base;
		IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);
		km_base = (QWORD)efi_loader_base + nt->OptionalHeader.SizeOfImage;
		km_base = km_base - 0x1000;
	}
	km_base = (QWORD)PAGE_ALIGN(km_base);


	clear_image_traces((QWORD)efi_loader_base);
	get_physical_memory_ranges(&g_memory_range_low, &g_memory_range_high);
	apex::reset_globals();
	features::reset();


	*(QWORD*)(KiGlobalBuffer + 0x00) = (QWORD)DpcRoutine;
	dpc_object = (PKDPC)(KiGlobalBuffer + 0x68);
	memset(dpc_object, 0, sizeof(KDPC));

	 

	_KeInitializeDpc(dpc_object, (PKDEFERRED_ROUTINE)JMP_QWORD_PTR_RDX, (PVOID)KiGlobalBuffer);

	ULONG num = KeGetCurrentProcessorNumber();
	UCHAR next_num = 0;

	for (CCHAR i = 0; i < _KeNumberProcessors; i++)
	{
		if ((CCHAR)num != i)
		{
			next_num = i;
			break;
		}
	}

	_KeSetTargetProcessorDpc(dpc_object, ((CHAR)next_num));
	dpc_object->Importance = LowImportance;
	dpc_object->Type = 19;
	_KeInsertQueueDpc(dpc_object, 0, 0);

	// Return success status.
	return STATUS_SUCCESS;
}

// Function to get the address of a specified exported function from a module.
static QWORD GetProcAddressQ(QWORD base, PCSTR export_name)
{
    // Implementation to find the address of the exported function.
	QWORD a0;
	DWORD a1[4];
	
	
	a0 = base + *(USHORT*)(base + 0x3C);
	a0 = base + *(DWORD*)(a0 + 0x88);
	a1[0] = *(DWORD*)(a0 + 0x18);
	a1[1] = *(DWORD*)(a0 + 0x1C);
	a1[2] = *(DWORD*)(a0 + 0x20);
	a1[3] = *(DWORD*)(a0 + 0x24);
	while (a1[0]--) {
		
		a0 = base + *(DWORD*)(base + a1[2] + (a1[0] * 4));
		if (strcmpi_imp((PCSTR)a0, export_name) == 0) {
			
			return (base + *(DWORD*)(base + a1[1] + (*(USHORT*)(base + a1[3] + (a1[0] * 2)) * 4)));
		}	
	}
	return 0;
}

// Function to clear traces of the image from memory.
static void clear_image_traces(QWORD efi_image_base)
{
	// Implementation to clear the image traces.
	QWORD km_base = 0;
	{
		IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)efi_image_base;
		IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);


		km_base = efi_image_base + nt->OptionalHeader.SizeOfImage;
		for (int i = nt->OptionalHeader.SizeOfImage; i--;)
			((unsigned char*)efi_image_base)[i] = 0;
	}

	{
		IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)km_base;
		IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64*)((char*)dos + dos->e_lfanew);

		for (int i = nt->OptionalHeader.SizeOfHeaders; i--;)
			((unsigned char*)km_base)[i] = 0;
	}
}

// Function to get the physical memory ranges.
static void get_physical_memory_ranges(QWORD *low, QWORD *high)
{
	PPHYSICAL_MEMORY_RANGE memory_range = _MmGetPhysicalMemoryRanges();
	int counter=0;

	while (1)
	{
		if (memory_range[counter].BaseAddress.QuadPart == 0)
		{
			break;
		}
		counter++;
	}
	
	*low = memory_range[0].BaseAddress.QuadPart;
	*high = memory_range[counter - 1].BaseAddress.QuadPart + memory_range[counter - 1].NumberOfBytes.QuadPart;
	_ExFreePoolWithTag(memory_range, 'hPmM');
}

// Function to get the data location of ntoskrnl.
static QWORD get_ntoskrnl_data_location(QWORD ntoskrnl)
{
	// Implementation to find the data location of ntoskrnl.
	QWORD ntoskrnl_data_address = 0;

	IMAGE_DOS_HEADER *hdr = (IMAGE_DOS_HEADER*)(ntoskrnl);
	IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64*)((char*)hdr + hdr->e_lfanew);
	IMAGE_SECTION_HEADER *section =
	(IMAGE_SECTION_HEADER *)((UINT8 *)&nt->OptionalHeader +
		nt->FileHeader.SizeOfOptionalHeader);

	for (UINT16 i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
		if (*(QWORD*)section[i].Name == 0x617461642e)
		{
			ntoskrnl_data_address = (QWORD)ntoskrnl + section[i].VirtualAddress +
				section[i].Misc.VirtualSize - 0x1000;

			break;
		}
	}
	return ntoskrnl_data_address;
}

// Compares a sequence of bytes with a given mask.
static BOOLEAN bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
    // Iterate through the mask and compare bytes.
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if ((*szMask == 1 || *szMask == 'x') && *pData != *bMask)
			return 0;
	return (*szMask) == 0;
}

// Searches for a pattern in a specified memory range.
static QWORD FindPatternEx(UINT64 dwAddress, QWORD dwLen, BYTE *bMask, char * szMask)
{
    // Iterate through the memory range and use bDataCompare to find the pattern.	
	if (dwLen <= 0)
		return 0;
	for (QWORD i = 0; i < dwLen; i++)
		if (bDataCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (QWORD)(dwAddress + i);
	return 0;
}

// Searches for a pattern within the sections of a module.
static QWORD FindPattern(QWORD module, BYTE *bMask, CHAR *szMask, QWORD len, int counter)
{
	// Extract headers and sections from the module.
	ULONG_PTR ret = 0;
	PIMAGE_DOS_HEADER pidh = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);
	PIMAGE_SECTION_HEADER pish = (PIMAGE_SECTION_HEADER)((BYTE*)pinh + sizeof(IMAGE_NT_HEADERS64));
	
	// Iterate through each section and search for the pattern.
	for (USHORT sec = 0; sec < pinh->FileHeader.NumberOfSections; sec++)
	{
		
		if ((pish[sec].Characteristics & 0x00000020))
		{
			QWORD address = FindPatternEx(pish[sec].VirtualAddress + (ULONG_PTR)(module),
				pish[sec].Misc.VirtualSize - len, bMask, szMask);
 
			if (address) {
				ret = address;

				counter --;

				if (counter == 0)
					break;
			}
		}
		
	}
	return ret;
}

// Structure representing an entry in the loader data table.
#pragma warning(disable : 4201)
typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	WORD LoadCount;
	WORD TlsIndex;
	union
	{
		LIST_ENTRY HashLinks;
		struct
		{
			PVOID SectionPointer;
			ULONG CheckSum;
		};
	};
	union
	{
		ULONG TimeDateStamp;
		PVOID LoadedImports;
	};
	PVOID* EntryPointActivationContext;
	PVOID PatchInformation;
	LIST_ENTRY ForwarderLinks;
	LIST_ENTRY ServiceTagLinks;
	LIST_ENTRY StaticLinks;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

// Custom implementation of string comparison for wide strings.
inline int wcscmp_imp(const unsigned short* s1, const unsigned short* s2)
{
    // Compare two wide strings character by character.
	while (*s1 && (to_lower_imp(*s1) == to_lower_imp(*s2)))
	{
		s1++;
		s2++;
	}
	return *(const unsigned short*)s1 - *(const unsigned short*)s2;
}

// Retrieves the base address of a system module.
static QWORD GetSystemBaseAddress(PDRIVER_OBJECT DriverObject, const unsigned short* driver_name)
{
	// Iterate through the loader data table entries to find the module.
	PLDR_DATA_TABLE_ENTRY ldr = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;
	for (PLIST_ENTRY pListEntry = ldr->InLoadOrderLinks.Flink; pListEntry != &ldr->InLoadOrderLinks; pListEntry = pListEntry->Flink)
	{
		PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
		if (pEntry->BaseDllName.Buffer && wcscmp_imp(pEntry->BaseDllName.Buffer, driver_name) == 0) {
			
			return (QWORD)pEntry->DllBase;
		}
	}
	return 0;
}

// Function to initialize the mouse object by obtaining references to the MouClass and MouHID driver objects.
static BOOL mouse::open(void)
{
	// https://github.com/nbqofficial/norsefire

	// Check if the mouse object is already initialized.
	if (gMouseObject.use_mouse == 0) {
		
		// Define the name of the MouClass driver.
		UNICODE_STRING class_string;
		_RtlInitUnicodeString(&class_string, L"\\Driver\\MouClass");
		PDRIVER_OBJECT class_driver_object = NULL;
		
		// Obtain a reference to the MouClass driver object.
		NTSTATUS status = _ObReferenceObjectByName(&class_string, OBJ_CASE_INSENSITIVE, NULL, 0, *_IoDriverObjectType, KernelMode, NULL, (PVOID*)&class_driver_object);
		if (!NT_SUCCESS(status)) {
			gMouseObject.use_mouse = 0;
			return 0;
		}

		// Define the name of the MouHID driver.
		UNICODE_STRING hid_string;
		_RtlInitUnicodeString(&hid_string, L"\\Driver\\MouHID");
		PDRIVER_OBJECT hid_driver_object = NULL;
		
		// Obtain a reference to the MouHID driver object.
		status = _ObReferenceObjectByName(&hid_string, OBJ_CASE_INSENSITIVE, NULL, 0, *_IoDriverObjectType, KernelMode, NULL, (PVOID*)&hid_driver_object);
		if (!NT_SUCCESS(status))
		{
			if (class_driver_object) {
				_ObfDereferenceObject(class_driver_object);
			}
			gMouseObject.use_mouse = 0;
			return 0;
		}

        // Search for the service callback function within the device extensions.
		PVOID class_driver_base = NULL;
		PDEVICE_OBJECT hid_device_object = hid_driver_object->DeviceObject;
		while (hid_device_object && !gMouseObject.service_callback)
		{
			PDEVICE_OBJECT class_device_object = class_driver_object->DeviceObject;
			while (class_device_object && !gMouseObject.service_callback)
			{
				if (!class_device_object->NextDevice && !gMouseObject.mouse_device)
				{
					gMouseObject.mouse_device = class_device_object;
				}

                // Check the device extension for the service callback function.
				PULONG_PTR device_extension = (PULONG_PTR)hid_device_object->DeviceExtension;
				ULONG_PTR device_ext_size = ((ULONG_PTR)hid_device_object->DeviceObjectExtension - (ULONG_PTR)hid_device_object->DeviceExtension) / 4;
				class_driver_base = class_driver_object->DriverStart;
				for (ULONG_PTR i = 0; i < device_ext_size; i++)
				{
					if (device_extension[i] == (ULONG_PTR)class_device_object && device_extension[i + 1] > (ULONG_PTR)class_driver_object)
					{
						gMouseObject.service_callback = (MouseClassServiceCallbackFn)(device_extension[i + 1]);
					
						break;
					}
				}
				class_device_object = class_device_object->NextDevice;
			}
			hid_device_object = hid_device_object->AttachedDevice;
		}
	
	    // If the mouse device is not found, search for it in the device list.
		if (!gMouseObject.mouse_device)
		{
			PDEVICE_OBJECT target_device_object = class_driver_object->DeviceObject;
			while (target_device_object)
			{
				if (!target_device_object->NextDevice)
				{
					gMouseObject.mouse_device = target_device_object;
					break;
				}
				target_device_object = target_device_object->NextDevice;
			}
		}

        // Dereference the driver objects.
		_ObfDereferenceObject(class_driver_object);
		_ObfDereferenceObject(hid_driver_object);

        // Update the mouse object's state.
		if (gMouseObject.mouse_device && gMouseObject.service_callback) {
			gMouseObject.use_mouse = 1;
		}
	}
	return gMouseObject.mouse_device && gMouseObject.service_callback;
}

// External declaration of the MouseClassServiceCallback function.
extern "C" VOID MouseClassServiceCallback(PDEVICE_OBJECT, PMOUSE_INPUT_DATA, PMOUSE_INPUT_DATA, PULONG);

// Function to simulate mouse movement and button actions.
static void mouse::move(long x, long y, unsigned short button_flags)
{
	// Prepare the mouse input data.
	ULONG input_data = 0;
	MOUSE_INPUT_DATA mid = { };
	mid.LastX = x;
	mid.LastY = y;
	mid.ButtonFlags = button_flags;
	mid.UnitId = 1;

    // Call the service callback to simulate the mouse action.	
	MouseClassServiceCallback(gMouseObject.mouse_device, &mid, (PMOUSE_INPUT_DATA)&mid + 1, &input_data);
}

