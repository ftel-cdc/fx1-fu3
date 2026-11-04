# flash_targets.cmake - custom CMake target de build+nap bootloader/app that
# len phan cung that qua ST-LINK, DUNG CHUNG cho moi app co bootloader-enabled
# target (xem NEON_MDF_BOOTLOADER_ENABLED trong cmake/mcu_variants.cmake) -
# cung 1 pattern voi target `menuconfig` (xem cmake/kconfig.cmake): custom
# target hien THANG trong CMake Tools cua VS Code (status bar "Build Target"),
# KHONG can .vscode/tasks.json rieng cho tung app nua - viet 1 lan o day,
# app nao goi neon_mdf_add_flash_targets() la co du 3 target.
#
# neon_mdf_add_flash_targets(<bootloader_target> <app_target> <openocd_cfg>)
#   bootloader_target - ten target executable bootloader (vd "bootloader",
#                        tao boi neon_mdf_add_bootloader_executable())
#   app_target         - ten target executable app (vd "blinky", tao boi
#                        neon_mdf_add_app_executable())
#   openocd_cfg        - duong dan file .cfg openocd (vd
#                        ${NEON_MDF_PATH}/cmake/openocd/WEACT_F412_openocd.cfg -
#                        file DUY NHAT trong SDK, moi app tham chieu THANG toi,
#                        khong tu copy rieng nua - xem cmake/openocd/)
#
# Sinh ra 3 target (chay duoc qua `cmake --build <dir> --target <ten>` HOAC
# chon trong CMake Tools status bar cua VS Code - khong can tasks.json).
# CHI CON DUNG 2 CACH NAP - CA 2 DEU GHI TRON BO [bootloader+header+app]
# TRONG 1 LAN, khong con cach nao nap RIENG mot phan (khong flash_bootloader/
# flash_uart/flash_all rieng le nua - du thua/de nham lan, va khong dung voi
# huong tuong lai cua FU2: kha nang FU2 khong co bootloader tu viet dang chay
# san de "nap them" app rieng duoc nhu STM32, nen luon phai nap TRON BO tu
# dau giong het idf.py flash cua ESP-IDF):
#   flash_jtag       - nap CA bootloader va app (factory) qua SWD trong 1
#                       PHIEN openocd duy nhat (khong can bootloader dang
#                       chay, khong qua UART) - giong idf.py flash cua
#                       ESP-IDF (luon ghi lai toan bo, khong tu "do" xem cho
#                       nao doi). Ghi THANH 2 LENH `program` RIENG (2026-08-10,
#                       bo hang gia dinh "app nam ngay sau bootloader" -
#                       xem CHINH SACH duoi) - bootloader.bin ghi thang tai
#                       dau vung flash, app dong goi [header+app] qua
#                       tools/build_ota_image.py roi ghi tai dia chi THAT
#                       cua no (NEON_MDF_FLASHMAP_APP_BASE neu app co
#                       flashmap.csv, khong thi NEON_MDF_APP_BASE co dinh) -
#                       KHONG con phu thuoc thu tu dong trong flashmap.csv.
#   flash_uart_stm32 - nap CA bootloader va app (factory) qua System
#                       Memory Bootloader THAT cua chip (ROM, KHONG PHAI
#                       bootloader tu viet cua SDK) qua UART, dung client TU
#                       VIET (tools/stm32_uart_flash_firmware.py - KHONG dung
#                       stm32flash, cong cu chuan nhung xac nhan khong dang
#                       tin cay tren macOS: chi thu bat tay 1 lan, khong
#                       retry). CUNG co che 2-vung nhu flash_jtag (1 lan mass
#                       erase + ghi 2 vung dia chi that trong CUNG 1 phien
#                       UART, xem client). CAN GIU NUT B0 + NOI DAY PB2->GND
#                       (thao tac tay), TU RESET qua SWD (khong can bam nut
#                       Reset - da xac nhan bam tay KHONG on dinh bang SWD).
#   serial_monitor   - mo serial monitor doc log UART (miniterm).
#
# App CO flashmap.csv (xem cmake/flashmap.cmake) co them 1 ham rieng
# neon_mdf_add_ota_flash_target(<ota_app_target> <openocd_cfg>) sinh target
# `flash_ota` - GIA LAP OTA qua SWD de test bootloader (xem ham do o duoi,
# examples/OTA_Demo/).
#
# Client UART cho protocol tu viet cua SDK nam tai tools/send_ota_uart.py;
# examples/OTA_Demo tao target flash_ota_uart/flash_ota_uart_bad de goi no.
# Hai ham build_header()/build_app_blob() dung cho cac duong SWD/full-image
# van nam rieng trong tools/build_full_image.py.
# Bootloader tu viet cua SDK (components/bootloader/neon_bootloader_main.c)
# van duoc build va ghi vao flash nhu binh thuong. Sau lan nap factory dau,
# send_ota_uart.py co the nap RIENG slot app_ota qua bootloader nay.
#
# ============================================================================
# CHINH SACH BAT BUOC cho MOI target flash-toan-bo (full-image) - hien tai la
# flash_jtag/flash_uart_stm32, va BAT KY giao thuc flash moi nao them sau nay
# (vd 1 giao thuc mang that cho FU2): sau khi flash xong, app CO flashmap.csv
# PHAI luon boot vao FACTORY (khong duoc de otadata cu tro sang mot slot/app
# KHAC song sot qua lan flash) - "flash lai binh thuong" nen luon co nghia la
# "gio chay dung ban vua nap", CHI flash_ota (xem duoi) moi duoc dat
# active_slot=1. Dat duoc bang 1 trong 2 cach, TUY co che ghi:
#   (a) Co che ghi la MASS ERASE THAT toan chip (vd flash_uart_stm32, lenh
#       Extended Erase ma dac biet 0xFFFF cua AN3155) - otadata TU DONG bi
#       xoa ve blank/invalid nhu tac dung phu cua buoc erase, khong can lam
#       gi them: NEON_OTA_ReadData() doc magic sai -> tra ve 0 (invalid),
#       boot_flashmap() coi la "chua tung OTA" -> nhay factory mac dinh (xem
#       components/bootloader/neon_bootloader_main.c). flash_uart_stm32 DA
#       THOA MAN chinh sach nay san, KHONG can sua gi.
#   (b) Co che ghi CHI xoa vung/sector cua chinh anh dang ghi (vd flash_jtag
#       qua `openocd program` - chi erase sector anh de len, KHONG dung toi
#       otadata vi anh factory theo flashmap dung lai DUNG truoc otadata) -
#       BAT BUOC tu ghi kem 1 otadata factory (active_slot=0, state=VALID,
#       boot_attempts=0, dung `tools/build_otadata_image.py 0 0 0 <out>`)
#       trong CUNG 1 giao dich/phien voi anh chinh, xem cach flash_jtag lam o
#       duoi (2 lenh `program` cua openocd trong 1 phien).
# Neu them 1 target flash-toan-bo moi: xac dinh co che ghi cua no roi ap dung
# (a) hoac (b) - DUNG gia dinh "ghi ca bootloader+app la da xoa het flash",
# da xac nhan SAI voi flash_jtag (chinh la bug that gap 3 lan trong 1 phien
# lam viec truoc khi sua o day).
# ============================================================================
#
# NEON_MDF_SERIAL_PORT la CACHE STRING chi dung lam GIA TRI MAC DINH luc
# configure (vd -DNEON_MDF_SERIAL_PORT=... neu cam lai cap bi doi ten cong
# co dinh mai). flash_uart_stm32/serial_monitor o duoi doc cong THAT
# qua BIEN MOI TRUONG shell $NEON_MDF_SERIAL_PORT (fallback ve gia tri cache
# nay neu bien moi truong chua dat) - KHONG con nhung cung cap luc configure
# nua, de vscode-extension/ (xem src/commands/flash.ts) co the doi cong moi
# lan chay targets nay (qua ShellExecution env) ma khong can reconfigure lai
# CMake.
set(NEON_MDF_SERIAL_PORT "/dev/cu.usbmodem102" CACHE STRING
  "Cong serial MAC DINH (fallback) cua ST-Link V2.1 VCP. Ghi de tam thoi qua bien moi truong NEON_MDF_SERIAL_PORT.")

function(neon_mdf_add_flash_targets bootloader_target app_target openocd_cfg)
  find_program(NEON_MDF_OPENOCD openocd REQUIRED)
  find_program(NEON_MDF_PYTHON3 python3 REQUIRED)

  # Ghi lai CHINH XAC ten bootloader_target/app_target vao 1 file manifest
  # (dung boi vscode-extension/src/commands/debug.ts) - TRUOC day debug.ts tu
  # DOAN "executable nao la ban OTA" bang cach kiem tra ten co ket thuc bang
  # "_ota" khong (vd "freertos_blinky_ota") - bug that: examples/OTA_Demo co
  # them "ota_demo_ota_bad" (KHONG ket thuc "_ota", ket thuc "_bad") dung de
  # test rollback, debug.ts doan sai thanh flash_jtag trong khi executable do
  # thuc ra dung chung linker script/slot voi "ota_demo_ota" - flash 1 anh
  # nhung debug voi symbol/entry point cua anh KHAC. Ghi that ra file thay vi
  # doan - "factory"/"bootloader" o day, "ota" duoc neon_mdf_add_ota_flash_target()
  # APPEND them (goi SAU ham nay trong moi CMakeLists.txt thuc te).
  file(WRITE "${CMAKE_BINARY_DIR}/neon_mdf_debug_targets.txt"
    "bootloader=${bootloader_target}\nfactory=${app_target}\n")

  if(NOT TARGET serial_monitor)
    add_custom_target(serial_monitor
      COMMAND bash -c "${NEON_MDF_PYTHON3} -m serial.tools.miniterm \"\${NEON_MDF_SERIAL_PORT:-${NEON_MDF_SERIAL_PORT}}\" 115200"
      USES_TERMINAL
      VERBATIM
      COMMENT "Serial Monitor - doc log UART qua bien moi truong NEON_MDF_SERIAL_PORT, mac dinh ${NEON_MDF_SERIAL_PORT} (Ctrl+] de thoat)"
    )
  endif()

  # Dia chi THAT de ghi bootloader (dau vung flash) va app (factory) - tinh 1
  # LAN o day, dung chung boi flash_jtag/flash_uart_stm32 ben duoi (2026-08-10,
  # thay cho cach ghep 1 file .bin lien tuc gia dinh app nam ngay sau
  # bootloader - gia dinh do bat flashmap.csv PHAI liet ke "app" la dong DAU
  # TIEN, khac esp-idf partitions.csv that su linh hoat thu tu partition).
  # App CO flashmap.csv: dia chi app lay THAT tu NEON_MDF_FLASHMAP_APP_BASE
  # (tinh boi tools/gen_flashmap.py tu dung thu tu THAT trong file, khong
  # quan trong dong nao dung truoc). App KHONG co flashmap.csv: dung dia chi
  # co dinh NEON_MDF_APP_BASE (tinh thang tu bootloader.size trong YAML).
  math(EXPR NEON_MDF_FLASH_BASE "${NEON_MDF_APP_BASE} - ${NEON_MDF_BOOTLOADER_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
  if(DEFINED NEON_MDF_FLASHMAP_APP_BASE)
    set(_neon_mdf_app_write_base ${NEON_MDF_FLASHMAP_APP_BASE})
  else()
    set(_neon_mdf_app_write_base ${NEON_MDF_APP_BASE})
  endif()

  if(NOT TARGET flash_jtag)
    # Dong goi app thanh [header 16 byte][dem][app.bin] qua
    # tools/build_ota_image.py (dung LAI ham build_app_blob() - KHONG viet
    # lai logic dong goi/CRC rieng cho duong nay, xem file do) -
    # bootloader.bin ghi THANG (da la 1 file .bin hop le tu chinh build cua
    # no, khong can dong goi gi them) - 2 lenh `program` RIENG cua openocd
    # trong CUNG 1 phien, KHONG con ghep chung 1 file .bin lien tuc.
    if(DEFINED NEON_MDF_FLASHMAP_OTADATA_BASE)
      # App CO flashmap.csv (OTA) - flash_jtag chi ghi partition "bootloader"
      # + "app" (factory), KHONG dung toi "app_ota"/"otadata" - nhung bootloader
      # lai quyet dinh boot vao dau dua tren otadata.active_slot, con sot lai
      # tu lan test OTA truoc (cua CHINH app nay hoac 1 app KHAC tung dung
      # chung board that) - neu khong reset, flash_jtag co the "thanh cong"
      # (Verified OK that) ma board VAN boot vao slot OTA cu, KHONG PHAI ban
      # factory vua nap - da gap that nhieu lan luc test tren phan cung.
      # Fix (xem CHINH SACH day du o dau file): LUON ghi kem otadata ve trang
      # thai factory (active_slot=0, state=VALID, boot_attempts=0) trong CUNG
      # 1 phien openocd - day la truong hop (b) cua chinh sach (co che ghi
      # chi xoa sector cua anh, khong tu dung toi otadata). flash_uart_stm32
      # KHONG can khoi nay - no la truong hop (a), mass erase that cua no da
      # tu lo otadata roi (xem mass_erase() trong stm32_uart_flash_firmware.py).
      add_custom_target(flash_jtag
        COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/build_ota_image.py
                $<TARGET_FILE:${app_target}>.bin
                ${NEON_MDF_IMAGE_HEADER_SIZE}
                $<TARGET_FILE_DIR:${app_target}>/${app_target}_blob.bin
        COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/build_otadata_image.py
                0 0 0
                $<TARGET_FILE_DIR:${app_target}>/otadata_factory.bin
        COMMAND ${NEON_MDF_OPENOCD} -f ${openocd_cfg}
                -c "program $<TARGET_FILE:${bootloader_target}>.bin ${NEON_MDF_FLASH_BASE} verify"
                -c "program $<TARGET_FILE_DIR:${app_target}>/${app_target}_blob.bin ${_neon_mdf_app_write_base} verify"
                -c "program $<TARGET_FILE_DIR:${app_target}>/otadata_factory.bin ${NEON_MDF_FLASHMAP_OTADATA_BASE} verify reset exit"
        DEPENDS ${bootloader_target} ${app_target}
        USES_TERMINAL
        VERBATIM
        COMMENT "Nap bootloader + app (factory, dia chi THAT tu flashmap.csv) + RESET otadata ve factory (active_slot=0,state=VALID) trong CUNG 1 phien SWD - dam bao luon boot dung ban vua nap"
      )
    else()
      add_custom_target(flash_jtag
        COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/build_ota_image.py
                $<TARGET_FILE:${app_target}>.bin
                ${NEON_MDF_IMAGE_HEADER_SIZE}
                $<TARGET_FILE_DIR:${app_target}>/${app_target}_blob.bin
        COMMAND ${NEON_MDF_OPENOCD} -f ${openocd_cfg}
                -c "program $<TARGET_FILE:${bootloader_target}>.bin ${NEON_MDF_FLASH_BASE} verify"
                -c "program $<TARGET_FILE_DIR:${app_target}>/${app_target}_blob.bin ${_neon_mdf_app_write_base} verify reset exit"
        DEPENDS ${bootloader_target} ${app_target}
        USES_TERMINAL
        VERBATIM
        COMMENT "Nap CA bootloader lan app qua SWD trong 1 phien openocd (2 lenh program rieng, dia chi that) - giong idf.py flash, khong can UART/bootloader dang chay"
      )
    endif()
  endif()

  if(NOT TARGET flash_uart_stm32)
    # Ghi qua System Memory Bootloader THAT cua chip (ROM cua STM32, KHONG
    # PHAI bootloader tu viet - xem AN2606/AN3155 cua ST) bang client TU
    # VIET (tools/stm32_uart_flash_firmware.py) - client nay nhan NHIEU cap
    # (file, dia chi) va ghi trong CUNG 1 phien UART (1 lan dong bo + 1 lan
    # mass erase + nhieu vong ghi) - CUNG co che 2-vung nhu flash_jtag,
    # KHONG con ghep 1 file .bin lien tuc gia dinh app nam sat sau bootloader.
    # ROM bootloader nay LUON CO SAN bat ke flash dang chua gi (giong ROM
    # bootloader cua ESP32 ma esptool.py dung) - khong can bootloader tu
    # viet dang chay/hop tac gi ca.
    #
    # otadata: target nay la truong hop (a) cua CHINH SACH o dau file - client
    # dung Extended Erase (ma 0xFFFF = mass erase TOAN CHIP that, xem
    # mass_erase() trong stm32_uart_flash_firmware.py) TRUOC khi ghi anh, nen
    # otadata TU DONG ve blank/invalid, bootloader mac dinh coi la factory -
    # KHONG can tu ghi kem otadata nhu flash_jtag lam (truong hop (b)).
    #
    # KHONG dung `stm32flash` (cong cu chuan cua cong dong) - da xac nhan
    # THAT tren may nay: no chi thu gui byte dong bo (0x7F) DUNG 1 LAN roi
    # bao loi ngay ("Failed to init device") neu khong trung dung khoanh
    # khac bootloader san sang, KHONG retry nhu can thiet. Client tu viet
    # gui lap lai cho toi khi thanh cong - da test THAT nhieu lan, luon
    # thanh cong.
    #
    # CHI 1 THAO TAC TAY (khong can bam nut Reset nua): NOI DAY NHAY PB2 ->
    # GND (KHONG PHAI nut bam - la chan tren header GPIO) + GIU NUT B0 TREN
    # BOARD, roi target nay TU RESET qua SWD (ghi thang AIRCR.SYSRESETREQ -
    # da xac nhan THAT dang tin cay hon han bam nut Reset vat ly: nut Reset
    # tay cho ket qua KHONG ON DINH tren board nay, co luc vao dung System
    # Bootloader co luc khong du voi CUNG 1 quy trinh - reset qua SWD thi
    # LUON thanh cong qua nhieu lan test).
    #
    # STM32F412 dung CA BOOT0 LAN BOOT1 (=PB2) de chon che do boot (khac
    # nhieu dong F4 khac chi dung BOOT0) - va PB2 TRUNG voi chan LED tren
    # board WeAct nay, bi THA NOI theo thiet ke (van de da biet cua dong
    # board WeAct Blackpill/F4x1, khong phai loi cua SDK nay) - neu khong
    # noi GND, BOOT1 doc sai gia tri ngau nhien moi lan reset, co luc vao
    # dung System Bootloader co luc khong.
    #
    # Sau khi ghi xong: THA B0 + RUT day PB2->GND (PB2 la chan LED cua app,
    # de noi GND mai se can LED), roi reset lai de chip chay app that.
    add_custom_target(flash_uart_stm32
      COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/build_ota_image.py
              $<TARGET_FILE:${app_target}>.bin
              ${NEON_MDF_IMAGE_HEADER_SIZE}
              $<TARGET_FILE_DIR:${app_target}>/${app_target}_blob.bin
      COMMAND bash -c "echo '=== NOI DAY PB2->GND + GIU NUT B0 TREN BOARD (KHONG CAN BAM NUT RESET - tu dong reset qua SWD) ==='; ${NEON_MDF_OPENOCD} -f ${openocd_cfg} -c init -c \"mww 0xE000ED0C 0x05FA0004\" -c shutdown >/dev/null 2>&1; PORT=\"\${NEON_MDF_SERIAL_PORT:-${NEON_MDF_SERIAL_PORT}}\"; ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/stm32_uart_flash_firmware.py \"$PORT\" $<TARGET_FILE:${bootloader_target}>.bin ${NEON_MDF_FLASH_BASE} $<TARGET_FILE_DIR:${app_target}>/${app_target}_blob.bin ${_neon_mdf_app_write_base}"
      DEPENDS ${bootloader_target} ${app_target}
      USES_TERMINAL
      VERBATIM
      COMMENT "Nap CA bootloader lan app qua System Memory Bootloader THAT cua STM32 (UART, KHONG phai bootloader tu viet) - CAN NOI PB2->GND + GIU NUT B0 LUC RESET"
    )
  endif()
endfunction()

# neon_mdf_add_ota_flash_target(<ota_app_target> <openocd_cfg> [flash_target])
#   ota_app_target - ten target executable link VOI linker script cua
#                    partition "ota" trong flashmap.csv (vd link
#                    -T${CMAKE_BINARY_DIR}/flashmap/app_ota.ld) - xem
#                    examples/OTA_Demo/CMakeLists.txt.
#   openocd_cfg    - giong het tham so cung ten trong neon_mdf_add_flash_targets().
#   flash_target   - tuy chon, mac dinh `flash_ota`; cho phep vi du tao them
#                    target nhu `flash_ota_bad` ma van dung chung logic dong goi.
#
# Sinh target `flash_ota` - GIA LAP OTA qua SWD (chua co duong mang that,
# xem plan file + memory project_fu2_bootloader_ota_direction) - ghi ANH OTA
# + OTADATA (danh dau pending, chua xac nhan) trong 1 phien openocd, dung
# de TEST co che verify/switch/rollback cua bootloader tren phan cung that
# ma khong can cho toi khi co transport mang thuc su. CHI goi duoc SAU khi
# app da co flashmap.csv (can NEON_MDF_FLASHMAP_APP_OTA_BASE/OTADATA_BASE,
# xem cmake/flashmap.cmake) - FATAL_ERROR ro rang neu goi thieu.
function(neon_mdf_add_ota_flash_target ota_app_target openocd_cfg)
  find_program(NEON_MDF_OPENOCD openocd REQUIRED)
  find_program(NEON_MDF_PYTHON3 python3 REQUIRED)

  if(NOT DEFINED NEON_MDF_FLASHMAP_APP_OTA_BASE OR NOT DEFINED NEON_MDF_FLASHMAP_OTADATA_BASE)
    message(FATAL_ERROR "neon_mdf_add_ota_flash_target: thieu NEON_MDF_FLASHMAP_APP_OTA_BASE/OTADATA_BASE - goi neon_mdf_process_flashmap() (tu dong qua neon-mdf/CMakeLists.txt khi co flashmap.csv) TRUOC")
  endif()

  set(_neon_mdf_ota_flash_target flash_ota)
  if(ARGC GREATER 2)
    set(_neon_mdf_ota_flash_target ${ARGV2})
  endif()

  # APPEND vao manifest da tao boi neon_mdf_add_flash_targets() (xem comment
  # o do) - ham nay LUON duoc goi SAU trong moi CMakeLists.txt thuc te cua
  # SDK (Blinky/FreeRTOS_Blinky/OTA_Demo deu theo dung thu tu nay).
  if(_neon_mdf_ota_flash_target STREQUAL "flash_ota")
    file(APPEND "${CMAKE_BINARY_DIR}/neon_mdf_debug_targets.txt" "ota=${ota_app_target}\n")
  endif()

  if(NOT TARGET ${_neon_mdf_ota_flash_target})
    add_custom_target(${_neon_mdf_ota_flash_target}
      COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/build_ota_image.py
              $<TARGET_FILE:${ota_app_target}>.bin
              ${NEON_MDF_IMAGE_HEADER_SIZE}
              $<TARGET_FILE_DIR:${ota_app_target}>/${ota_app_target}_blob.bin
      COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/build_otadata_image.py
              1 1 0
              $<TARGET_FILE_DIR:${ota_app_target}>/otadata_pending.bin
      COMMAND ${NEON_MDF_OPENOCD} -f ${openocd_cfg}
              -c "program $<TARGET_FILE_DIR:${ota_app_target}>/${ota_app_target}_blob.bin ${NEON_MDF_FLASHMAP_APP_OTA_BASE} verify"
              -c "program $<TARGET_FILE_DIR:${ota_app_target}>/otadata_pending.bin ${NEON_MDF_FLASHMAP_OTADATA_BASE} verify reset exit"
      DEPENDS ${ota_app_target}
      USES_TERMINAL
      VERBATIM
      COMMENT "GIA LAP OTA qua SWD (${_neon_mdf_ota_flash_target}): ghi [header+${ota_app_target}] vao slot OTA + otadata pending"
    )
  endif()
endfunction()
