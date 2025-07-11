'use strict';
const svm = require('../self-vm');

const isolate = new svm.Isolate();

/* isolate有一个默认的context */
const ctx = isolate.context

/* context可通过eval进行代码的同步运行 */
try {
    console.time("eval")
    const result = ctx.eval(`this.a = {name: 'Jack', age: 18}`);
    console.timeEnd("eval")
    console.log(`eval result = `, result)
} catch (e) {
    console.error(e)
}

/* context可通过evalSync进行代码的异步运行 */
try {
    console.time("eval_Async")
    ctx.evalAsync(`this.a = {name: 'Jack', age: 18}`).then(result => {
        console.timeEnd("eval_Async")
        console.log(`微任务成功 evalAsync() = `, result)
    }, error => {
        console.log(`微任务失败  evalAsync() = `, error)
    })
} catch (err) {
    console.error('Error:', err)
}

const data = isolate.getHeapStatistics();
console.log(data)

/*  释放isolate隔离实例的相关资源 */
// isolate.release();

/* 通知nodejs主环境进行垃圾回收 */
svm.gc();
