#include "pdb_lib_c.h"

#include <time.h>

void print_guid(const GUID* guid) {
    printf("GUID: %08X-%04X-%04X-", guid->Data1, guid->Data2, guid->Data3);
    for (int i = 0; i < 2; i++) printf("%02X", guid->Data4[i]);
    printf("-");
    for (int i = 2; i < 8; i++) printf("%02X", guid->Data4[i]);
    printf("\n");
}

void print_timestamp(DWORD timestamp) {
    if (timestamp == 0 || timestamp == 0xFFFFFFFF) {
        printf("TimeDateStamp: 0x%08X (no valida o sin significado)\n", timestamp);
        return;
    }

    time_t t = (time_t)timestamp;
    struct tm tm_info;
    char buffer[64];

    if (localtime_s(&tm_info, &t) != 0) {
        printf("Error convirtiendo timestamp a fecha local.\n");
        return;
    }

    if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_info) == 0) {
        printf("Error formateando fecha.\n");
        return;
    }

    printf("TimeDateStamp: 0x%08X (%s)\n", timestamp, buffer);
}



// Obtener y mostrar informacion basica del archivo PE (timestamp, tamaño)
void print_pe_info(const char* path) {
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("No se pudo abrir %s para obtener info PE.\n", path);
        return;
    }

    HANDLE hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMapping) {
        CloseHandle(hFile);
        printf("No se pudo crear file mapping.\n");
        return;
    }

    BYTE* data = (BYTE*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (!data) {
        CloseHandle(hMapping);
        CloseHandle(hFile);
        printf("No se pudo mapear archivo.\n");
        return;
    }

    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)data;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        printf("No es un archivo PE valido.\n");
        goto cleanup;
    }

    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(data + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        printf("No es un archivo PE valido.\n");
        goto cleanup;
    }

    DWORD timestamp = nt->FileHeader.TimeDateStamp;
    print_timestamp(timestamp);


    DWORD fileSize = GetFileSize(hFile, NULL);
    printf("Size archivo: %u bytes\n", fileSize);

cleanup:
    UnmapViewOfFile(data);
    CloseHandle(hMapping);
    CloseHandle(hFile);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <ruta_al_exe>\n", argv[0]);
        return 1;
    }

    const char* path = argv[1];

    GUID guid;
    DWORD age = 0;
    char pdbName[MAX_PATH] = {0};
    char url[512] = {0};
    char output[MAX_PATH] = {0};

    if (!GetPdbInfoFromImage(path, &guid, &age, pdbName, sizeof(pdbName))) {
        printf("No se pudo extraer la informacion del PDB.\n");
        return 1;
    }

    printf("Informacion del PDB:\n");
    print_guid(&guid);
    printf("Age: %u\n", age);
    printf("PDB Name: %s\n", pdbName);

    if (!GetPdbUrl(&guid, age, pdbName, url, sizeof(url))) {
        printf("No se pudo generar la URL del PDB.\n");
        return 1;
    }

    snprintf(output, MAX_PATH, "%s", pdbName);  // Guardar con el mismo nombre

    printf("Descargando:\nURL: %s\nDestino: %s\n", url, output);

    if (!DownloadPdb(url, output)) {
        printf("Error al descargar el archivo PDB.\n");
        return 1;
    }

    printf("PDB descargado correctamente en %s\n", output);

    // Mostrar info del PE
    print_pe_info(path);

    SymbolList symbols;
    if (!LoadSymbolsFromFile(path, &symbols)) {
        fprintf(stderr, "Fallo al cargar los simbolos.\n");
        return 1;
    }

    printf("Simbolos:\n");
    for (size_t i = 0; i < symbols.count; ++i) {
        printf("0x%llx: %s\n", symbols.items[i].address, symbols.items[i].name);
    }

    SymbolList_Free(&symbols);
    return 0;
}