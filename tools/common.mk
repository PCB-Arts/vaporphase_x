# Shared part of the project Makefiles. Command-line build: runs CubeIDE's
# headless Eclipse builder on .cproject, i.e. exactly what the IDE's "Build"
# button runs. Compiler options are changed in the IDE, never here.
#
# The including Makefile sets before 'include ../tools/common.mk':
#   PROJECT     project name (= .ioc, .elf and CubeIDE project name)
#   FLASH_FREQ  SWD frequency in kHz for 'make flash'
#   FLASH_HELP  'make help' description of 'make flash'
# and optionally:
#   FLASH_ARGS  extra STM32_Programmer_CLI arguments for 'make flash'

BUILD_CONFIG ?= Debug
CUBEIDE      ?= /opt/st/stm32cubeide_2.2.0/stm32cubeide
# stm32cubeprog_2.23.0 does not work with external loader (leaves NVIC ISPR set for IRQs 0-125)
PROGRAMMER   ?= /opt/st/stm32cubeprog_2.22.0/bin/STM32_Programmer_CLI
CUBEMX       ?= /opt/st/stm32cubemx_6.18.1/STM32CubeMX
# Eclipse workspace for the headless build (metadata + CDT index only).
WORKSPACE    ?= $(abspath ../.cubeide-ws/$(PROJECT))

OUT     := $(BUILD_CONFIG)
ELF     := $(OUT)/$(PROJECT).elf
HEX     := $(OUT)/$(PROJECT).hex
IOC     := $(abspath $(PROJECT).ioc)
# Required for grep '\r'
SHELL   := /bin/bash

.PHONY: all build hex clean flash size generate help

all: build

# CubeIDE's verdict is untrustworthy (non-zero exit on success, one phantom
# linker error), so it is ignored and filtered, the .elf decides.
build:
	@rm -f $(ELF)
	-$(CUBEIDE) --launcher.suppressErrors -nosplash \
	  -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
	  -data $(WORKSPACE) -import . -build $(PROJECT)/$(BUILD_CONFIG) 2>&1 \
	  | sed -e '/^Stm32cubeide:$$/,$$d' -e '/Build Failed\./d' -e '/Build Finished\./d'
	@test -f $(ELF) \
	  && echo "==> build OK: $(ELF)" \
	  || { echo "==> BUILD FAILED: $(ELF) not produced" >&2; exit 1; }

clean:
	rm -rf Debug Release

hex: build
	arm-none-eabi-objcopy -O ihex $(ELF) $(HEX)

flash: hex
	$(PROGRAMMER) -c port=SWD freq=$(FLASH_FREQ) -d $(HEX) $(FLASH_ARGS) -hardRst

size:
	arm-none-eabi-size $(ELF)

# Runs CubeMX on the .ioc and dos2unix afterwards (except for ./TouchGFX/generated)
generate:
	@tmpfile=$$(mktemp) && trap 'rm -f $$tmpfile $$tmpfile.log' EXIT \
	  && printf 'config load %s\nproject generate\nexit\n' '$(IOC)' > $$tmpfile \
	  && $(CUBEMX) -q $$tmpfile 2>&1 | tee $$tmpfile.log
	@grep -I -r -l -Z $$'\r' . | grep -z -v '^\./TouchGFX/generated/' | xargs -0 -r dos2unix

help:
	@echo "make              build BUILD_CONFIG=$(BUILD_CONFIG) -> $(ELF)"
	@echo "make hex          build + derive $(HEX)"
	@echo "make clean        remove build output"
	@echo "make flash        $(FLASH_HELP)"
	@echo "make size         section sizes"
	@echo "make generate     run CubeMX code generation on the .ioc"
	@echo "                  TouchGFX projects: CubeMX deletes Middlewares/ST/touchgfx_components,"
	@echo "                  restore it with 'git restore Middlewares/ST/touchgfx_components'"
	@echo "                  or TouchGFX Designer 'Generate Code'"
	@echo
	@echo "BUILD_CONFIG=Release"
	@echo "                  select the Release configuration (now $(BUILD_CONFIG))"
	@echo "CUBEIDE=<path>    now $(CUBEIDE)"
	@echo "PROGRAMMER=<path> now $(PROGRAMMER)"
	@echo "CUBEMX=<path>     now $(CUBEMX)"
	@echo "WORKSPACE=<dir>   now $(WORKSPACE)"
