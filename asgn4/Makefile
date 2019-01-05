CC=gcc
CFLAGS= -D_FILE_OFFSET_BITS=64 -Wno-discarded-qualifiers
OBJS= directory.o fsbase.o superblock.o main.o
CONF= directory.o fsbase.o superblock.o configure.o

fileSystem : $(OBJS)
	$(CC) -o $@ `pkgconf fuse --cflags --libs` $(OBJS)

%.o : %.c
	$(CC) -c -o $@

config: $(CONF)
	$(CC) -o $@ $(CONF)

clean:
	rm $(OBJS) configure.o
