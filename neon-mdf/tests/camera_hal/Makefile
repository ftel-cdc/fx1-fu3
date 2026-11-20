# Host tests of the camera HAL. Needs only gcc and make: no cross toolchain, no hardware.
DRV := ../../components/driver
RT := ../../components/neon_camera_runtime
ARCH := ../../arch/RISCV/Device/FTEL/FX1/Include
CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Werror -g -O0 -Ihost -I$(DRV) -I$(RT) -I$(ARCH) -I.
BUILD ?= build

PHY_SRC := $(DRV)/phy/neon_phy.c $(DRV)/phy/neon_phy_mipi_dphy.c \
           $(DRV)/phy/platform/fx1/neon_phy_fx1.c $(DRV)/phy/platform/fx1/fx1_csidphy.c
# test_neon_csi.c and test_neon_camera_runtime.c still use the neon_mipi link driver that the PHY layer
# replaced: they are kept out of TESTS until they are rewritten.
CSI_SRC := $(DRV)/csi/platform/fx1/neon_csi.c host/csi_mmio.c
ISP_SRC := $(DRV)/isp/platform/common/neon_isp.c $(DRV)/isp/platform/fx1/fx1_isp_caps.c $(DRV)/isp/platform/fx1/fx1_isp_profile.c \
            $(DRV)/isp/platform/fx1/fx1_isp_simulation.c
RT_SRC := $(RT)/src/neon_camera_runtime.c $(RT)/src/neon_camera_runtime_init.c $(RT)/3a/neon_camera_3a_reference.c
TESTS := $(BUILD)/test_neon_isp

.PHONY: all test fuzz clean
all: $(TESTS)

test: all
	@set -e; for t in $(TESTS); do echo "== $$t"; $$t; done

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/test_neon_csi: test_neon_csi.c test_common.h host/csi_mmio.h host/fx1_fw.h $(PHY_SRC) $(CSI_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ test_neon_csi.c $(PHY_SRC) $(CSI_SRC)

$(BUILD)/test_neon_isp: test_neon_isp.c test_common.h $(ISP_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ test_neon_isp.c $(ISP_SRC)

$(BUILD)/test_neon_camera_runtime: test_neon_camera_runtime.c test_common.h host/csi_mmio.h host/fx1_fw.h $(PHY_SRC) $(CSI_SRC) $(ISP_SRC) $(RT_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ test_neon_camera_runtime.c $(PHY_SRC) $(CSI_SRC) $(ISP_SRC) $(RT_SRC)

# Random model-based tests, with address and undefined-behaviour sanitizers. Slower than the unit tests.
FUZZ_FLAGS := -std=c11 -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all -Ihost -I$(DRV) -I$(ARCH)
fuzz: | $(BUILD)
	$(CC) $(FUZZ_FLAGS) -o $(BUILD)/fuzz_neon_isp fuzz_neon_isp.c $(ISP_SRC)
	$(CC) $(FUZZ_FLAGS) -o $(BUILD)/fuzz_neon_csi fuzz_neon_csi.c $(PHY_SRC) $(CSI_SRC)
	$(BUILD)/fuzz_neon_isp 20000
	$(BUILD)/fuzz_neon_csi

clean:
	rm -rf $(BUILD)
