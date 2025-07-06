CC              = gcc
CFLAGS          = -DDEBUG -I. -Wall
DLL_FLAGS       = -lurlmon -lshlwapi -ldbghelp
PATH_EXAMPLE    = example
LIB_SRC         = pdb_lib_c.c
EXAMPLE_SRC     = $(wildcard $(PATH_EXAMPLE)/*.c)
EXAMPLE_EXE     = $(patsubst %.c,%.exe,$(notdir $(EXAMPLE_SRC)))


# Regla por defecto
all: $(EXAMPLE_EXE)

# Compilar cada .exe a partir de su .c
%.exe: $(PATH_EXAMPLE)/%.c $(LIB_SRC)
	$(CC) $(CFLAGS) $< $(LIB_SRC) -o $@ $(DLL_FLAGS)

# Limpieza
clean:
	del /Q $(PATH_EXAMPLE)\*.exe 2>nul
