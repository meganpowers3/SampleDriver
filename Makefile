KERNELDIR=/lib/modules/`uname -r`/build


#Change the names here to your file name
MODULES = device_driver_main.ko
obj-m += device_driver_main.o 

all:
	make -C $(KERNELDIR) M=$(PWD) modules
	$(CC) driver_tester.c -o test

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	rm test

install:	
	make -C $(KERNELDIR) M=$(PWD) modules_install

quickInstall:
	cp $(MODULES) /lib/modules/`uname -r`/extra
