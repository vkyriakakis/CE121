CC= gcc
CFLAGS= -Wall -g

#Stoxos all xwris kanona, wste na kataskeuastoun aytomata ola ta ektelesima
all: hw2 dirlist p2archive p2crypt p2unarchive 

hw2: hw2.o hw2_functions.o
	$(CC) $(CFLAGS) hw2.o hw2_functions.o -o $@

hw2.o: hw2.c hw2_functions.h
	$(CC) $(CFLAGS) -c hw2.c 

dirlist:
	$(CC) $(CFLAGS) dirlist.c -o $@

p2archive: p2archive.o hw2_functions.o
	$(CC) $(CFLAGS) p2archive.o hw2_functions.o -o $@

p2archive.o: p2archive.c hw2_functions.h
	$(CC) $(CFLAGS) -c  p2archive.c 

p2unarchive: p2unarchive.o hw2_functions.o
	$(CC) $(CFLAGS) p2unarchive.o hw2_functions.o -o $@

p2unarchive.o: p2unarchive.c hw2_functions.h
	$(CC) $(CFLAGS) p2unarchive.c -c 

p2crypt: p2crypt.o hw2_functions.o
	$(CC) $(CFLAGS) p2crypt.o hw2_functions.o -o $@

p2crypt.o: p2crypt.c hw2_functions.h
	$(CC) $(CFLAGS) p2crypt.c -c 

hw2_functions.o: hw2_functions.c hw2_functions.h
	$(CC) $(CLAGS) hw2_functions.c -c 

.PHONY: clean clean_exec 

#Katharisma twn object files
clean:
	rm -rf *.o

#katharisma twn executables
clean_exec:
	rm dirlist p2crypt p2unarchive p2archive hw2

