const {svm} = require('../self-vm');

const isolate = new svm.Isolate();
const ctx = isolate.context

// context可通过eval进行代码的同步运行
async function test(i) {
    try {
        console.time("ScriptAsync" + i)
        isolate.createScriptAsync("this.a = {name: 'Jack', age: 18}"
            , "filename.js").then(script => {
            console.timeEnd("ScriptAsync" + i);
            script.run(ctx);
        }, error => {
            console.log(`微任务失败`, error);
        });
    } catch (err) {
        console.error('Error:', err);
    }
}

for (let i = 0; i < 100; i++) {
    test(i);
}
