// @ts-check
const binding = require('bindings')('cpu-perf-counters');

/** @typedef {import('.').CounterGroup} CounterGroup */
/** @typedef {import('.').CounterType} CounterType */

const CounterType = {
    CYCLES: 0,
    INSTRUCTIONS: 1,
    TASK_CLOCK: 2,
    CPU_CLOCK: 3,
    CONTEXT_SWITCHES: 4,
    BRANCH_INSTRUCTIONS: 5,
    BRANCH_MISSES: 6,
    CACHE_REFERENCES: 7,
    CACHE_MISSES: 8,

    toString(counter) {
        switch (counter) {
            case this.CYCLES: return 'Cycles';
            case this.INSTRUCTIONS: return 'Instructions';
            case this.TASK_CLOCK: return 'Task Clock (ns)';
            case this.CPU_CLOCK: return 'CPU Clock (ns)';
            case this.CONTEXT_SWITCHES: return 'Context switches';
            case this.BRANCH_INSTRUCTIONS: return 'Branch instructions';
            case this.BRANCH_MISSES: return 'Branch misses';
            case this.CACHE_REFERENCES: return 'Cache references';
            case this.CACHE_MISSES: return 'Cache misses';
            default: return 'Unknown';
        }
    }
}

/** @implements {CounterGroup} */
class CounterGroupImpl {
    #id = -1;
    counters;

    /** @param {CounterType[]} counters */
    constructor(...counters) {
        this.#id = binding.create(counters);
        this.counters = counters;
    }

    reset() {
        binding.reset(this.#id);
    }

    readAll(to = new BigInt64Array(this.counters.length)) {
        binding.readAll(this.#id, to);
        return to;
    }

    /** @param {CounterType} [counterType] */
    read(counterType = this.counters[0]) {
        return binding.read(this.#id, counterType);
    }

    stop() {
        binding.stop(this.#id);
        this.#id = -1;
    }
}

exports.CounterType = CounterType;

exports.begin = (...counters) => new CounterGroupImpl(...counters);
