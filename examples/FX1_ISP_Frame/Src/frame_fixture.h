/* Hợp đồng DDR của app kiểm chứng M1, nằm ngoài vùng ELF/stack của BSP VP. */
#ifndef FX1_FRAME_FIXTURE_H
#define FX1_FRAME_FIXTURE_H
#define FRAME_WIDTH 64u
#define FRAME_HEIGHT 48u
#define FRAME_INPUT_STRIDE 160u
#define FRAME_Y_STRIDE 80u
#define FRAME_UV_STRIDE 96u
#define FRAME_RAW_BASE 0x81000000u
#define FRAME_OUTPUT_BASE 0x88000000u
#define FRAME_ARENA_BYTES 0x10000u
#define FRAME_SLOT_BYTES 0x4000u
#define FRAME_GUARD_BYTES 64u
#define FRAME_UV_OFFSET 0x2000u
#endif
