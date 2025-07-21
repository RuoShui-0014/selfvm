'use strict';
const svm = require('../self-vm');
const WebSocket = require("ws");

const isolate = new svm.Isolate({memoryLimit: 512});

/* isolate有一个默认的context */
const ctx = isolate.context

/* context可通过eval进行代码的同步运行 */
try {
    console.time("eval sync")
    const result = ctx.eval(`this.a = {name: 'Jack', age: 18, title: 'eval sync test!'}`);
    console.timeEnd("eval sync")
    console.log(`同步任务成功 `, result)
} catch (e) {
    console.error(e)
}

/* context可通过evalSync进行代码的异步运行 */
try {
    console.time("eval async")
    ctx.evalAsync(`this.a = {name: 'Jack', age: 18, title: 'eval async test!'}`).then(result => {
        console.timeEnd("eval async")
        console.log(`微任务成功 `, result)
    }, error => {
        console.log(`微任务失败 `, error)
    })
} catch (err) {
    console.error('Error:', err)
}

const data = isolate.getHeapStatistics();
console.log(data)

/*  释放isolate隔离实例的相关资源 */
// isolate.release();

/* 通知nodejs主环境进行垃圾回收 */
// svm.gc();

const script = isolate.createScript("this.a = {name: 'Jack', age: 18, title: 'script test!'};", "filename.js");
console.log(script.run(ctx))

// const channel = isolate.createInspectorSession()
// channel.addContext(ctx);
// Create an inspector channel on port 10000
// let wss = new WebSocket.Server({port: 10000});
// wss.on('connection', function (ws) {
//     function dispose() {
//         try {
//             channel.dispose();
//         } catch (err) {
//         }
//     }
//     ws.on('error', dispose);
//     ws.on('close', dispose);
//
//     ws.on('message', function (message) {
//         console.log('onMessage<', message.toString())
//         try {
//             channel.dispatchMessage(String(message));
//         } catch (err) {
//             ws.close();
//         }
//     });
//
//     channel.onResponse = (msg) => {
//         console.log('onResponse>', msg.toString())
//         try {
//             ws.send(msg);
//         } catch (err) {
//             dispose();
//         }
//     };
//     channel.onNotification = (msg) => {
//         console.log('onNotification>', msg.toString())
//         try {
//             ws.send(msg);
//         } catch (err) {
//             dispose();
//         }
//     };
// });
// console.log('devtools://devtools/bundled/inspector.html?experiments=true&v8only=true&ws=127.0.0.1:10000');
//
// setInterval(async () => {
//     try {
//         await channel.dispatchMessage('{"id":1,"method":"Debugger.enable"}');
//         const result = await ctx.evalAsync(`debugger;this.a = {name: 'Jack', age: 18}`, "test");
//         console.log(`调试 eval result = `, result)
//     } catch (e) {
//         console.error(e)
//     }
// }, 5000);