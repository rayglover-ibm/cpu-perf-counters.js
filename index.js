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


/** @type import('.').OnlineStats */
class OnlineStats {
    #n = 0;
    #M1 = 0;
    #M2 = 0;
    #M3 = 0;
    #M4 = 0;

    N() { return this.#n; }

    mean() { return this.#n === 0 ? NaN : this.#M1; }

    std() { return Math.sqrt(this.#M2 / this.#n); }

    cov() { return this.std() / this.mean(); }

    skewness() { return Math.sqrt(this.#n) * this.#M3 / (this.#M2 ** 1.5); }

    kurtosis() { return (this.#n * this.#M4) / (this.#M2 * this.#M2) - 3; }

    /**
     * @param {number} x
     */
    push(x) {
        const n1 = this.#n;
        const n = ++this.#n;

        const delta = x - this.#M1;
        const delta_n = delta / n;
        const delta_n2 = delta_n * delta_n;
        const term1 = delta * delta_n * n1;

        this.#M1 += delta_n;
        this.#M4 += term1 * delta_n2 * (n * n - 3 * n + 3) + 6 * delta_n2 * this.#M2 - 4 * delta_n * this.#M3;
        this.#M3 += term1 * delta_n * (n - 2) - 3 * delta_n * this.#M2;
        this.#M2 += term1;

        return n;
    }

    clone() {
        const s = new OnlineStats();

        s.#n = this.#n;
        s.#M1 = this.#M1;
        s.#M2 = this.#M2;
        s.#M3 = this.#M3;
        s.#M4 = this.#M4;

        return;
    }
}

exports.CounterType = CounterType;

exports.OnlineStats = OnlineStats;

exports.begin = (...counters) => new CounterGroupImpl(...counters);
