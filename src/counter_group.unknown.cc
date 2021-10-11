#include "counter_group.h"
#include <stdexcept>

namespace node_perf_counters
{
    struct counter_group::impl { };

    /** ------------------------------------------------------------------------------------------- */

    counter_group::counter_group(const std::vector<counter>&)
        : _pimpl{ new impl() }
    {
        throw std::runtime_error("Unsupported platform");
    }

    int32_t counter_group::id() { return -1; }

    void counter_group::read(std::function<void(counter, std::int64_t)>&&) { }

    void counter_group::reset() { }

    counter_group::~counter_group() { }
}