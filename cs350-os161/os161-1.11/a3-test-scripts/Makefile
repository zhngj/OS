
# This is the defs.mk that is found in the os161-1.11 directory.
include ../defs.mk

# This is relative to the $(HOME) directory which doesn't work
# when running as part of cs350_submit 
# The include above sets this OSTREE variable and we just use that.
# ROOT=$(OSTREE)

# Changed so it can be run out of other locations.
# ROOT=$(HOME)/cs350-os161/root

# Needs to be a relative path
ROOT=../../root

SYMLINKS = testbin uw-testbin bin kernel
GENERATED = bigfile.out tailfile catfile hashfile badcallfile
TESTOUTPUT = out.A3 out.A3-option1 out.A3-option2
ASST = -ASST3

setup:
	-ln -s $(ROOT)/testbin .
	-ln -s $(ROOT)/uw-testbin .
	-ln -s $(ROOT)/bin .
	-ln -s $(ROOT)/kernel$(ASST) kernel

all: setup runtests

runtests: 
	./run-batch-A3-all
	-/bin/rm -f SWAPFILE

# pgms:
# 	cd vm-crash1; make depend; make; make localinstall
# 	cd vm-crash2; make depend; make; make localinstall
# 	cd vm-crash3; make depend; make; make localinstall
# 	cd vm-data1; make depend; make; make localinstall
# 	cd vm-data2; make depend; make; make localinstall
# 	cd vm-data3; make depend; make; make localinstall
# 	cd vm-stack1; make depend; make; make localinstall
# 	cd vm-stack2; make depend; make; make localinstall
# 	cd vm-mix1; make depend; make; make localinstall
# 	cd vm-mix2; make depend; make; make localinstall
# 	cd vm-mix1-fork; make depend; make; make localinstall

#	cd ..; tar -czf a3-test-scripts.tar.gz `cat a3-test-scripts/tarlist`; cd a3-test-scripts

dotar:
	make vclean
	cd ..; tar --exclude CVS -czf a3-test-scripts.tar.gz `cat a3-test-scripts/tarlist`; cd a3-test-scripts

# installpgms:
# 	cp vm-crash1/vm-crash1 newtests/
# 	cp vm-crash2/vm-crash2 newtests/
# 	cp vm-crash3/vm-crash3 newtests/
# 	cp vm-data1/vm-data1 newtests/
# 	cp vm-data2/vm-data2 newtests/
# 	cp vm-data3/vm-data3 newtests/
# 	cp vm-stack1/vm-stack1 newtests/
# 	cp vm-stack2/vm-stack2 newtests/

clean:
	-/bin/rm $(SYMLINKS)
	-/bin/rm $(GENERATED)

vclean: clean
	-/bin/rm $(TESTOUTPUT)
	-/bin/rm DISK1.img DISK2.img
	-/bin/rm -rf newtests
	-/bin/rm tmp.got tmp.expected
	-/bin/rm out.A3*
	-/bin/rm SWAPFILE

#	cd vm-crash1; make vclean
#	cd vm-crash2; make vclean
#	cd vm-crash3; make vclean
#	cd vm-data1; make vclean
#	cd vm-data2; make vclean
#	cd vm-data3; make vclean
#	cd vm-stack1; make vclean
#	cd vm-stack2; make vclean
#	cd vm-mix1; make vclean
#	cd vm-mix2; make vclean
#	cd vm-mix1-fork; make vclean
