
obj-m   := revpimod.o


ccflags-y := -O2
ccflags-$(_ACPI_DEBUG) += -DACPI_DEBUG_OUTPUT

KBUILD_CFLAGS += -g

PWD   	:= $(shell pwd)

EXTRA_CFLAGS = -I$(src)/

EXTRA_CFLAGS += -D__KUNBUSPI_KERNEL__ -DDEBUG


.PHONY: compiletime.h

modules: compiletime.h
ifeq ($(strip $(KDIR)),)
	@echo "Error: please specify kernel dir variable KDIR"
	exit 1
endif

	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KDIR) M=$(PWD)  modules

compiletime.h:
	echo "#define COMPILETIME \""`date`"\"" > compiletime.h

clean:
	$(MAKE) ARCH=arm -C $(KDIR) M=$(PWD) clean




