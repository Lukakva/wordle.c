const { execFileSync } = require('child_process');

const times = [];

function time(t, w) {
    const n = 1;
    // Runs it n times to average out
    let total = 0;

    for (let i = 0; i < n; i++) {
        const start = Date.now();

        execFileSync('./wordle', [
            `-t ${t}`,
            `-w ${w}`
        ]);

        const end = Date.now();
        total += end - start;
    }

    console.log(`./wordle -t ${t} -w ${w}: ${total} ms`);

    return total / n;
}

const tValues = [4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14];
const wValues = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];

tValues.forEach(t => {
    wValues.forEach(w => {
        const result = time(t, w);
        times.push({
            t,
            w,
            time: result
        })
    })
})

times.sort((a, b) => a.time - b.time);
console.log(JSON.stringify(times, null, 4));