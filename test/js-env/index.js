const {svm, registerSession, unregisterSession} = require('../../self-vm');
const fs = require('fs');
const Tool = require(`${__dirname}/Tools/ToolCode.js`);

const WindowCode = Tool.GetWindowCode();        // 非Worker的环境代码
const WorkerCode = Tool.GetWorkerCode();        // Worker的环境代码

const allCode = WindowCode + fs.readFileSync(`${__dirname}/Sites/rs_药监局.js`);

const isolate = new svm.Isolate({memoryLimit: 1024});
const ctx = isolate.context;
// registerSession();
// isolate.session.connect(10001);
// isolate.session.addContext(ctx, "session_01");
debugger
setTimeout(async () => {
    let result = await ctx.evalAsync("debugger;999", "test1.js");
    console.log(result)
    // console.time("test")
    result = await ctx.evalAsync(allCode, "test2.js");
    console.log(result);
    // console.timeEnd("test")
    isolate.release();
}, 3000);

// console.time("test")
// result = ctx.eval(allCode, "test2.js");
// console.log(result);
// console.timeEnd("test")
