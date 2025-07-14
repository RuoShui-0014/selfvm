const svm = require('../self-vm');
const WebSocket = require('ws');

const isolate = new svm.Isolate();

const ctx = isolate.context

// 创建调试会话
const channel = isolate.createInspectorSession()
// 将需要调试的context加入会话
channel.addContext(ctx);

// Create an inspector channel on port 10000
let wss = new WebSocket.Server({port: 10000});
wss.on('connection', function (ws) {
    function dispose() {
        try {
            channel.dispose();
        } catch (err) {
        }
    }

    ws.on('error', dispose);
    ws.on('close', dispose);

    ws.on('message', function (message) {
        console.log('<', message.toString())
        try {
            channel.dispatchMessage(String(message));
        } catch (err) {
            ws.close();
        }
    });

    function send(message) {
        console.log('>', message.toString())
        try {
            ws.send(message);
        } catch (err) {
            dispose();
        }
    }

    channel.onResponse = message => send(message);
    channel.onNotification = send;
});
console.log('Inspector: devtools://devtools/bundled/inspector.html?experiments=true&v8only=true&ws=127.0.0.1:10000');

// 调试运行时必须使用异步函数，否则会导致主线程nodejs卡住
async function test() {
    try {
        await channel.dispatchMessage('{"id":1,"method":"Debugger.enable"}');
        const result = await ctx.evalAsync(`debugger;this.a = {name: 'Jack', age: 18}`);
        console.log(`调试中... result = `, result)
    } catch (e) {
        console.error(e)
    }
}
test()
setInterval(test, 5000);