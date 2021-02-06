ifneq ($(KERNELRELEASE),)
# kbuild part of makefile

EXTRA_CFLAGS=-std=gnu99
obj-m += sparse-keymap-all.o

else
# normal makefile

KBUILD_DIR ?= /lib/modules/`uname -r`/build
default:; $(MAKE) -C $(KBUILD_DIR) M=$$PWD
clean:; $(MAKE) clean -C $(KBUILD_DIR) M=$$PWD

endif
