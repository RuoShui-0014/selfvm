const svm = require('../self-vm');

const isolate = new svm.Isolate();

const ctx = isolate.context

// context可通过eval进行代码的同步运行
let times = 10;
console.time("eval_sync")
for (let i = 0; i < times; i++) {
    try {
        console.time("eval_" + i)
        const result = ctx.eval(`this.a = {name: 'Jack', age: 18}`);
        console.timeEnd("eval_" + i)
        console.log(`eval result = `, result)
    } catch (e) {
        console.error(e)
    }
}
console.timeEnd("eval_sync")