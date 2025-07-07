const svm = require("../index.js")

function sleep(time) {
    let start = Date.now()
    while (Date.now() - start <= time) {
    }
}

const isolate = new svm.Isolate();

// isolate有一个默认的context
const default_ctx = isolate.context

// context可通过eval进行代码的同步运行
console.time("evalSync")
const result = default_ctx.evalSync("this.a = {name: 'Jack', age: 18}")
console.timeEnd("evalSync")
console.log(`evalSync() = `, result);
console.log("---------");

// context可通过evalSync进行代码的异步运行
async function test(i) {
    try {
        let start = Date.now();
        console.log("宏任务", start);

        const asyncResult = await default_ctx.evalAsync(`this.a = {name: 'Jack', age: 19}`);
        console.log(`微任务 ${Date.now()} ${Date.now() - start} evalAsync() = `, asyncResult);
    } catch (err) {
        console.error('Error:', err);
    } finally {
        // 清理资源
    }
}

test(1)
test(2)

setInterval(function () {
    test()
}, 100);

// 释放isolate隔离实例的相关资源
// isolate.release();
// svm.gc();
