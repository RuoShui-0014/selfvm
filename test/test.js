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
const result = default_ctx.evalSync("this.a = {name: 'Jack', age: 18}")
console.log(`result = `, result);

// context可通过evalSync进行代码的异步运行
async function main() {
    try {
        const syncResult = default_ctx.evalSync("this.a = {name: 'Jack', age: 18}");
        console.log(`Sync result = `, syncResult);

        const asyncResult = await default_ctx.evalAsync(`this.a = {name: 'Jack', age: 19}`);
        console.log(`Async result = `, asyncResult);
    } catch (err) {
        console.error('Error:', err);
    } finally {
        // 清理资源
    }
}

main().then(() => {
    console.log('All operations completed');
    process.exit(0);
});

console.log(Date.now());

setTimeout(function () {

}, 10000)

// sleep(10000)
//
// // 释放isolate隔离实例的相关资源
// isolate.release();
// svm.gc();
//
// sleep(10000)
