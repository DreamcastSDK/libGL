ifneq ($(DEBUG),true)
  QUIET:=@
endif

ifndef PLATFORM
  PLATFORM:=dreamcast
endif

ifndef ARCH
  ARCH:=sh-elf
endif

ifndef INSTALL_PATH
  INSTALL_PATH:=/usr/local
endif

OBJS:=gl-rgb.o gl-fog.o gl-sh4-light.o gl-light.o gl-clip.o \
	gl-clip-arrays.o gl-arrays.o gl-pvr.o gl-matrix.o \
	gl-api.o gl-texture.o glu-texture.o gl-framebuffer.o \
	gl-cap.o gl-error.o

TARGET:=libGL.a

CFLAGS:=-ffast-math -O3 \
	-std=c11 \
        -Wall -Wextra\
        -fno-builtin \
        -fno-strict-aliasing \
        -fomit-frame-pointer \
        -ffunction-sections \
        -fdata-sections

CFLAGS+=-Iinclude \
	-I$(INSTALL_PATH)/$(PLATFORM)/$(ARCH)/include

GCCPREFIX:=$(PLATFORM)-$(shell echo $(ARCH) | cut -d '-' -f 1)

$(TARGET): $(OBJS)
	@echo Linking: $@
	$(QUIET) $(GCCPREFIX)-ar rcs $@ $(OBJS)

install: $(TARGET)
	@echo "Installing..."
	$(QUIET) cp -R include/* $(INSTALL_PATH)/$(PLATFORM)/$(ARCH)/include/
	$(QUIET) cp $(TARGET)    $(INSTALL_PATH)/$(PLATFORM)/$(ARCH)/lib/

clean:
	$(QUIET) rm -f $(OBJS) $(TARGET)

%.o: %.c
	@echo Building: $@
	$(QUIET) $(GCCPREFIX)-gcc $(CFLAGS) -c $< -o $@

%.o: %.s
	@echo Building: $@
	$(QUIET) $(GCCPREFIX)-as $< -o $@

%.o: %.S
	@echo Building: $@
	$(QUIET) $(GCCPREFIX)-as $< -o $@
