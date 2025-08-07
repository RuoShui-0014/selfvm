const { svm } = require('../self-vm');

const isolate = new svm.Isolate();

const ctx = isolate.context

// context可通过evalSync进行代码的异步运行
async function test(i) {
    try {
        console.time("eval_Async" + i)
        ctx.evalAsync(`
        this.a = {name: 'Jack', age: 18};
        JSON.stringify(this.a);`, "self-vm.js").then(result => {
            console.timeEnd("eval_Async" + i)
            console.log(`微任务成功 evalAsync() = `, result);
        }, error => {
            console.log(`微任务失败  evalAsync() = `, error);
        });
    } catch (err) {
        console.error('Error:', err);
    }
}
for (let i = 0; i < 100; i++) {
    test(i)
}
