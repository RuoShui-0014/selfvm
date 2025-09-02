!function () {
    globalThis.self = globalThis;
    rsvm.prototype.DedicatedWorkerGlobalScope = {
        memory: {
            name: "",
            onmessage: null,
            onmessageerror: null,
            self: globalThis,
            location: rsvm.prototype.WorkerLocation.new(),
            onerror: null,
            onlanguagechange: null,
            navigator: rsvm.prototype.WorkerNavigator.new(),
            onrejectionhandled: null,
            onunhandledrejection: null,
            isSecureContext: true,
            origin: "http://127.0.0.1:8848",
            trustedTypes: rsvm.prototype.TrustedTypePolicyFactory.new(),
            performance: rsvm.prototype.Performance.new(),
            crypto: rsvm.prototype.Crypto.new(),
            indexedDB: rsvm.prototype.IDBFactory.new(),
            fonts: rsvm.prototype.FontFaceSet.new(),
            caches: rsvm.prototype.CacheStorage.new(),
            crossOriginIsolated: false,
            scheduler: rsvm.prototype.Scheduler.new(),
        },
    };
    rsvm.prototype.DedicatedWorkerGlobalScope.memory.clientInformation = rsvm.prototype.DedicatedWorkerGlobalScope.memory.navigator;
    rsvm.RsSetPrivateProperty(window, "__memory__", rsvm.prototype.DedicatedWorkerGlobalScope.memory);
    Object.defineProperties(window, {
        onmessage: {
            get: rsvm.RsCreateWindowGetter("onmessage", function onmessage() {
                return rsvm.get(globalThis, "onmessage");
            }), set: rsvm.RsCreateWindowSetter("onmessage", function onmessage() {
                rsvm.set(globalThis, "onmessage", arguments[0]);
            }), enumerable: true, configurable: true
        },
        onmessageerror: {
            get: rsvm.RsCreateWindowGetter("onmessageerror", function onmessageerror() {
                return rsvm.get(globalThis, "onmessageerror");
            }), set: rsvm.RsCreateWindowSetter("onmessageerror", function onmessageerror() {
                rsvm.set(globalThis, "onmessageerror", arguments[0]);
            }), enumerable: true, configurable: true
        },
        name: {
            get: rsvm.RsCreateWindowGetter("name", function name() {
                return rsvm.get(globalThis, "name");
            }), set: rsvm.RsCreateWindowSetter("name", function name() {
                rsvm.set(globalThis, "name", arguments[0]);
            }), enumerable: true, configurable: true
        },
        cancelAnimationFrame: {
            value: rsvm.RsCreateWindowAction("cancelAnimationFrame", 1, function cancelAnimationFrame() {
            }), writable: true, enumerable: true, configurable: true
        },
        close: {
            value: rsvm.RsCreateWindowAction("close", 0, function close() {
            }), writable: true, enumerable: true, configurable: true
        },
        postMessage: {
            value: rsvm.RsCreateWindowAction("postMessage", 1, function postMessage() {
                arguments[0] = JSON.stringify(arguments[0])
                rsvm.RsWorkerPostMessageToParent.apply(this, arguments);
            }), writable: true, enumerable: true, configurable: true
        },
        requestAnimationFrame: {
            value: rsvm.RsCreateWindowAction("requestAnimationFrame", 1, function requestAnimationFrame() {
            }), writable: true, enumerable: true, configurable: true
        },
        webkitRequestFileSystem: {
            value: rsvm.RsCreateWindowAction("webkitRequestFileSystem", 2, function webkitRequestFileSystem() {
            }), writable: true, enumerable: true, configurable: true
        },
        webkitRequestFileSystemSync: {
            value: rsvm.RsCreateWindowAction("webkitRequestFileSystemSync", 2, function webkitRequestFileSystemSync() {
            }), writable: true, enumerable: true, configurable: true
        },
        webkitResolveLocalFileSystemURL: {
            value: rsvm.RsCreateWindowAction("webkitResolveLocalFileSystemURL", 2, function webkitResolveLocalFileSystemURL() {
            }), writable: true, enumerable: true, configurable: true
        },
        webkitResolveLocalFileSystemSyncURL: {
            value: rsvm.RsCreateWindowAction("webkitResolveLocalFileSystemSyncURL", 1, function webkitResolveLocalFileSystemSyncURL() {
            }), writable: true, enumerable: true, configurable: true
        },
    });
}();

// !function () {
//     let WindowProperties = rsvm.RsCreateNameInterceptor({
//         getter(target, property) {
//             let flag = rsvm.log;
//             rsvm.prevent();
//             let value = undefined;
//             function travel(nodeArray) {
//                 for (const node of nodeArray) {
//                     if (value) { return; }
//                     if (rsvm.get(node, 'nodeType') == 1) {
//                         if (property == rsvm.get(node, 'id')) {
//                             value = node; return;
//                         }
//                         travel(rsvm.RsGetPrivateProperty(node, "nodeArray"));
//                     }
//                 }
//             }
//             let nodeArray = rsvm.RsGetPrivateProperty(window.document, "nodeArray");
//             if (nodeArray) {
//                 travel(nodeArray);
//             }
//             if (flag) {
//                 let msg = { '原型': this[Symbol.toStringTag], '属性': property, '返回值': value };
//                 rsvm.$log.call(console, msg);
//             }
//             rsvm.recover();
//             if (value) {
//                 return { value: value, intercept: true };
//             } else {
//                 return { intercept: false };
//             }
//         },
//         query(target, property) {
//             rsvm.prevent();
//             let value = undefined;
//             function travel(nodeArray) {
//                 for (const node of nodeArray) {
//                     if (value) { return; }
//                     if (rsvm.get(node, 'nodeType') == 1) {
//                         if (property == rsvm.get(node, 'id')) {
//                             value = node; return;
//                         }
//                         travel(rsvm.RsGetPrivateProperty(node, "nodeArray"));
//                     }
//                 }
//             }
//             let nodeArray = rsvm.RsGetPrivateProperty(window.document, "nodeArray");
//             if (nodeArray) {
//                 travel(nodeArray);
//             }
//             rsvm.recover();
//             if (value) {
//                 return {
//                     value: { value: value, writable: true, enumerable: false, configurable: true },
//                     intercept: true,
//                 };
//             } else {
//                 return { intercept: false };
//             }
//         },
//         descriptor(target, property) {
//             rsvm.prevent();
//             let value = undefined;
//             function travel(nodeArray) {
//                 for (const node of nodeArray) {
//                     if (value) { return; }
//                     if (rsvm.get(node, 'nodeType') == 1) {
//                         if (property == rsvm.get(node, 'id')) {
//                             value = node; return;
//                         }
//                         travel(rsvm.RsGetPrivateProperty(node, "nodeArray"));
//                     }
//                 }
//             }
//             let nodeArray = rsvm.RsGetPrivateProperty(window.document, "nodeArray");
//             if (nodeArray) {
//                 travel(nodeArray);
//             }
//             rsvm.recover();
//             if (value) {
//                 return {
//                     value: { value: value, writable: true, enumerable: false, configurable: true },
//                     intercept: true,
//                 };
//             } else {
//                 return { intercept: false };
//             }
//         }
//     });
//
//     rsvm.prototype.WindowProperties = {
//         memory: {},
//         malloc(target) {
//             rsvm.mallocBaseMemory("WindowProperties", target);
//         },
//         instance() {
//             return WindowProperties;
//         },
//         new() {
//             let obj = rsvm.RsCreate(WindowProperties);
//             return obj;
//         },
//     };
//
//     Object.defineProperties(WindowProperties, {
//         [Symbol.toStringTag]: { value: "WindowProperties", writable: false, enumerable: false, configurable: true, },
//     });
//
//     Object.setPrototypeOf(WindowProperties, EventTarget.prototype);
//     Object.setPrototypeOf(Window.prototype, WindowProperties);
// }();
