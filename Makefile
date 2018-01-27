.phony all:
all: ssi inf

ssi: ssi.c
	gcc ssi.c -lreadline -lhistory -ltermcap -o ssi

inf: inf.c
	gcc inf.c -o inf

.PHONY clean:
clean:
	-rm -rf *.o *.exe