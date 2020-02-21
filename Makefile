ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM>")
endif

include $(DEVKITARM)/base_tools

TARGET  := $(shell basename $(CURDIR))

SOURCE  := source
BUILD   := build
INCDIR	:= include
SUBARCH := -mcpu=arm946e-s -mfloat-abi=soft -marm -mno-thumb-interwork -ggdb

DBG_FLAG :=
# ifneq ($(RELEASE),)
# DBG_FLAG := -DNDEBUG
# endif

INCLUDE := -I$(SOURCE) -I$(INCDIR)
ASFLAGS := $(SUBARCH) $(INCLUDE) -x assembler-with-cpp
CXXFLAGS := $(SUBARCH) $(INCLUDE) -MMD -MP -std=c++1z -O2 -pipe -Wall -Wextra \
			-Wno-unused-variable -Wno-unused-parameter -Wno-unused-function \
			-ffunction-sections -fdata-sections -ffreestanding -nostartfiles \
			-fno-rtti -fno-exceptions -fomit-frame-pointer -ffast-math -nostdlib \
			$(DBG_FLAG)

LDFLAGS := -Tlink.ld -Wl,--gc-sections,--nmagic,-z,max-page-size=4 \
           -nostartfiles $(SUBARCH) -ffreestanding -nostdlib

rwildcard = $(foreach d, $(wildcard $1*), \
            $(filter $(subst *, %, $2), $d) \
            $(call rwildcard, $d/, $2))

SOURCE_OUTPUT := $(patsubst $(SOURCE)/%.cc, $(BUILD)/%.cc.o, \
				$(patsubst $(SOURCE)/%.s, $(BUILD)/%.s.o, \
				$(call rwildcard, $(SOURCE), *.s *.cc)))

.PHONY: all
all: $(TARGET).bin

.PHONY: clean
clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).bin

$(TARGET).bin: $(TARGET).elf
	@$(OBJCOPY) -O binary $^ $@

$(TARGET).elf: $(SOURCE_OUTPUT)
	@mkdir -p "$(@D)"
	@$(CXX) $(LDFLAGS) $^ -o $@

$(BUILD)/%.cc.o: $(SOURCE)/%.cc
	@mkdir -p "$(@D)"
	@echo "  $<"
	@$(CXX) -c $(CXXFLAGS) -o $@ $<

$(BUILD)/%.s.o: $(SOURCE)/%.s
	@mkdir -p "$(@D)"
	@echo "  $<"
	@$(CC) -c $(ASFLAGS) -o $@ $<

include $(call rwildcard, $(BUILD), *.d)
