const svm = require('../self-vm');
const WebSocket = require('ws');

const isolate = new svm.Isolate();

const ctx = isolate.context
const channel = isolate.createInspectorSession()

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
            // This happens if inspector session was closed unexpectedly
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

    channel.onResponse = (callId, message) => send(message);
    channel.onNotification = send;
});
console.log('Inspector: devtools://devtools/bundled/inspector.html?experiments=true&v8only=true&ws=127.0.0.1:10000');

// context可通过eval进行代码的同步运行
try {
    const result = ctx.eval(`debugger;this.a = {name: 'Jack', age: 18}`);
    console.log(`eval result = `, result)
} catch (e) {
    console.error(e)
}