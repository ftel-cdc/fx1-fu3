add_library(neon_analyse STATIC
    StaticAnalyzer.cpp
    MemBWAnalyzer.cpp
    DMAProfiler.cpp
    MeasuredProfiler.cpp
    PerfReport.cpp
    compiler/CompilerMeasure.cpp
)

# uart_puts()/uart_putc() come from the target's startup code on a bare-metal
# build. A hosted build (host, or openc906 riscv64 Linux) has neither, so supply
# stdout-backed versions or every link fails with "undefined reference to `uart_puts'".
if(NEON_HOSTED)
    target_sources(neon_analyse PRIVATE host_uart.cpp)
endif()

target_include_directories(neon_analyse PUBLIC
    "${NEON_INCLUDE_ROOT}"
)

set_target_properties(neon_analyse PROPERTIES
    CXX_STANDARD 17
    POSITION_INDEPENDENT_CODE ON
)
