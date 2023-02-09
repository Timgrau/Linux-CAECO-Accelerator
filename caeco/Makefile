export ARCH:=arm64
export CROSS_COMPILE:=aarch64-linux-gnu-

CC=$(CROSS_COMPILE)gcc

obj-m = caeco.o

KVERSION = /home/timo/Projects/petalinux-script/caeco_design_scattergather_linux/build/tmp/work/zynqmp_generic-xilinx-linux/linux-xlnx/5.15.19+gitAUTOINC+b0c1be301e-r0/linux-zynqmp_generic-standard-build
PWD := $(shell pwd)

all:
	make -C $(KVERSION) M=$(PWD) modules

clean:
	make -C $(KVERSION) M=$(PWD) clean
