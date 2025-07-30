const svm = require('../self-vm');

const isolate = new svm.Isolate();

const ctx = isolate.context

// context可通过eval进行代码的同步运行
let times = 1000;
console.time("script async")
for (let i = 0; i < times; i++) {
    try {
        console.time("script async" + i)
        isolate.createScriptAsync("this.a = {name: 'Jack', age: 18};this.a"
            , "filename.js").then(script => {
            console.timeEnd("script async" + i)
            const result = script.run(ctx)
            console.log(`微任务成功 `, result);
        }, error => {
            console.log(`微任务失败 `, error);
        });
        // console.log(`eval result = `, result)
    } catch (e) {
        console.error(e)
    }
}
console.timeEnd("script async")