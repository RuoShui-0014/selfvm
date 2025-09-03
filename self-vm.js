const self_vm = require('./bin/self-vm.node');
const WebSocket = require("ws");

const sessionMap = new Map;

module.exports = {
    svm: self_vm,
    registerSession() {
        global.sessionOnResponse = function (port, message) {
            if (sessionMap.has(port)) {
                sessionMap.get(port).onResponse(message);
            }
        }
        global.sessionOnNotification = function (port, message) {
            if (sessionMap.has(port)) {
                sessionMap.get(port).onNotification(message);
            }
        }
        global.sessionConnect = (port) => {
            const wss = new WebSocket.Server({port: port});
            if (sessionMap.has(port)) {
                throw new TypeError(`${port}端口已经被占用!!!`);
            }
            sessionMap.set(port, {wss: wss});
            wss.on('connection', function (ws) {
                function dispose() {
                    try {
                        self_vm.sessionDispose(port);
                    } catch (err) {
                    }
                }

                ws.on('error', dispose);
                ws.on('close', dispose);

                ws.on('message', function (message) {
                    // console.log('onMessage<', message.toString())
                    try {
                        self_vm.sessionDispatchMessage(port, message.toString());
                    } catch (err) {
                        ws.close();
                    }
                });

                sessionMap.get(port).onResponse = (msg) => {
                    // console.log('onResponse>', msg.toString())
                    try {
                        ws.send(msg);
                    } catch (err) {
                        dispose();
                    }
                }
                sessionMap.get(port).onNotification = (msg) => {
                    // console.log('onNotification>', msg.toString())
                    try {
                        ws.send(msg);
                    } catch (err) {
                        dispose();
                    }
                }

            });
            console.log('devtools://devtools/bundled/inspector.html?experiments=true&v8only=true&ws=127.0.0.1:' + port);
        }
        global.sessionDisconnect = (port) => {
            sessionMap.delete(port);
        }
    },
    unregisterSession() {
        for (const [_, value] of sessionMap) {
            value.wss.close()
        }
    }
}