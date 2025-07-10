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
        // console.time("" + i)
        result = default_ctx.eval("this.a = {name: 'Jack', age: 18};")
        // console.timeEnd("" + i)
        console.log(`eval result = `, result);
    } catch (e) {
        console.error(e)
    }
}
console.timeEnd("eval")
console.log("---------");
isolate.gc()

// 创建新context
const ctx = isolate.createContext()
result = ctx.eval(`this.a = "" + btoa("test");`)
console.log(`ctx --> `, result);

// 异步创建新context
isolate.createContextAsync().then(ctx => {
    ctx.eval(`this.a = "" + btoa("test");`)
    console.log(`异步创建 ctx --> `, result);
});

// context可通过evalSync进行代码的异步运行
async function test() {
    try {
        let start = Date.now();
        console.log("宏任务");

        // 套娃，在子环境中创建新的环境，并同步运行代码，将结果返回
        default_ctx.evalAsync(`
        isolate = new Isolate();
        result = isolate.context.eval("this.a = {name: 'Jack', age: 18}")
        this.a = {name: 'Jack', age: 18, value: result}
        `).then(result => {
            console.log(`微任务成功 ${Date.now() - start} evalAsync() = `, result);
        }, error => {
            console.log(`微任务失败 ${Date.now() - start} evalAsync() = `, error);
        });

    } catch (err) {
        console.error('Error:', err);
    }
}

default_ctx.eval(`
isolate = new Isolate();
result = isolate.context.eval("this.a = {name: 'Jack', age: 18}")
this.a = {name: 'Jack', age: 18, value: result}
`)

// test()
for (let i = 0; i < 10; i++) {
    test()
}

setTimeout(function () {
    isolate.gc()
}, 3000);

// 释放isolate隔离实例的相关资源
// isolate.release();
// svm.gc();
