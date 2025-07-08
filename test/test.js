const svm = require("../index.js")

function sleep(time) {
    let start = Date.now()
    while (Date.now() - start <= time) {
    }
}

const isolate = new svm.Isolate();

// isolate有一个默认的context
const default_ctx = isolate.context
console.log(isolate.getHeapStatistics())
// context可通过eval进行代码的同步运行
console.time("evalSync")
// const result = default_ctx.evalSync("this.a = {name: 'Jack', age: 18}")
const result = default_ctx.evalSync(`
this.a = "" + btoa("test");
`)
console.timeEnd("evalSync")
console.log(`evalSync result = `, result);
console.log("---------");

// context可通过evalSync进行代码的异步运行
async function test() {
    try {
        let start = Date.now();
        console.log("宏任务");

        // 套娃，在子环境中创建新的环境，并同步运行代码，将结果返回
        const asyncResult = await default_ctx.evalAsync(`
isolate = new Isolate();
result = isolate.context.evalSync("this.a = {name: 'Jack', age: 18}")
this.a = {name: 'Jack', age: 18, value: result}
`);
        console.log(`微任务 ${Date.now() - start} evalAsync() = `, asyncResult);
    } catch (err) {
        console.error('Error:', err);
    } finally {
        // 清理资源
    }
}

for (let i = 0; i < 10; i++) {
    test()
}


setTimeout(function () {
    isolate.gc()
    console.log(isolate.getHeapStatistics())
}, 3000);

// 释放isolate隔离实例的相关资源
// isolate.release();
// svm.gc();
