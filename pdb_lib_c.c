/* pdb_downloader.c - Librería para extraer y descargar PDBs de archivos PE */

#include "pdb_lib_c.h"

static void GetLastErrorAsString(DWORD errorCode, char* buffer, size_t size) {
    if (!buffer || size == 0) return;

    DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    DWORD len = FormatMessage(
        flags,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buffer,
        (DWORD)size,
        NULL);

    if (len == 0) {
        // Mensaje genérico
        printf(buffer, size, TEXT("Error desconocido %lu"), errorCode);
    } else {
        // Remover salto de línea al final si existe
        while (len > 0 && (buffer[len-1] == L'\n' || buffer[len-1] == L'\r')) {
            buffer[len-1] = L'\0';
            len--;
        }
    }
}



int GuidToStr(const GUID* guid, char* out, size_t maxLen) {
    if (!guid || !out) return 0;
    return snprintf(out, maxLen,
        "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
        guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
        guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
}

int GetPdbUrl(const GUID* guid, DWORD age, const char* pdbName, char* outUrl, size_t outLen) {
    if (!guid || !pdbName || !outUrl) return 0;

    char guidStr[256] = {0};
    if (!GuidToStr(guid, guidStr, sizeof(guidStr))) {
        DEBUG_PRINT("Error al convertir el GUID a string.");
        return 0;
    }

    int result = snprintf(outUrl, outLen,
        "https://msdl.microsoft.com/download/symbols/%s/%s%x/%s",
        pdbName, guidStr, age, pdbName);

    DEBUG_PRINT("Generada URL: %s", outUrl);
    return result > 0 && (size_t)result < outLen;
}

BOOL DownloadPdb(const char* url, const char* destPath) {
    char folder[MAX_PATH] = {0};
    strncpy(folder, destPath, MAX_PATH - 1);
    PathRemoveFileSpecA(folder);

    if (folder[0] != '\0') { // Solo crear carpeta si hay una ruta
        if (!CreateDirectoryA(folder, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
            char errbuf[256];
            GetLastErrorAsString(GetLastError(), errbuf, sizeof(errbuf));
            DEBUG_PRINT("Error al crear carpeta destino: %s", errbuf);
            return FALSE;
        }
    }

    DEBUG_PRINT("Iniciando descarga desde:\nURL: %s\nDestino: %s", url, destPath);

    HRESULT hr = URLDownloadToFileA(NULL, url, destPath, 0, NULL);
    if (FAILED(hr)) {
        char errbuf[256];
        GetLastErrorAsString(GetLastError(), errbuf, sizeof(errbuf));
        DEBUG_PRINT("Error en URLDownloadToFile: HRESULT=0x%08X, Detalle: %s", (unsigned)hr, errbuf);
        return FALSE;
    }

    DEBUG_PRINT("Descarga completada correctamente.");
    return TRUE;
}


BOOL GetPdbInfoFromImage(const char* path, GUID* outGuid, DWORD* outAge, char* outPdbName, size_t maxLen) {
    HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        char errMsg[256] = {0};  
        GetLastErrorAsString(GetLastError(), errMsg, sizeof(errMsg)/sizeof(char)); 
        DEBUG_PRINT("Error(%s) al abrir archivo: %s", errMsg, path);
        return FALSE;
    }

    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMapping) {
        DEBUG_PRINT("Error al crear file mapping");
        CloseHandle(hFile);
        return FALSE;
    }

    BYTE* base = (BYTE*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (!base) {
        DEBUG_PRINT("Error al mapear vista del archivo");
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return FALSE;
    }

    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);

    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        DEBUG_PRINT("Firma DOS inválida: 0x%X", dos->e_magic);
        goto cleanup;
    }

    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        DEBUG_PRINT("Firma NT inválida: 0x%X", nt->Signature);
        goto cleanup;
    }

    DWORD debugRVA = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
    DWORD debugSize = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;

    DEBUG_PRINT("Debug RVA: 0x%X, Size: %u", debugRVA, debugSize);

    if (!debugRVA || !debugSize) {
        DEBUG_PRINT("No hay directorio de debug");
        goto cleanup;
    }

    IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);
    DWORD debugOffset = 0;

    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        DWORD start = section[i].VirtualAddress;
        DWORD end = start + section[i].Misc.VirtualSize;
        if (debugRVA >= start && debugRVA < end) {
            debugOffset = section[i].PointerToRawData + (debugRVA - start);
            break;
        }
    }

    if (!debugOffset) {
        DEBUG_PRINT("No se pudo determinar offset físico de debug");
        goto cleanup;
    }

    IMAGE_DEBUG_DIRECTORY* dbg = (IMAGE_DEBUG_DIRECTORY*)(base + debugOffset);

    DEBUG_PRINT("dbg->Type: %u", dbg->Type);
    DEBUG_PRINT("dbg->PointerToRawData: 0x%X", dbg->PointerToRawData);

    if (dbg->Type != IMAGE_DEBUG_TYPE_CODEVIEW) {
        DEBUG_PRINT("No es de tipo CodeView");
        goto cleanup;
    }

    CV_INFO_PDB70* cv = (CV_INFO_PDB70*)(base + dbg->PointerToRawData);
    DEBUG_PRINT("cv->Signature: 0x%X", cv->Signature);

    if (cv->Signature != RSDS_SIGNATURE) {
        DEBUG_PRINT("Firma RSDS inválida");
        goto cleanup;
    }

    *outGuid = cv->Guid;
    *outAge = cv->Age;
    strncpy_s(outPdbName, maxLen, cv->PdbName, _TRUNCATE);

    UnmapViewOfFile(base);
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return TRUE;

cleanup:
    UnmapViewOfFile(base);
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return FALSE;
}




void SymbolList_Init(SymbolList* list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void SymbolList_Free(SymbolList* list) {
    for (size_t i = 0; i < list->count; ++i) {
        free(list->items[i].name);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void SymbolList_Add(SymbolList* list, const char* name, DWORD64 addr) {
    if (list->count >= list->capacity) {
        size_t newCapacity = (list->capacity == 0) ? 64 : list->capacity * 2;
        SymbolInfo* newItems = realloc(list->items, newCapacity * sizeof(SymbolInfo));
        if (!newItems) {
            fprintf(stderr, "Error: sin memoria al aumentar lista de símbolos.\n");
            exit(EXIT_FAILURE);
        }
        list->items = newItems;
        list->capacity = newCapacity;
    }

    char* nameCopy = _strdup(name); // strdup segura en MSVC
    if (!nameCopy) {
        fprintf(stderr, "Error: sin memoria al duplicar nombre de símbolo.\n");
        exit(EXIT_FAILURE);
    }

    list->items[list->count].name = nameCopy;
    list->items[list->count].address = addr;
    list->count++;
}

BOOL CALLBACK EnumSymbolsCallback(PSYMBOL_INFO pSymInfo, ULONG SymbolSize, PVOID UserContext) {
    CallbackContext* ctx = (CallbackContext*)UserContext;
    SymbolList_Add(ctx->list, pSymInfo->Name, pSymInfo->Address);
    return TRUE;
}

int LoadSymbolsFromFile(const char* path, SymbolList* outList) {
    HANDLE hProcess = GetCurrentProcess();
    SymbolList_Init(outList);

    if (!SymInitialize(hProcess, NULL, FALSE)) {
        fprintf(stderr, "Error en SymInitialize: %lu\n", GetLastError());
        return 0;
    }

    DWORD64 baseAddress = SymLoadModuleEx(
        hProcess,
        NULL,
        path,
        NULL,
        0,
        0,
        NULL,
        0
    );

    if (baseAddress == 0) {
        fprintf(stderr, "Error en SymLoadModuleEx: %lu\n", GetLastError());
        SymCleanup(hProcess);
        return 0;
    }

    CallbackContext ctx = { .list = outList };

    if (!SymEnumSymbols(hProcess, baseAddress, NULL, EnumSymbolsCallback, &ctx)) {
        fprintf(stderr, "Error en SymEnumSymbols: %lu\n", GetLastError());
        SymUnloadModule64(hProcess, baseAddress);
        SymCleanup(hProcess);
        return 0;
    }

    SymUnloadModule64(hProcess, baseAddress);
    SymCleanup(hProcess);
    return 1;
}