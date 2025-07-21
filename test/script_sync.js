const svm = require('../self-vm');

const isolate = new svm.Isolate();

const ctx = isolate.context

// context可通过eval进行代码的同步运行
let times = 1000;
console.time("script sync")
for (let i = 0; i < times; i++) {
    try {
        console.time("eval " + i)
        const script = isolate.createScript("this.a = {name: 'Jack', age: 18};", "filename.js");
        const result = script.run(ctx)
        console.timeEnd("eval " + i)
        // console.log(`eval result = `, result)
    } catch (e) {
        console.error(e)
    }
}
console.timeEnd("script sync")