//
// Win32 type and function stubs for testing osdep_*_win32.cpp on Linux
//

#ifndef WIN32_STUBS_H
#define WIN32_STUBS_H

#include <string.h>
#include <stdlib.h>

// Basic Win32 types
typedef unsigned long DWORD;
typedef int BOOL;
typedef void *HANDLE;
typedef void *HMODULE;
typedef void *HINSTANCE;
typedef void *FARPROC;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef long LONG;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define INVALID_HANDLE_VALUE ((HANDLE)(long)-1)
#define WINAPI

// Memory protection constants
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define MEM_COMMIT             0x1000

// File access constants
#define GENERIC_READ           0x80000000UL
#define FILE_SHARE_READ        0x00000001
#define OPEN_EXISTING          3
#define FILE_ATTRIBUTE_NORMAL  0x00000080
#define FILE_MAP_READ          0x0004

// PE format constants
#define IMAGE_DOS_SIGNATURE    0x5A4D
#define IMAGE_NT_SIGNATURE     0x00004550
#define IMAGE_DIRECTORY_ENTRY_EXPORT 0

// PE structures
#pragma pack(push, 1)

typedef struct _IMAGE_DOS_HEADER {
	WORD  e_magic;
	WORD  e_cblp;
	WORD  e_cp;
	WORD  e_crlc;
	WORD  e_cparhdr;
	WORD  e_minalloc;
	WORD  e_maxalloc;
	WORD  e_ss;
	WORD  e_sp;
	WORD  e_csum;
	WORD  e_ip;
	WORD  e_cs;
	WORD  e_lfarlc;
	WORD  e_ovno;
	WORD  e_res[4];
	WORD  e_oemid;
	WORD  e_oeminfo;
	WORD  e_res2[10];
	LONG  e_lfanew;
} IMAGE_DOS_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
	DWORD VirtualAddress;
	DWORD Size;
} IMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct _IMAGE_FILE_HEADER {
	WORD  Machine;
	WORD  NumberOfSections;
	DWORD TimeDateStamp;
	DWORD PointerToSymbolTable;
	DWORD NumberOfSymbols;
	WORD  SizeOfOptionalHeader;
	WORD  Characteristics;
} IMAGE_FILE_HEADER;

typedef struct _IMAGE_OPTIONAL_HEADER {
	WORD  Magic;
	BYTE  MajorLinkerVersion;
	BYTE  MinorLinkerVersion;
	DWORD SizeOfCode;
	DWORD SizeOfInitializedData;
	DWORD SizeOfUninitializedData;
	DWORD AddressOfEntryPoint;
	DWORD BaseOfCode;
	DWORD BaseOfData;
	DWORD ImageBase;
	DWORD SectionAlignment;
	DWORD FileAlignment;
	WORD  MajorOperatingSystemVersion;
	WORD  MinorOperatingSystemVersion;
	WORD  MajorImageVersion;
	WORD  MinorImageVersion;
	WORD  MajorSubsystemVersion;
	WORD  MinorSubsystemVersion;
	DWORD Win32VersionValue;
	DWORD SizeOfImage;
	DWORD SizeOfHeaders;
	DWORD CheckSum;
	WORD  Subsystem;
	WORD  DllCharacteristics;
	DWORD SizeOfStackReserve;
	DWORD SizeOfStackCommit;
	DWORD SizeOfHeapReserve;
	DWORD SizeOfHeapCommit;
	DWORD LoaderFlags;
	DWORD NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER;

typedef struct _IMAGE_NT_HEADERS {
	DWORD                Signature;
	IMAGE_FILE_HEADER    FileHeader;
	IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS;

#define IMAGE_SIZEOF_SHORT_NAME 8

typedef struct _IMAGE_SECTION_HEADER {
	BYTE  Name[IMAGE_SIZEOF_SHORT_NAME];
	union {
		DWORD PhysicalAddress;
		DWORD VirtualSize;
	} Misc;
	DWORD VirtualAddress;
	DWORD SizeOfRawData;
	DWORD PointerToRawData;
	DWORD PointerToRelocations;
	DWORD PointerToLinenumbers;
	WORD  NumberOfRelocations;
	WORD  NumberOfLinenumbers;
	DWORD Characteristics;
} IMAGE_SECTION_HEADER;

typedef struct _IMAGE_EXPORT_DIRECTORY {
	DWORD Characteristics;
	DWORD TimeDateStamp;
	WORD  MajorVersion;
	WORD  MinorVersion;
	DWORD Name;
	DWORD Base;
	DWORD NumberOfFunctions;
	DWORD NumberOfNames;
	DWORD AddressOfFunctions;
	DWORD AddressOfNames;
	DWORD AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY;

#pragma pack(pop)

#define IMAGE_FIRST_SECTION(ntheaders) \
	((IMAGE_SECTION_HEADER *)((char *)&(ntheaders)->OptionalHeader + \
	 (ntheaders)->FileHeader.SizeOfOptionalHeader))

// Win32 API function stubs (defined in win32_stubs.cpp)
extern "C" {

BOOL IsBadReadPtr(const void *lp, unsigned int ucb);
BOOL IsBadStringPtrA(const char *lpsz, unsigned int ucchMax);

HANDLE CreateFileA(const char *lpFileName, DWORD dwDesiredAccess,
                   DWORD dwShareMode, void *lpSecurityAttributes,
                   DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                   HANDLE hTemplateFile);
HANDLE CreateFileMapping(HANDLE hFile, void *lpAttributes, DWORD flProtect,
                         DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow,
                         const char *lpName);
void *MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess,
                    DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow,
                    unsigned long dwNumberOfBytesToMap);
BOOL UnmapViewOfFile(const void *lpBaseAddress);
BOOL CloseHandle(HANDLE hObject);

void *VirtualAlloc(void *lpAddress, unsigned long dwSize, DWORD flAllocationType,
                   DWORD flProtect);
BOOL VirtualProtect(void *lpAddress, unsigned long dwSize, DWORD flNewProtect,
                    DWORD *lpflOldProtect);

DWORD GetLastError(void);

HINSTANCE LoadLibraryA(const char *lpLibFileName);
FARPROC GetProcAddress(HINSTANCE hModule, const char *lpProcName);
BOOL FreeLibrary(HINSTANCE hLibModule);

} // extern "C"

#endif // WIN32_STUBS_H
