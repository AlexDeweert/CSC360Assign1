.phony all:
all: sample inf

sample: sample.c
	gcc sample.c -lreadline -lhistory -ltermcap -o sample

inf: inf.c
	gcc inf.c -o inf

.PHONY clean:
clean:
	-rm -rf *.o *.exe