// Clay
#include <stdio.h>
#define CLAY_IMPLEMENTATION
#include "clay_include.h"

void fixClayLimits(void) {
  // int32_t Clay__defaultMaxElementCount = 8192;
  // int32_t Clay__defaultMaxMeasureTextWordCacheCount = 16384;
  Clay__defaultMaxElementCount = 4096;
  Clay__defaultMaxMeasureTextWordCacheCount = 16384;
}
