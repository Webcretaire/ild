ifneq (${KERNELRELEASE},)

	obj-m += example-III-01.o
	obj-m += example-III-02.o
	obj-m += example-III-03.o
	obj-m += example-III-04.o
	obj-m += example-III-05.o

else

	KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
	MODULE_DIR := $(shell pwd)

.PHONY: all

all: modules

.PHONY: modules

modules:
	${MAKE} -C ${KERNEL_DIR} M=${MODULE_DIR}  modules

.PHONY: clean

clean:
	rm -f *.o *.ko *.mod.c .*.o .*.ko .*.mod.c .*.cmd *~ *.ko.unsigned *.mod
	rm -f Module.symvers Module.markers modules.order
	rm -rf .tmp_versions .cache.mk

endif
