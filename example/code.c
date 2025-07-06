#include "pdb_lib_c.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Uso: %s <ruta_al_exe>\n", argv[0]);
        return 1;
    }

    const char* path = argv[1];

    GUID guid;
    DWORD age;
    char pdbName[MAX_PATH];
    char url[512];
    char output[MAX_PATH];

    if (!GetPdbInfoFromImage(path, &guid, &age, pdbName, sizeof(pdbName))) {
        printf("No se pudo extraer la información del PDB.\n");
        return 1;
    }

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

    SymbolList symbols;
    if (!LoadSymbolsFromFile(path, &symbols)) {
        fprintf(stderr, "Fallo al cargar los símbolos.\n");
        return 1;
    }

    for (size_t i = 0; i < symbols.count; ++i) {
        printf("0x%llx: %s\n", symbols.items[i].address, symbols.items[i].name);
    }

    SymbolList_Free(&symbols);
    return 0;
}
