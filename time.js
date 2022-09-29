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

const values = [1, 2, 4, 8, 10, 12, 16, 24, 32, 48, 64, 96, 128, 256, 512, 1024];

values.forEach(t => {
    values.forEach(w => {
        const result = time(t, w);
        times.push({
            t,
            w,
            time: result
        })
    })
})

console.log(JSON.stringify(times, null, 4))