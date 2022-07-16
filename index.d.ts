/*
 * Copyright 2021 Ray Glover
 * Licensed under the Apache License, Version 2.0
 */

declare namespace cpuPerfCounters
{
    /**
     * Supported Hardware and software Counters.
     *
     * @remarks Not all counters may be available on the current hardware
     * configuration and some some combinations of counters may cause errors on
     * some CPUs.
     */
    enum CounterType {
        /** A count of total cycles */
        CYCLES,

        /** A count of retired instructions */
        INSTRUCTIONS,

        /** A clock count specific to the task that is running. (nanoseconds) */
        TASK_CLOCK,

        /** The CPU clock count, a high-resolution per-CPU timer. (nanoseconds) */
        CPU_CLOCK,

        /** A count of context switches. */
        CONTEXT_SWITCHES,

        /** A count of retired branch instructions */
        BRANCH_INSTRUCTIONS,

        /** A count of mispredicted branch instructions. */
        BRANCH_MISSES,

        /**
         * A count of cache accesses. Usually this indicates Last Level Cache accesses
         * but this may vary depending on the CPU.
         */
        CACHE_REFERENCES,

        /** A count of cache misses. Usually this indicates Last Level Cache misses */
        CACHE_MISSES,
    }

    namespace CounterType {
        function toString(c: CounterType): string;
    }

    const x = 0n;

    /**
     * A group of counters which can be read and reset simultaneously
     */
    interface CounterGroup {
        /** The counters of this group. */
        readonly counters: ArrayLike<CounterType>;

        /** Reset all counters to 0 */
        reset();

        /**
         * Read the current values of all counters to an array ordered by
         * the counters property
         */
        readAll(to?: BigInt64Array): BigInt64Array;

        /**
         * @returns The current value of a single counter, or the first
         * counter if @param counter is not supplied
         */
        read(counter?: CounterType): bigint;

        /** Stop all counters */
        stop();
    }

    /**
     * Begin a new counter group with the given counter types
     */
    function begin(...counters: CounterType[]): CounterGroup;
}

export = cpuPerfCounters;
