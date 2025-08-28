module.exports = function measurePerformance(fn, runs) {
    const times = [];

    for (let i = 0; i < runs; i++) {
        const start = process.hrtime.bigint();
        fn();
        const end = process.hrtime.bigint();

        // 转换为毫秒（1毫秒 = 1,000,000纳秒）
        const duration = Number(end - start) / 1000000;
        times.push(duration);
    }

    const total = times.reduce((sum, time) => sum + time, 0);
    const average = total / runs;
    const min = Math.min(...times);
    const max = Math.max(...times);

    const result = {
        runs,
        total: parseFloat(total.toFixed(4)),
        average: parseFloat(average.toFixed(6)),
        min: parseFloat(min.toFixed(6)),
        max: parseFloat(max.toFixed(6)),
        times
    };
    console.log('性能测试结果:');
    console.log(`运行次数: ${result.runs}`);
    console.log(`总耗时: ${result.total} 毫秒`);
    console.log(`平均耗时: ${result.average} 毫秒`);
    console.log(`最小耗时: ${result.min} 毫秒`);
    console.log(`最大耗时: ${result.max} 毫秒`);
    return result;
}