# Build the MDF NEONGRF runtime and the V2.0 NPU adapter with the matching
# FreeRTOS/BSP ABI. The model remains a separate IFLASH payload, not part of ELF.
.DEFAULT_GOAL := all
HERE := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
APP_ROOT := $(HERE)
SDK_ROOT := $(abspath $(HERE)/../..)
FX1_V2_BSP_ROOT := $(APP_ROOT)/firmware_support/FX1_V2.0
RUNTIME_ROOT := $(SDK_ROOT)/neon-mdf/components/neon_runtime
NPU_ROOT := $(SDK_ROOT)/neon-mdf/components/driver/runtime_npu
DRIVER_ROOT := $(SDK_ROOT)/neon-mdf/components/driver/runtime_npu/platform/FX1_V2.0
MODEL_DIR := $(APP_ROOT)/models/mobilenet_nano_qat_int8_nchw_native
MODEL_NSM ?= $(MODEL_DIR)/mobilenet_nano_qat_int8.nsm
OUT_DIR ?= $(APP_ROOT)/build/FX1_V2_RUNTIME
OBJ_DIR := $(OUT_DIR)/obj

CROSS ?= $(shell command -v riscv-none-elf-gcc >/dev/null 2>&1 && echo riscv-none-elf- || echo $(HOME)/tools/riscv/xpack-riscv-none-elf-gcc-15.2.0-1/bin/riscv-none-elf-)
CC := $(CROSS)gcc
CXX := $(CROSS)g++
OBJDUMP := $(CROSS)objdump
OBJCOPY := $(CROSS)objcopy

FREERTOS := $(FX1_V2_BSP_ROOT)/third_party/FreeRTOS-Kernel
PORT_DIR := $(FREERTOS)/portable/GCC/RISC-V
CHIP_EXT := $(PORT_DIR)/chip_specific_extensions/RISCV_MTIME_CLINT_no_extensions
ARCH := -march=rv32imac_zicsr -mabi=ilp32
INCLUDES := -I$(HERE) -I$(HERE)/include -I$(RUNTIME_ROOT) -I$(RUNTIME_ROOT)/runtime/include \
  -I$(RUNTIME_ROOT)/third_party/dlpack/include -I$(RUNTIME_ROOT)/analyse \
  -I$(NPU_ROOT) -I$(DRIVER_ROOT) \
  -I$(FX1_V2_BSP_ROOT) -I$(FX1_V2_BSP_ROOT)/include -I$(FX1_V2_BSP_ROOT)/common/include \
  -I$(FREERTOS)/include -I$(PORT_DIR) -I$(CHIP_EXT) \
  -I$(SDK_ROOT)/neon-mdf/arch/RISCV/Device/FTEL/FX1/Include
CFLAGS := $(ARCH) -Os -g -ffreestanding -fno-builtin -nostdlib -fno-pic \
  -ffunction-sections -fdata-sections -MMD -MP -Wall -Wextra $(INCLUDES)
MODEL_BYTES := $(shell wc -c < "$(MODEL_NSM)")
CXXFLAGS := $(filter-out -ffreestanding,$(CFLAGS)) -std=c++17 -fno-exceptions -fno-rtti \
  -fno-threadsafe-statics -fno-use-cxa-atexit -Wno-unused-parameter \
  -DNEON_RUNTIME_NO_FILESYSTEM=1 -DNEON_RUNTIME_EXPLICIT_CPU_FALLBACK_REGISTRATION=1 \
  -DNEON_MODEL_BYTES=$(MODEL_BYTES)

RUNTIME_CPP := \
  $(RUNTIME_ROOT)/runtime/loader/ModelLoader.cpp \
  $(RUNTIME_ROOT)/runtime/executor/Executor.cpp \
  $(RUNTIME_ROOT)/runtime/graph/GraphLoader.cpp \
  $(RUNTIME_ROOT)/runtime/graph/ArenaPlanner.cpp \
  $(RUNTIME_ROOT)/runtime/graph/GraphRunner.cpp \
  $(RUNTIME_ROOT)/runtime/binder/OperandBinder.cpp \
  $(RUNTIME_ROOT)/runtime/capi/NeonCAPI.cpp \
  $(RUNTIME_ROOT)/analyse/StaticAnalyzer.cpp \
  $(RUNTIME_ROOT)/analyse/PerfReport.cpp \
  $(RUNTIME_ROOT)/analyse/MeasuredProfiler.cpp \
  $(RUNTIME_ROOT)/analyse/DMAProfiler.cpp
FALLBACK_CPP := \
  $(RUNTIME_ROOT)/runtime/opfallbacks/TransposeCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/DWConvCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/DWIm2ColCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/Im2ColCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/LutCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/SliceCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/StridedSliceCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/PadCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/ConcatCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/ResizeCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/MaxPoolCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/SoftmaxCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/LayerNormCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/ConstCopyCPUFallback.cpp \
  $(RUNTIME_ROOT)/runtime/opfallbacks/EmbeddingCPUFallback.cpp
NPU_DRIVER_CPP := \
  $(DRIVER_ROOT)/NPUV4Runtime.cpp \
  $(DRIVER_ROOT)/NPUV4Relocator.cpp \
  $(DRIVER_ROOT)/SoftwareRequantFallback.cpp
CPP_SRCS := $(HERE)/main.cpp $(HERE)/src/RuntimeInputProvider.cpp $(RUNTIME_CPP) $(FALLBACK_CPP) $(NPU_DRIVER_CPP)
C_SRCS := \
  $(FREERTOS)/tasks.c $(FREERTOS)/list.c $(FREERTOS)/queue.c \
  $(FREERTOS)/portable/MemMang/heap_4.c $(PORT_DIR)/port.c \
  $(FX1_V2_BSP_ROOT)/bsp/uart.c $(FX1_V2_BSP_ROOT)/bsp/plic.c $(FX1_V2_BSP_ROOT)/src/libc_min.c \
  $(DRIVER_ROOT)/bsp_ext/npu_v4_native_cfg.c \
  $(DRIVER_ROOT)/bsp_ext/npu_v4_native_driver.c
ASM_SRCS := $(FX1_V2_BSP_ROOT)/startup/start.S $(PORT_DIR)/portASM.S
CPP_OBJS := $(addprefix $(OBJ_DIR)/,$(notdir $(CPP_SRCS:.cpp=.o)))
C_OBJS := $(addprefix $(OBJ_DIR)/,$(notdir $(C_SRCS:.c=.o)))
ASM_OBJS := $(addprefix $(OBJ_DIR)/,$(notdir $(ASM_SRCS:.S=.o)))
TARGET := $(OUT_DIR)/neon_graph_vp_v2.elf
MAP := $(OUT_DIR)/neon_graph_vp_v2.map
VP_DIR := $(APP_ROOT)/platform/virtual_platform/fx1/vp/bin/VP_FX1_V2.0
VP := $(VP_DIR)/vp_fx1_full_soc
VP_LIB := $(VP_DIR)

define make_cpp_rule
$(OBJ_DIR)/$(notdir $(basename $(1))).o: $(1)
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $$< -o $$@
endef
$(foreach src,$(CPP_SRCS),$(eval $(call make_cpp_rule,$(src))))

define make_c_rule
$(OBJ_DIR)/$(notdir $(basename $(1))).o: $(1)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $$< -o $$@
endef
$(foreach src,$(C_SRCS),$(eval $(call make_c_rule,$(src))))

define make_asm_rule
$(OBJ_DIR)/$(notdir $(basename $(1))).o: $(1)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $$< -o $$@
endef
$(foreach src,$(ASM_SRCS),$(eval $(call make_asm_rule,$(src))))

all: $(TARGET)

$(TARGET): $(CPP_OBJS) $(C_OBJS) $(ASM_OBJS) $(FX1_V2_BSP_ROOT)/linker.ld
	@mkdir -p $(OUT_DIR)
	$(CXX) $(ARCH) -nostdlib -nostartfiles -Wl,--gc-sections,-Map,$(MAP) \
	  -T$(FX1_V2_BSP_ROOT)/linker.ld $(ASM_OBJS) $(C_OBJS) $(CPP_OBJS) \
	  -lstdc++ -lm -lc -lnosys -lgcc -o $@
	$(OBJDUMP) -h -t $@ > $(OUT_DIR)/neon_graph_vp_v2.sections.txt
	$(OBJCOPY) -O binary $@ $(OUT_DIR)/neon_graph_vp_v2.bin
	@echo "Built SDK Runtime firmware: $@"
	@echo "External model (not linked into ELF): $(MODEL_NSM) ($(MODEL_BYTES) bytes)"

run: $(TARGET)
	NEON_FX1_V2_OUT_DIR=$(OUT_DIR) python3 $(APP_ROOT)/run_vp.py

clean:
	rm -rf $(OUT_DIR)

.PHONY: all run clean
-include $(CPP_OBJS:.o=.d) $(C_OBJS:.o=.d) $(ASM_OBJS:.o=.d)
