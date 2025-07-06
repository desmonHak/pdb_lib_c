# PDB_LIB_C

Pequeña utilidad en C para la auto descarga y dumpeado de simbolos de PDB. Creditos a [BlackShell256](https://github.com/BlackShell256) por contribuir al codigo.

----

Puede usar el Makefile para compilar o usar  la siguiente linea:
```c
gcc -DDEBUG -I. -Wall example/code.c pdb_lib_c.c -o code.exe -lurlmon -lshlwapi -ldbghelp
```

- `-DDEBUG`: permite el modo depuracion.
- `-I.`: indica que se usara el directorio actual como directorio de inclusion, esto permite que `pdb_lib_c.c` pueda incluir `pdb_lib_c.h` directamente.
- `-lurlmon -lshlwapi -ldbghelp`: dll's que usar para enlazar.

----

El programa espera recibir como argumento, una DLL o ejecutable oficial de microsoft con un PDB asociado para poder ser precesado:

```
C:\Users\desmon0xff\Desktop\pdb_lib_c>code.exe ntdll.dll
[DEBUG] Debug RVA: 0x19C4C0, Size: 112
[DEBUG] dbg->Type: 2
[DEBUG] dbg->PointerToRawData: 0x1A2B9C
[DEBUG] cv->Signature: 0x53445352
Informacion del PDB:
GUID: C7A9C64B-6440-8F28-DE65-08D118DED28D
Age: 1
PDB Name: ntdll.pdb
[DEBUG] Generada URL: https://msdl.microsoft.com/download/symbols/ntdll.pdb/C7A9C64B64408F28DE6508D118DED28D1/ntdll.pdb
Descargando:
URL: https://msdl.microsoft.com/download/symbols/ntdll.pdb/C7A9C64B64408F28DE6508D118DED28D1/ntdll.pdb
Destino: ntdll.pdb
[DEBUG] Iniciando descarga desde:
URL: https://msdl.microsoft.com/download/symbols/ntdll.pdb/C7A9C64B64408F28DE6508D118DED28D1/ntdll.pdb
Destino: ntdll.pdb
[DEBUG] Descarga completada correctamente.
PDB descargado correctamente en ntdll.pdb
TimeDateStamp: 0x9194561F (2047-05-25 19:21:35)
Size archivo: 2513744 bytes
Simbolos:
0x180169b6e: RtlUnicodeStringToOemString$fin$0
0x1801689e8: TppCleanupGroupMemberInitialize$fin$1
0x1801689c0: TppCleanupGroupMemberInitialize$fin$0
0x180168a87: TppCleanupGroupMemberInitialize$fin$3
0x180168a05: TppCleanupGroupMemberInitialize$fin$2
0x180120b14: crc_word
0x1800ccdd0: RtlWideCharArrayCopyStringWorker
0x180159d20: detect_data_type
0x18016b68d: TppCritSetThread$fin$3
0x18015a2b4: send_all_trees
0x18016a8c1: TppWorkerThread$filt$11
0x180073ce0: RtlpSaveX87State
0x1801694ec: RtlReAllocateHeap$filt$0
0x180169fe0: RtlpNotOwnerCriticalSection$filt$0
0x18016a3ac: TppRaiseInvalidParameter$filt$0
0x180169029: RtlSetUserValueHeap$fin$0
0x180169d3c: LdrGetDllHandleByMapping$filt$0
0x1800d78f8: PsspInitializeContextOrExtendedContext
0x1800d884c: PsspWalkHandleTable
0x18016a280: RtlpTpWaitCheckReset$fin$0
```