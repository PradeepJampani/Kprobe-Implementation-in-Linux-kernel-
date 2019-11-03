#obj-m:= kprobe_drv.o
SRCS   = ht530_drv.c Mprobe_drv.c
OBJS   = $(SRCS:.c=.o)

obj-m += $(OBJS)
KDIR  := /lib/modules/$(shell uname -r)/build

# PWD is the current working directory and the location of our module
# source files.
PWD   := $(shell pwd)

# default is the default make target.  The rule here says to run make
# with a working directory of the directory containing the kernel
# source and compile only the modules in the PWD (local) directory.
all:		
	$(MAKE) -C $(KDIR) M=$(PWD) modules


	

clean:
	rm -f *.ko
	rm -f *.o
	rm -f Module.symvers
	rm -f modules.order
	rm -f *.mod.c
	rm -rf .tmp_versions
	rm -f *.mod.c
	rm -f *.mod.o
	rm -f \.*.cmd
	rm -f Module.markers

