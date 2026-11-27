# Host tests of the camera HAL. Needs only gcc and make: no cross toolchain, no hardware.
DRV := ../../components/driver
RT := ../../components/neon_camera_runtime
ARCH := ../../arch/RISCV/Device/FTEL/FX1/Include
CC ?= gcc
.DEFAULT_GOAL := all
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Wconversion -Werror -g -O0 -Ihost -I$(DRV) -I$(RT)/include -I$(ARCH) -I.
BUILD ?= build

PHY_SRC := $(DRV)/phy/neon_phy.c $(DRV)/phy/neon_phy_mipi_dphy.c \
           $(DRV)/phy/platform/fx1/neon_phy_fx1.c $(DRV)/phy/platform/fx1/fx1_csidphy.c
# test_neon_csi.c and test_neon_camera_runtime.c still use the neon_mipi link driver that the PHY layer
# replaced: they are kept out of TESTS until they are rewritten.
CSI_SRC := $(DRV)/csi/platform/fx1/neon_csi.c host/csi_mmio.c
ISP_SRC := $(DRV)/isp/platform/common/neon_isp.c $(DRV)/isp/platform/fx1/fx1_isp_caps.c $(DRV)/isp/platform/fx1/fx1_isp_profile.c \
            $(DRV)/isp/platform/fx1/fx1_isp_simulation.c
RT_SRC := $(RT)/src/camera_legacy.c $(RT)/src/camera_legacy_config.c $(RT)/algorithms/camera_3a_reference.c
SESSION_SRC := $(RT)/src/camera_runtime.c $(RT)/src/camera_config.c $(RT)/src/camera_events.c \
  $(RT)/src/camera_scheduler.c $(RT)/src/camera_buffers.c $(RT)/src/camera_history.c $(RT)/src/camera_recovery.c
CLAIM_SRC := $(DRV)/isp/platform/common/neon_isp_claim.c
TESTS := $(BUILD)/test_neon_isp $(BUILD)/test_ftel_isp_contract $(BUILD)/test_camera_session $(BUILD)/test_fx1_camera_irq $(BUILD)/test_camera_freertos

$(BUILD)/test_camera_session: test_camera_session.c test_common.h $(SESSION_SRC) $(CLAIM_SRC) $(wildcard $(RT)/include/*.h) $(RT)/src/camera_internal.h $(DRV)/isp/neon_isp_session.h $(DRV)/isp/neon_isp_claim.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ test_camera_session.c $(SESSION_SRC) $(CLAIM_SRC)

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

# CSR/MAS contract checks with memory-backed MMIO, independent of the VP version.
FTEL := ../../components/driver/isp
FTEL_CONTRACT_SRC := $(FTEL)/common/ftel_isp_common.c $(FTEL)/stats/aec/ftel_isp_aec.c $(FTEL)/io/dma/ftel_isp_dma.c
$(BUILD)/test_ftel_isp_contract: test_ftel_isp_contract.c host/ftel_isp_mmio.h $(FTEL_CONTRACT_SRC) $(wildcard $(FTEL)/common/*.h) $(wildcard $(FTEL)/registers/*.h) $(ARCH)/fx1_isp/fx1_isp_csr.h $(FTEL)/bayer\ domain/ifmt/ftel_isp_ifmt.c $(FTEL)/bayer\ domain/lsc/ftel_isp_lsc.c $(FTEL)/rgb\ domain/ccm/ftel_isp_ccm.c $(FTEL)/yuv\ domain/cnf/ftel_isp_cnf.c | $(BUILD)
	$(CC) -std=c11 -Wall -Wextra -Werror -g -I. -I$(FTEL) -I$(ARCH) -DISP_BASE=0x11000000u \
	  -include host/ftel_isp_mmio.h -ffunction-sections -fdata-sections -Wl,--gc-sections \
	  -o $@ test_ftel_isp_contract.c $(FTEL_CONTRACT_SRC) \
	  "$(FTEL)/bayer domain/ifmt/ftel_isp_ifmt.c" "$(FTEL)/bayer domain/lsc/ftel_isp_lsc.c" \
	  "$(FTEL)/rgb domain/ccm/ftel_isp_ccm.c" "$(FTEL)/yuv domain/cnf/ftel_isp_cnf.c"

FX1_SESSION_HEADERS := $(wildcard $(DRV)/isp/platform/fx1/fx1_isp_*.h) $(ARCH)/fx1_isp/fx1_isp_csr.h $(DRV)/isp/neon_isp_session.h $(wildcard $(RT)/include/*.h)
FX1_SESSION_SRC := $(DRV)/isp/platform/fx1/fx1_isp_irq.c $(DRV)/isp/platform/fx1/fx1_isp_backend.c $(DRV)/isp/platform/fx1/fx1_isp_profile_txn.c
$(BUILD)/test_fx1_camera_irq: test_fx1_camera_irq.c host/isp_runtime_mmio.h $(FX1_SESSION_HEADERS) $(FX1_SESSION_SRC) $(SESSION_SRC) $(CLAIM_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ test_fx1_camera_irq.c $(FX1_SESSION_SRC) $(SESSION_SRC) $(CLAIM_SRC)

CAMERA_SERVICE := ../../rtos/services/camera
$(BUILD)/test_camera_freertos: test_camera_freertos.c host/isp_runtime_mmio.h $(wildcard host/freertos/*.h) $(CAMERA_SERVICE)/neon_camera_freertos.c $(CAMERA_SERVICE)/include/neon_camera_freertos.h $(FX1_SESSION_HEADERS) $(FX1_SESSION_SRC) $(SESSION_SRC) $(CLAIM_SRC) | $(BUILD)
	$(CC) -Ihost/freertos $(CFLAGS) -I$(CAMERA_SERVICE)/include -o $@ test_camera_freertos.c $(FX1_SESSION_SRC) $(SESSION_SRC) $(CLAIM_SRC)
