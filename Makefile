OBJS = pbackup.o

INSTALLDIR = /usr/local/bin/

CC = gcc
CFLAGS = -Wall
INSTALL = install

all: pbackup

pbackup: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o pbackup

install:
	mkdir -p $(INSTALLDIR)
	$(INSTALL) pbackup $(INSTALLDIR) 

uninstall:
	rm -f $(INSTALLDIR)/pbackup
