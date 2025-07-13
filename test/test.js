'use strict';
const svm = require('../self-vm');
const WebSocket = require("ws");

const isolate = new svm.Isolate();

/* isolate有一个默认的context */
const ctx = isolate.context
const channel = isolate.createInspectorSession()
channel.addContext(ctx);

/* context可通过eval进行代码的同步运行 */
// try {
//     console.time("eval")
//     const result = ctx.eval(`this.a = {name: 'Jack', age: 18}`);
//     console.timeEnd("eval")
//     console.log(`eval result = `, result)
// } catch (e) {
//     console.error(e)
// }
//
// /* context可通过evalSync进行代码的异步运行 */
// try {
//     console.time("eval_Async")
//     ctx.evalAsync(`this.a = {name: 'Jack', age: 18}`).then(result => {
//         console.timeEnd("eval_Async")
//         console.log(`微任务成功 evalAsync() = `, result)
//     }, error => {
//         console.log(`微任务失败  evalAsync() = `, error)
//     })
// } catch (err) {
//     console.error('Error:', err)
// }
//
// const data = isolate.getHeapStatistics();
// console.log(data)

/*  释放isolate隔离实例的相关资源 */
// isolate.release();

/* 通知nodejs主环境进行垃圾回收 */
// svm.gc();


// Create an inspector channel on port 10000
let wss = new WebSocket.Server({port: 10000});
async function test() {
    try {
        console.log('test');
        await channel.dispatchMessage('{"id":1,"method":"Debugger.enable"}');
        const result = await ctx.evalAsync(`debugger;this.a = {name: 'Jack', age: 18}`);
        console.log(`调试 eval result = `, result)
    } catch (e) {
        console.error(e)
    }
}
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

    channel.onResponse = message => send(message);
    channel.onNotification = send;

    test();
});
console.log('Inspector: devtools://devtools/bundled/inspector.html?experiments=true&v8only=true&ws=127.0.0.1:10000');


// setInterval(test, 5000);