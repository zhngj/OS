
writeread.o: \
 writeread.c \
 $(OSTREE)/include/stdio.h \
 $(OSTREE)/include/sys/types.h \
 $(OSTREE)/include/machine/types.h \
 $(OSTREE)/include/kern/types.h \
 $(OSTREE)/include/stdarg.h \
 $(OSTREE)/include/stdlib.h \
 $(OSTREE)/include/assert.h \
 $(OSTREE)/include/sys/stat.h \
 $(OSTREE)/include/kern/stat.h \
 $(OSTREE)/include/fcntl.h \
 $(OSTREE)/include/unistd.h \
 $(OSTREE)/include/kern/unistd.h \
 $(OSTREE)/include/kern/ioctl.h \
 ../lib/testutils.h

