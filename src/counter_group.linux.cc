#include "counter_group.h"

#include <cassert>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cerrno>

#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <asm/unistd.h>
#include <unistd.h>
#include <linux/perf_event.h>

#include <stdexcept>
#include <array>

namespace node_perf_counters
{
    /** Zero fills a POD type, such as a structure or union. */
    template<class T>
    void bzero(T& s)
    {
        static_assert(std::is_standard_layout<T>::value, "s must be a is_standard_layout type");
        std::memset(&s, 0, sizeof(T));
    }

    int perf_event_open(
        perf_event_attr *hw_event,
        pid_t pid,
        int cpu,
        int group_fd,
        unsigned long flags)
    {
        int ret;
        ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
        return ret;
    }

    int begin_event(counter c, int32_t parent)
    {
        perf_event_attr pe;
        bzero(pe);

        pe.size = sizeof(pe);
        pe.disabled = 1;
        pe.exclude_kernel = 1;
        pe.exclude_hv = 1;
        pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;

        if (parent < 0) {
            // the counter should always be on the CPU if at all possible.
            pe.pinned = 1;
        }

        switch (c) {
            case counter::cycles:
                pe.type = PERF_TYPE_HARDWARE;
                pe.config = PERF_COUNT_HW_CPU_CYCLES;
                break;

            case counter::instructions:
                pe.type = PERF_TYPE_HARDWARE;
                pe.config = PERF_COUNT_HW_INSTRUCTIONS;
                break;

            case counter::task_clock:
                pe.type = PERF_TYPE_SOFTWARE;
                pe.config = PERF_COUNT_SW_TASK_CLOCK;
                break;

            case counter::cpu_clock:
                pe.type = PERF_TYPE_SOFTWARE;
                pe.config = PERF_COUNT_SW_CPU_CLOCK;
                break;

            case counter::context_switches:
                pe.type = PERF_TYPE_SOFTWARE;
                pe.config = PERF_COUNT_SW_CONTEXT_SWITCHES;
                break;

            case counter::branch_instructions:
                pe.type = PERF_TYPE_HARDWARE;
                pe.config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
                break;

            case counter::branch_misses:
                pe.type = PERF_TYPE_HARDWARE;
                pe.config = PERF_COUNT_HW_BRANCH_MISSES;
                break;

            case counter::cache_references:
                pe.type = PERF_TYPE_HARDWARE;
                pe.config = PERF_COUNT_HW_CACHE_REFERENCES;
                break;

            case counter::cache_misses:
                pe.type = PERF_TYPE_HARDWARE;
                pe.config = PERF_COUNT_HW_CACHE_MISSES;
                break;

            default:
                // Invalid counter
                return -1;
        }

        return perf_event_open(&pe, 0, -1, parent, 0);
    }

    std::uint64_t event_id(int fd) {
        assert(fd != -1);

        std::uint64_t event_id;
        ioctl(fd, PERF_EVENT_IOC_ID, &event_id);

        return event_id;
    }

    /** ------------------------------------------------------------------------------------------- */

    static const size_t MAX_COUNTERS = 64;

    struct read_format
    {
        struct counter_value {
            uint64_t value;
            uint64_t id;
        };

        uint64_t nr;
        std::array<counter_value, MAX_COUNTERS> values;
    };

    struct counter_state
    {
        int32_t fd;
        uint64_t event_id;
        counter type;
    };

    struct counter_group::impl {
        int32_t gfd;
        std::unordered_map<uint64_t, counter_state> counters;
        read_format read_buffer;
    };

    /** ------------------------------------------------------------------------------------------- */

    counter_group::counter_group(const std::vector<counter>& counters)
        : _pimpl{ new impl{ -1, {}, {} } }
    {
        if (counters.size() > MAX_COUNTERS) {
            throw std::runtime_error("Maximum number of counters exceeded");
        }

        auto& impl = *_pimpl;
        for (counter c : counters) {
            const int32_t fd = begin_event(c, impl.gfd);

            if (fd < 0) {
                throw std::runtime_error(std::strerror(errno));
            } else if (impl.gfd < 0) {
                impl.gfd = fd;
            }

            uint64_t eid = event_id(fd);
            impl.counters.emplace(eid, counter_state{ fd, eid, c });
        }

        if (impl.gfd >= 0) {
            ioctl(impl.gfd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
        }
    }

    int32_t counter_group::id() {
        return _pimpl->gfd;
    }

    void counter_group::read(std::function<void(counter, std::int64_t)>&& cb)
    {
        auto& [gfd, counters, r] = *_pimpl;

        if (counters.size() == 0) return;

        if (::read(gfd, &r, sizeof(r)) == 0) {
            throw std::runtime_error(
                std::string("Failed to read counters (") + std::strerror(errno) + ")");
        }

        assert(r.nr == counters.size() && "Incorrect counter count");

        for (uint64_t i = 0; i < r.nr; i++) {
            auto [value, event_id] = r.values[i];

            assert(counters.count(event_id) == 1 && "Counter not registered");
            cb(counters[event_id].type, value);
        }
    }

    void counter_group::reset()
    {
        auto gfd = _pimpl->gfd;
        if (gfd >= 0) {
            ioctl(gfd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
        }
    }

    counter_group::~counter_group()
    {
        auto gfd = _pimpl->gfd;
        if (gfd >= 0) {
            ioctl(gfd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
            close(gfd);
        }
        _pimpl->counters.clear();
    }
}