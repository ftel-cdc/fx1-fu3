#pragma once
// C++-only extensions for platform glue that already speaks the runtime's C++
// types (F14). NOT part of the stable C ABI.
#include <neon/c_runtime_api.h>
#include "../graph/GraphRunner.h"

int NeonGraphSetObserver(NeonGraphHandle h, GraphObserver* observer);
int NeonGraphAttachProfilers(NeonGraphHandle h, MeasuredProfiler* profiler, DMAProfiler* dma_profiler);
