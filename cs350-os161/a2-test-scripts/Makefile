
# This is the defs.mk that is found in the os161-1.11 directory.
# include ../defs.mk
# The include above sets this OSTREE variable and we just use that.
# ROOT=$(OSTREE)

# Changed so it can be run more easily run out of other locations.
ROOT=$(HOME)/cs350-os161/root

SYMLINKS = testbin bin kernel uw-testbin
GENERATED = bigfile.out tailfile catfile hashfile badcallfile FILE1 FILE2 WRITE_READ_FILE
TESTOUTPUT = out.1 out.2 out.3
ASST = -ASST2

setup:
	-ln -s $(ROOT)/testbin .
	-ln -s $(ROOT)/uw-testbin .
	-ln -s $(ROOT)/bin .
	-ln -s $(ROOT)/kernel$(ASST) kernel

dotar:
	make vclean
	cd ..; tar -czf a2-test-scripts.tar.gz `cat a2-test-scripts/tarlist`; cd a2-test-scripts

runtests:
	./run-batch

clean:
	-/bin/rm $(SYMLINKS)
	-/bin/rm $(GENERATED)

vclean: clean
	-/bin/rm $(TESTOUTPUT)
	-/bin/rm DISK1.img DISK2.img

