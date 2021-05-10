#pragma once

#include <vector>
#include <functional>
#include <memory>

namespace node_perf_counters
{
    /** Counter types */
    enum class counter
    {
        cycles = 0,

        instructions = 1,

        task_clock = 2,

        cpu_clock = 3,

        context_switches = 4,

        branch_instructions = 5,

        branch_misses = 6,

        cache_references = 7,

        cache_misses = 8,
    };

    /** A Group of counters with RAII semantics */
    struct counter_group
    {
        counter_group(const std::vector<counter>& counters);

        /** A unique identifier for this group */
        std::int32_t id();

        /** Read the current values of all counters */
        void read(std::function<void(counter, std::int64_t)>&& cb);

        /** Reset all counters to zero */
        void reset();

        ~counter_group();

      private:
        struct impl;

        std::unique_ptr<impl> _pimpl;
    };
}
