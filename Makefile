#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

include $(DEVKITPRO)/libnx/switch_rules

APP_TITLE   := GTASA Unexplored
APP_AUTHOR  := sklart
APP_VERSION := 0.1.0

TARGET      := gtasa-unexplored
BUILD       := build
SOURCES     := source
DATA        := assets
INCLUDES    := include
ICON        := $(DEVKITPRO)/libnx/default_icon.jpg

ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

PKG_CONFIG := $(DEVKITPRO)/portlibs/switch/bin/aarch64-none-elf-pkg-config
SDL_CFLAGS := $(shell $(PKG_CONFIG) SDL2_ttf SDL2_image --cflags)
SDL_LIBS   := $(shell $(PKG_CONFIG) SDL2_ttf SDL2_image --libs)

CFLAGS := -g -Wall -Wextra -O2 -ffunction-sections \
          $(ARCH) $(DEFINES) $(SDL_CFLAGS)
CFLAGS += $(INCLUDE) -D__SWITCH__
CXXFLAGS := $(CFLAGS) -std=gnu++17 -fexceptions -fno-rtti
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS := $(SDL_LIBS) -lnx
LIBDIRS := $(PORTLIBS) $(LIBNX)

# Do not derive the current directory name: GNU make splits a path containing
# spaces before applying notdir().  The project root is the only level holding
# source/, while the recursive invocation runs from build/.
ifneq ($(wildcard source),)
# The child make runs from build/.  Keep every project path relative to it so
# a repository directory containing spaces (for example "GTASA Unexplored")
# works with both GNU make and the devkitPro rules.
export OUTPUT := ../$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),../$(dir)) \
                $(foreach dir,$(DATA),../$(dir))
export DEPSDIR := .

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif

export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN := $(addsuffix .h,$(subst .,_,$(BINFILES)))
export INCLUDE := $(foreach dir,$(INCLUDES),-I../$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I.
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export APP_ICON := $(ICON)
export NROFLAGS += --icon=$(APP_ICON) --nacp=../$(TARGET).nacp

.PHONY: $(BUILD) clean all
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f ../Makefile

clean:
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf

else
.PHONY: all
DEPENDS := $(OFILES:.o=.d)
all: $(OUTPUT).nro
$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf: $(OFILES)
$(OFILES_SRC): $(HFILES_BIN)
%.bin.o %_bin.h: %.bin
	@echo $(notdir $<)
	@$(bin2o)
-include $(DEPENDS)
endif
