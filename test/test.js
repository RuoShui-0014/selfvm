const svm = require("../index.js")

const isolate = new svm.Isolate();

// isolate有一个默认的context
const default_ctx = isolate.context

// context可通过eval进行代码的同步运行
console.time("eval")
let result = default_ctx.eval("this.a = {name: 'Jack', age: 18}")
result = default_ctx.eval(`
this.a = "" + btoa("test");
`)
console.timeEnd("eval")
console.log(`eval result = `, result);
console.log("---------");

// 创建新context
const ctx = isolate.createContext()
ctx.eval(`
this.a = "" + btoa("test");
`)
console.log(`ctx --> `, result);

isolate.createContextAsync().then(ctx => {
    ctx.eval(`this.a = "" + btoa("test");`)
    console.log(`ctx --> `, result);
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
    } finally {
        // 清理资源
    }
}

for (let i = 0; i < 10; i++) {
    test()
}

setTimeout(function () {
    isolate.gc()
}, 3000);

// 释放isolate隔离实例的相关资源
// isolate.release();
// svm.gc();
