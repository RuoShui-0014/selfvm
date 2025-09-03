const {svm} = require('../self-vm');

const isolate = new svm.Isolate();
const ctx = isolate.context

// context可通过evalSync进行代码的异步运行
async function test(index, num) {
    try {
        if (index === num - 1) {
            console.time("eval_Async" + index)
        }
        ctx.evalAsync(`
        this.a = {name: 'Jack', age: ${index}};
        JSON.stringify(this.a);`, "self-vm.js").then(result => {
            if (index === num - 1) {
                console.timeEnd("eval_Async" + index)
                console.log(result)
                isolate.release();
            }
        }, error => {
            console.log(`微任务失败  evalAsync() = `, error);
        });
    } catch (err) {
        console.error('Error:', err);
    }
}

const num = 1000
for (let i = 0; i < num; i++) {
    test(i, num)
}
