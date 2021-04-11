all: rm-elf pong.elf

include $(KOS_BASE)/Makefile.rules

OBJS = pong.o

KOS_LOCAL_CFLAGS = -I$(KOS_BASE)/addons/zlib
	
clean:
	-rm -f pong.elf $(OBJS)
	-rm -f romdisk_boot.*

dist:
	-rm -f $(OBJS)
	-rm -f romdisk_boot.*
	$(KOS_STRIP) pong.elf
	
rm-elf:
	-rm -f pong.elf
	-rm -f romdisk_boot.*

pong.elf: $(OBJS) romdisk_boot.o 
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $@ $(KOS_START) $^ -lpng -lz -lm $(KOS_LIBS)

wfont.o: wfont.bin
	$(KOS_BASE)/utils/bin2o/bin2o $< wfont $@

romdisk_boot.img:
	$(KOS_GENROMFS) -f $@ -d romdisk_boot -v

romdisk_boot.o: romdisk_boot.img
	$(KOS_BASE)/utils/bin2o/bin2o $< romdisk_boot $@

run: pong.elf
	$(KOS_LOADER) $<


