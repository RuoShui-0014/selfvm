const { svm, registerSession, unregisterSession } = require('../self-vm');

// 使用调试前需调用该函数进行会话注册
registerSession();

const isolate = new svm.Isolate();
const ctx = isolate.context

// 创建调试会话
const session = isolate.session
// 连接会话
session.connect(10001);
// 将需要调试的context加入会话
session.addContext(ctx, "session_01");

let index = 0, id = 0;
id = setInterval(async () => {
    try {
        const result = await ctx.evalAsync(`debugger;this.a = {name: 'Jack', age: 18}`);
        console.log(`调试 eval result = `, result)
    } catch (e) {
        console.error(e)
    }

    if (index++ >5) {
        unregisterSession();
        clearInterval(id);
    }
}, 5000);