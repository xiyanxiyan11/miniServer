#CC=/home/padmate/padmate/rootfs/i686-linux/usr/libexec/armv7a-vfp-neon-oe-linux-gnueabi/gcc/arm-oe-linux-gnueabi/4.6.3/arm-oe-linux-gnueabi-gcc 
#CFLAGS=--sysroot=/home/padmate/padmate/rootfs/i686-linux/9615-cdp -O2 -pipe -g -march=armv7-a -fno-tree-vectorize -mthumb-interwork -mfloat-abi=softfp -mfpu=neon -mtune=cortex-a8 -mno-thumb-interwork -mno-thumb

CC = gcc

##CC=powerc-tsl-linux-gunspe-gcc

SRC = core.c sockunion.c stream.c thread.c prefix.c linklist.c network.c vector.c log.c hash.c main.c memtypes.c sigevent.c common.c  \
      udp_link.c ludp_link.c tcp_link.c sdio_link.c echo_task.c kmem.c thpool.c tasklist.c shm.c -lpthread -lev -lm


all:$(SRC)
	$(CC) $(SRC) -o misaka $(CFLAGS) -g

clean:
	rm misaka
