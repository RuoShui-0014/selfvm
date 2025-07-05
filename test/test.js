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
async function test(i = 0) {
    const result = default_ctx.evalAsync("this.a = {name: 'Jack', age: 18}")
    console.log(`${i} -> result = ${result}`);
    result.then((e) => {
        console.log(`success e = `, e);
    }, (e) => {
        console.log(`failed e = `, e);
    })
}

console.log(process.memoryUsage())
let start = Date.now()
for (let i = 0; i < 299; i++) {
    test(i);
}
console.log(Date.now() - start, "ms")
console.log(process.memoryUsage())

setInterval(function () {
    console.log(process.memoryUsage())
}, 1000)

// sleep(10000)
//
// // 释放isolate隔离实例的相关资源
// isolate.release();
// svm.gc();
//
// sleep(10000)
