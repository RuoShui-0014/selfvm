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
const result = default_ctx.eval("this.a = {name: 'Jack', age: 18}")
console.log(result);

let start = Date.now()
for (let i = 0; i < 10000; i++) {
    const resu = default_ctx.eval("this.a = {name: 'Jack', age: 18}")
    console.log(resu, `第${i}次`)
}
console.log(Date.now() - start, "ms")

sleep(10000)

// 释放isolate隔离实例的相关资源
isolate.release();
svm.gc();

sleep(10000)
