TARGET = yeti3d

PSP_OBJS = platform/psp/main.o
YETI_OBJS = src/data.o \
			src/draw.o \
			src/extra.o \
			src/font.o \
			src/game.o \
			src/maps.o \
			src/model.o \
			src/sprites.o \
			src/yeti.o

OBJS = $(PSP_OBJS) $(YETI_OBJS)

INCDIR = 
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS = 

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Yeti3D

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
