# cpu-perf-counters.js

_Performance counters for Node.js_

This package exposes performance counters of modern processors to Node.js applications. It's designed as a drop-in replacement for [`process.hrtime.bigint()`](https://nodejs.org/api/process.html#process_process_hrtime_bigint), enabling different types of performance analysis.

Currently this package only supports Linux via the `perf_event_open` performance monitoring API (used by Linux [perf](https://en.wikipedia.org/wiki/Perf_(Linux))).

## Install

```bash
npm install cpu-perf-counters
```

Note this package is a Node.js N-API native addon and requires at least:
- A recent C++ compiler
- Node.js - `v12.17.0` (for `BigInt` support)
- CMake - `v3.11`

## Usage

Consider the following function we want to analyse:

```js
function factorial(n) {
    let result = 0n;
    for (let i = 1n; i <= n; i++) {
        result *= i;
    }
    return result;
}
```

There are several ways to consider the run-time performance of a function like this. For instance if we want to measure the number of cpu cycles it takes to execute, configure a counter with `begin(CounterType.CYCLES)` and read counter values with `read()`:

```js
import { begin, CounterType } from 'cpu-perf-counters';

/* Create a sampler */
const sampler = begin(CounterType.CYCLES);
/* Run the function */
factorial(100n);
/* Read the current counter count */
console.info(sampler.read());
/* Clean up */
sampler.stop();

```

The result of a `read()` is a `bigint`, e.g.:

```js
183297n
```

It's also possible to run and read multiple counters simultaneously. Here we count the number of instructions executed and total cycles required to execute them:

```js
/* Create a sampler with two counters */
const sampler = begin(
    CounterType.CYCLES, CounterType.Instructions);

/* Run the function */
factorial(100n);
/* Read the current counter values */
console.info(sampler.readAll());
/* Clean up */
sampler.stop();
```

The result of `readAll()` is a `BigInt64Array` with an entry for each counter, ordered corresponding to their index in `begin()`:

```js
BigInt64Array(2) [
    12482n /* Cycles */,
    13815n /* Instructions */
]
```

You can also read each counter independantly, e.g. with `sampler.read(CounterType.Instructions)`. Note, however, reads like this aren't done simultaneously.

## Documentation

- See [index.d.ts](./index.d.ts) for API documentation
- Read the [FAQ](./docs/faq.md) for more details

---

## License

Copyright 2021 Ray Glover

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
