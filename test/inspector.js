const {svm, registerSession} = require('../self-vm');

// 使用调试前需调用该函数进行会话注册
registerSession();

const isolate = new svm.Isolate();
const ctx = isolate.context

// 创建调试会话
const session = isolate.session
// 将需要调试的context加入会话
session.addContext(ctx);

setInterval(async () => {
    try {
        await session.dispatchMessage('{"id":1,"method":"Debugger.enable"}');
        const result = await ctx.evalAsync(`debugger;this.a = {name: 'Jack', age: 18}`);
        console.log(`调试 eval result = `, result)
    } catch (e) {
        console.error(e)
    }
}, 5000);