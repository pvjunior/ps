#pragma once

#include "LoaderTypes.h"

#include <cstdint>

// Plans an absolute load when base equals the H record's original start
// address, or a relocated load when base differs from it. memorySize is the
// number of addressable bytes in the target memory.
LoadPlanResult planLoad(const ParsedObject& object, std::uint32_t base,
                        std::uint32_t memorySize);
