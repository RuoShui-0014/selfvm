'use strict';

const { svm, registerSession, unregisterSession } = require('../self-vm');

/* 创建Isolate */
const isolate = new svm.Isolate({memoryLimit: 128});
/* isolate有一个默认的上下文环境 */
const ctx = isolate.context

/* context可通过eval进行代码的同步运行 */
// try {
//     console.time("eval sync")
//     const result = ctx.eval(`this.a = {name: 'Jack', age: 18, title: 'eval sync test!'}`);
//     console.timeEnd("eval sync")
//     console.log(`同步任务成功 `, result)
// } catch (e) {
//     console.error(e)
// }
//
// /* context可通过evalSync进行代码的异步运行 */
// try {
//     console.time("eval async")
//     ctx.evalAsync(`this.a = {name: 'Jack', age: 18, title: 'eval async test!'}`).then(result => {
//         console.timeEnd("eval async")
//         console.log(`微任务成功 `, result)
//     }, error => {
//         console.log(`微任务失败 `, error)
//     })
// } catch (err) {
//     console.error('Error:', err)
// }

// const data = isolate.getHeapStatistics();
// console.log(data)

/*  释放isolate隔离实例的相关资源 */
// isolate.release();

/* 通知nodejs主环境进行垃圾回收 */
// svm.gc();

// this.ctx = isolate.createContext()
// delete this.ctx
// svm.gc()
let port = 10001;
registerSession();
const session = isolate.session
session.connect(port++);
session.addContext(ctx, "session_01");

debugger
// 子环境中创建子环境
// const script = isolate.createScript(`
// const isolate = new Isolate();
// const ctx = isolate.context;
// const session = isolate.session
// session.connect(10002);
// session.addContext(ctx, "session_02");
// this.result = ctx.eval("this.a = {title: 'test 套娃'}");
// `, "filename.js");
// console.log(script.run(ctx))

isolate.createScriptAsync("this.a = {title: 'createScriptAsync test.'}"
    , "filename.js").then(script1 => {
    const result = script1.run(ctx);
    console.log(`微任务成功 createScriptAsync() = `, result);
}, error => {
    console.log(`微任务失败 createScriptAsync() = `, error);
});

let index = 0, id = 0;
id = setInterval(async () => {
    try {
        const result = await ctx.evalAsync(`this.isolate = new Isolate();
this.ctx = isolate.context;
this.session = isolate.session
this.session.connect(${port++});
debugger;
this.session.addContext(ctx, "session_02");
this.result = this.ctx.eval("debugger;this.a = {title: 'test 套娃'}");
`, "test" + index++);
        console.log(`调试 eval result = `, result)
        svm.gc()
    } catch (e) {
        console.error(e)
    }

    if (index >= 2) {
        unregisterSession();
        clearInterval(id);
    }
}, 10000);