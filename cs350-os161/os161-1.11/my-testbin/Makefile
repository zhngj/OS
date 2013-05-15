include ../defs.mk
SHELL:=/bin/bash
CMPDIRS := $(shell find ./ -maxdepth 1 -type d -a -iregex './.+')

all depend tags install:
	for i in $(CMPDIRS); do \
		echo "Compiling $$i"; \
		pushd $$i;\
		echo "PROG=$$i" > Makefile ;\
		echo SRCS=`find . -iname "*.c"` >> Makefile ;\
		echo "include ../my-prog.mk" >> Makefile ;\
		$(MAKE) $$i $@ ;\
		popd;\
	done

clean vclean:
	for i in $(CMPDIRS); do \
		pushd $$i ;\
		$(MAKE) $$i $@ ;\
		rm -f Makefile ;\
		popd;\
	done
