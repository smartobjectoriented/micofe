obj-m :=  qemu.o  

ifeq ($(IB_PLATFORM),$(filter $(IB_PLATFORM),rpi4))
obj-m := senseled.o
endif

ifeq ($(IB_PLATFORM),$(filter $(IB_PLATFORM),virt64))
obj-m := modtry.o
endif


