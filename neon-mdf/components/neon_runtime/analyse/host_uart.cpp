// ============================================================
// NEON STUDIO — host-side uart_puts / uart_putc
//
// The report and CLI code call uart_puts()/uart_putc() directly, which on
// bare metal are provided by the target's support code (see
// device/qemu_mps3_an547/baremetal/semihosting_uart.c, or the VP_FX1_V2.0
// SDK's uart.c). A host build links neither, so every host build of
// neon-eval failed at link with
//
//     undefined reference to `uart_puts'
//
// even though every translation unit compiled cleanly — the tests link
// fine and hide it, so it only shows up when building the tools.
//
// Routing them to stdout keeps one output path on both host and device
// rather than making callers choose. Compiled ONLY for host builds (see
// analyse/CMakeLists.txt); a cross build gets the real UART instead.
// ============================================================

#include <cstdio>
#include <cstdint>

extern "C" void uart_putc(char c) {
    std::fputc(c, stdout);
}

extern "C" void uart_puts(const char* s) {
    if (!s) return;
    std::fputs(s, stdout);
}

extern "C" void uart_put_u32(const char* prefix, uint32_t value, const char* suffix) {
    if (prefix) std::fputs(prefix, stdout);
    std::printf("%u", value);
    if (suffix) std::fputs(suffix, stdout);
}
