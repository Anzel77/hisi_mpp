ifeq ($(EXTDRV_PARAM_FILE), ) 
    EXTDRV_PARAM_FILE:=../Makefile.param
    include $(EXTDRV_PARAM_FILE)
endif

SRCS := ssp_sony.c

INC := -I$(REL_INC)
INC += -I$(OSAL_ROOT)/include
INC += -I$(ISP_ROOT)/ext_inc
INC += -I$(ISP_ROOT)/kernel/mkp/include
INC += -I$(ISP_ROOT)/kernel/arch/$(HIARCH)/include
INC += -I$(ISP_ROOT)/user/firmware/arch/$(HIARCH)/include
INC += -I$(ISP_ROOT)/user/firmware/include
EXTDRV_CFLAGS += $(INC)
EXTDRV_CFLAGS += $(COMM_INC)


		
#*************************************************************************
TARGET := hi_ssp_sony

#*************************************************************************
# compile linux or HuaweiLite
include $(PWD)/../Make.$(OSTYPE)


