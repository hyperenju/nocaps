obj-m = nocaps.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	cp $(obj-m:.o=.ko) /lib/modules/$(shell uname -r)/
	depmod
	modprobe $(obj-m:.o=)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
