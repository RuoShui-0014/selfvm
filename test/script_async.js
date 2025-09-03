const {svm} = require('../self-vm');

const isolate = new svm.Isolate();
const ctx = isolate.context

// context可通过eval进行代码的同步运行
async function test(index, num) {
    try {
        if (index === num - 1) {
            console.time("ScriptAsync" + index)
        }
        isolate.createScriptAsync("this.a = {name: 'Jack', age: 18}"
            , "filename.js").then(script => {
            script.runAsync(ctx).then(value => {
                if (index === num - 1) {
                    console.log("script.runAsync", value);
                    console.timeEnd("ScriptAsync" + index);
                    isolate.release();
                }
            })
        }, error => {
            console.log(`微任务失败`, error);
        });
    } catch (err) {
        console.error('Error:', err);
    }
}

const num = 1000;
for (let i = 0; i < num; i++) {
    test(i, num);
}
