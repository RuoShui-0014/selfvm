!function () {
    Object.defineProperty(window, "Worker", {
        value: rsvm.RsCreateConstructor("Worker", 1, function Worker() {
            const url = arguments[0];

            // 初始化框架
            const isolate = new ivm.Isolate({ inspector: false, memoryLimit: 1024 });
            const context = isolate.createContextSync({ inspector: false, rsvm: true, worker: true, intercept: false });
            context.global.set('atobb', new ivm.Reference(function(...args) {
                return "";
            }));
            context.global.set('btoaa', function(...args) {
                return "";
            });
            context.eval(rsvm.workerCode);

            // 判断是否为已创建的blob地址，是的话则获取相应的代码执行
            if (Object.keys(rsvm.blobMap).includes(url)) {
                context.eval(rsvm.blobMap[url].content);
            }

            const obj = rsvm.prototype.Worker.new.apply(this, arguments);
            rsvm.set(obj, "isolate", isolate);
            rsvm.set(obj, "context", context);
            rsvm.RsSetWorker(isolate, obj);

            return obj;
        }), writable: true, enumerable: false, configurable: true
    });

    rsvm.prototype.Worker = {
        memory: {
            onmessage: null,
            onerror: null,
            isolate: null,
            context: null,
        },
        malloc(target) {
            rsvm.mallocBaseMemory("Worker", target);
        },
        new() {
            const obj = rsvm.RsCreate(Worker.prototype);
            return obj;
        },
    };

    Object.defineProperties(Worker, {
        prototype: {value: Worker.prototype, writable: false, enumerable: false, configurable: false},
    });

    Object.defineProperties(Worker.prototype, {
        onmessage: {
            get: rsvm.RsCreateGetter("onmessage", function onmessage() {
                return rsvm.get(this, "onmessage");
            }), set: rsvm.RsCreateSetter("onmessage", function onmessage() {
                rsvm.set(this, "onmessage", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        postMessage: {
            value: rsvm.RsCreateAction("postMessage", 1, function postMessage() {
                const ctx = rsvm.get(this, "context");
                rsvm.RsPostMessageToWorker(ctx, arguments[0]);
            }), writable: true, enumerable: true, configurable: true,
        },
        terminate: {
            value: rsvm.RsCreateAction("terminate", 0, function terminate() {
            }), writable: true, enumerable: true, configurable: true,
        },
        constructor: {writable: true, enumerable: false, configurable: true, value: Worker},
        onerror: {
            get: rsvm.RsCreateGetter("onerror", function onerror() {
                return rsvm.get(this, "onerror");
            }), set: rsvm.RsCreateSetter("onerror", function onerror() {
                rsvm.set(this, "onerror", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        [Symbol.toStringTag]: {value: "Worker", writable: false, enumerable: false, configurable: true,},
    });
    Object.setPrototypeOf(Worker, EventTarget);
    Object.setPrototypeOf(Worker.prototype, EventTarget.prototype);
}();