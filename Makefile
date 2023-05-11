.PHONY: clean

OBJ = main.o table.o database.o cmpfunc.o tui.o strtools.o history.o keyword.o
BIN = SQL
CC  = gcc

all: $(BIN)

clean: 
	rm -f $(BIN) $(OBJ)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN)

main.o: main.c
	$(CC) -c main.c -o main.o

table.o: table.c
	$(CC) -c table.c -o table.o

database.o: database.c
	$(CC) -c database.c -o database.o

cmpfunc.o: cmpfunc.c
	$(CC) -c cmpfunc.c -o cmpfunc.o

tui.o: tui.c
	$(CC) -c tui.c -o tui.o

strtools.o: strtools.c
	$(CC) -c strtools.c -o strtools.o

history.o: history.c
	$(CC) -c history.c -o history.o

keyword.o: keyword.c
	$(CC) -c keyword.c -o keyword.o