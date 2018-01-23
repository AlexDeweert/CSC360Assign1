.phony all:
all: sample

sample: sample.c
	gcc sample.c -lreadline -lhistory -ltermcap -o sample

.PHONY clean:
clean:
	-rm -rf *.o *.exe
