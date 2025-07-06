#ifndef PDB_LIB_C_H
#define PDB_LIB_C_H

#include <windows.h>
#include <shlwapi.h>
#include <dbghelp.h>
#include <stdio.h>
#include <stdlib.h>
#include <urlmon.h>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shlwapi.lib")

#define RSDS_SIGNATURE 0x53445352

#ifdef DEBUG
  #define DEBUG_PRINT(fmt, ...) printf(TEXT("[DEBUG] ") TEXT(fmt) TEXT("\n"), ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(fmt, ...) (void)0
#endif

typedef struct CV_INFO_PDB70 {
    DWORD Signature;
    GUID Guid;
    DWORD Age;
    CHAR PdbName[1];
} CV_INFO_PDB70;




typedef struct SymbolInfo {
    char* name;
    DWORD64 address;
} SymbolInfo;

typedef struct SymbolList {
    SymbolInfo* items;
    size_t count;
    size_t capacity;
} SymbolList;


typedef struct CallbackContext {
    SymbolList* list;
} CallbackContext;


BOOL GetPdbInfoFromImage(const char* path, GUID* outGuid, DWORD* outAge, char* outPdbName, size_t maxLen);
int GetPdbUrl(const GUID* guid, DWORD age, const char* pdbName, char* outUrl, size_t outLen);
int GuidToStr(const GUID* guid, char* out, size_t maxLen);
BOOL DownloadPdb(const char* url, const char* destPath);


void SymbolList_Init(SymbolList* list);
void SymbolList_Free(SymbolList* list);
void SymbolList_Add(SymbolList* list, const char* name, DWORD64 addr);
BOOL CALLBACK EnumSymbolsCallback(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext);
int LoadSymbolsFromFile(const char* path, SymbolList* outList);

#endif