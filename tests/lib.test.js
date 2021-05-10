// @ts-check
const { execFileSync } = require("child_process");
const { equal, ok, throws } = require('assert').strict;
const { CounterType, begin } = require('..');

/** @param {number} n */
function factorial(n) {
    let result = 0n;
    for (let i = 1n; i <= n; i++) {
        result *= i;
    }
    return result;
}

const tests = {
    'Single counter': () => {
        const types = [
            CounterType.BRANCH_INSTRUCTIONS,
            CounterType.BRANCH_MISSES,
            CounterType.CACHE_MISSES,
            CounterType.CACHE_REFERENCES,
            CounterType.CONTEXT_SWITCHES,
            CounterType.CPU_CLOCK,
            CounterType.CYCLES,
            CounterType.INSTRUCTIONS,
        ];

        types.forEach(type => {
            const group = begin(type);

            for (let i = 0; i < 5; i++) {
                group.reset();
                factorial(10);
                const x = group.read();
                equal(typeof x, 'bigint');
                ok(x >= 0n);
            }

            group.stop();

            throws(() => group.reset());
            throws(() => group.read());
        });
    },
    'Multiple counters - read()': () => {
        const group = begin(CounterType.CYCLES, CounterType.INSTRUCTIONS);

        for (let i = 0; i < 5; i++) {
            group.reset();
            factorial(10);

            const x0 = group.read();
            const x1 = group.read(CounterType.CYCLES);
            const x2 = group.read(CounterType.INSTRUCTIONS);

            ok(x0 > 0n);
            ok(x1 > 0n);
            ok(x2 > 0n);
        }

        equal(group.read(CounterType.CONTEXT_SWITCHES), undefined);
        group.stop();
    },
    'Multiple counters - readAll()': () => {
        const group = begin(CounterType.CYCLES, CounterType.INSTRUCTIONS);

        for (let i = 0; i < 5; i++) {
            group.reset();
            factorial(10);
            const measurements = group.readAll();

            equal(measurements.constructor, BigInt64Array);
            equal(measurements.length, 2);
            ok(measurements[0] > 0n);
        }

        group.stop();
    },
    'Multiple counters - readAll(arr)': () => {
        const group = begin(CounterType.CYCLES, CounterType.INSTRUCTIONS);

        const buffer = new BigInt64Array([-1n, -1n]);
        for (let i = 0; i < 5; i++) {
            group.reset();
            factorial(10);
            const measurements = group.readAll(buffer);

            equal(measurements, buffer);
            ok(measurements[0] > 0n);
        }

        group.stop();
    }
}

/** @returns true if one or more cpu's support performance monitoring */
function supportedCpu() {
    try {
        const out = execFileSync('grep', ['-c', 'arch_permon', '/proc/cpuinfo'], { encoding: 'utf-8' });
        return Number(out) > 0;
    } catch (err) {
        // 0 matches causes grep to error
        equal(Number(err.stdout), 0);
        return false;
    }
}

if (!supportedCpu()) {
    console.warn('##\n## UNSUPPORTED CPU TESTS NOT RUN\n##\n');
} else {
    module.exports = tests;
}
