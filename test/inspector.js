const svm = require('../self-vm');
const WebSocket = require('ws');

const isolate = new svm.Isolate();

const ctx = isolate.context

// 创建调试会话
const channel = isolate.createInspectorSession()
// 将需要调试的context加入会话
channel.addContext(ctx);

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
        console.log('onMessage<', message.toString())
        try {
            channel.dispatchMessage(String(message));
        } catch (err) {
            ws.close();
        }
    });

    channel.onResponse = (msg) => {
        console.log('onResponse>', msg.toString())
        try {
            ws.send(msg);
        } catch (err) {
            dispose();
        }
    };
    channel.onNotification = (msg) => {
        console.log('onNotification>', msg.toString())
        try {
            ws.send(msg);
        } catch (err) {
            dispose();
        }
    };
});
console.log('devtools://devtools/bundled/inspector.html?experiments=true&v8only=true&ws=127.0.0.1:10000');

setInterval(async () => {
    try {
        await channel.dispatchMessage('{"id":1,"method":"Debugger.enable"}');
        const result = await ctx.evalAsync(`debugger;this.a = {name: 'Jack', age: 18}`);
        console.log(`调试 eval result = `, result)
    } catch (e) {
        console.error(e)
    }
}, 5000);