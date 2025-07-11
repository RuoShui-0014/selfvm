const svm = require("../index.js")

const isolate = new svm.Isolate();

// isolate有一个默认的context
const default_ctx = isolate.context

// context可通过eval进行代码的同步运行
let times = 1000;
let result;
console.time("eval")
for (let i = 0; i < times; i++) {
    try {
        console.time("eval_" + i)
        result = default_ctx.eval("this.a = {name: 'Jack', age: 18};")
        console.timeEnd("eval_" + i)
        console.log(`eval result = `, result);
    } catch (e) {
        console.error(e)
    }
}
console.timeEnd("eval")
// console.log("---------");
// isolate.gc()
//
// // 创建新context
// const ctx = isolate.createContext()
// result = ctx.eval(`this.a = "" + btoa("test");`)
// console.log(`ctx --> `, result);
//
// // 异步创建新context
// console.time("createContextAsync")
// isolate.createContextAsync().then(ctx => {
//     console.timeEnd("createContextAsync")
//     ctx.eval(`this.a = "" + btoa("test");`)
//     console.log(`异步创建 ctx --> `, result, ctx);
// });
//
// context可通过evalSync进行代码的异步运行
async function test(i) {
    try {
        console.time("evalAsync_" + i)
        // 套娃，在子环境中创建新的环境，并同步运行代码，将结果返回
        default_ctx.evalAsync(`
        isolate = new Isolate();
        result = isolate.context.eval("this.a = {name: 'Jack', age: 18}")
        this.a = {name: 'Jack', age: 18, value: result}
        `).then(result => {
            console.timeEnd("evalAsync_" + i)
            console.log(`微任务成功 evalAsync() = `, result);
        }, error => {
            console.log(`微任务失败  evalAsync() = `, error);
        });

    } catch (err) {
        console.error('Error:', err);
    }
}
//
// default_ctx.eval(`
// isolate = new Isolate();
// result = isolate.context.eval("this.a = {name: 'Jack', age: 18}")
// this.a = {name: 'Jack', age: 18, value: result}
// `)

// test()
for (let i = 0; i < 10; i++) {
    test(i)
}

setTimeout(function () {
    isolate.gc()
}, 3000);

// 释放isolate隔离实例的相关资源
// isolate.release();
// svm.gc();
