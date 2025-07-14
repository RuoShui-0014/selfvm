const svm = require('../self-vm');
const WebSocket = require('ws');

const isolate = new svm.Isolate();

const ctx = isolate.context
const ctx2 = isolate.createContext()
const channel = isolate.createInspectorSession()
channel.addContext(ctx);
channel.addContext(ctx2);

// Create an inspector channel on port 10000
let wss = new WebSocket.Server({port: 10000});
wss.on('connection', function (ws) {
    function dispose() {
        try {
            // channel.dispose();
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

    channel.onResponse = (message) => send(message);
    channel.onNotification = send;
});
console.log('Inspector: devtools://devtools/bundled/inspector.html?experiments=true&v8only=true&ws=127.0.0.1:10000');

async function test() {
    try {
        await channel.dispatchMessage('{"id":1,"method":"Debugger.enable"}');
        const result = await ctx.evalAsync(`debugger;this.a = {name: 'Jack', age: 18}`);
        console.log(`调试 eval result = `, result)
    } catch (e) {
        console.error(e)
    }
}

setInterval(test, 5000);