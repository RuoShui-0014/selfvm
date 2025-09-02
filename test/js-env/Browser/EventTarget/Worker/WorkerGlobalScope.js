!function () {
    Object.defineProperty(window, "WorkerGlobalScope", {
        value: rsvm.RsCreateConstructor("WorkerGlobalScope"), writable: true, enumerable: false, configurable: true
    });

    rsvm.prototype.WorkerGlobalScope = {
        memory: {
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
        malloc(target) {
            rsvm.mallocBaseMemory("WorkerGlobalScope", target);
        },
        new() {
            let obj = rsvm.RsCreate(WorkerGlobalScope.prototype);
            return obj;
        },
    };

    Object.defineProperties(WorkerGlobalScope, {
        prototype: {value: WorkerGlobalScope.prototype, writable: false, enumerable: false, configurable: false},
    });

    Object.defineProperties(WorkerGlobalScope.prototype, {
        self: {
            get: rsvm.RsCreateGetter("self", function self() {
                return rsvm.get(this, "self");
            }), set: undefined, enumerable: true, configurable: true,
        },
        location: {
            get: rsvm.RsCreateGetter("location", function location() {
                return rsvm.get(this, "location");
            }), set: undefined, enumerable: true, configurable: true,
        },
        onerror: {
            get: rsvm.RsCreateGetter("onerror", function onerror() {
                return rsvm.get(this, "onerror");
            }), set: rsvm.RsCreateSetter("onerror", function onerror() {
                rsvm.set(this, "onerror", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        onlanguagechange: {
            get: rsvm.RsCreateGetter("onlanguagechange", function onlanguagechange() {
                return rsvm.get(this, "onlanguagechange");
            }), set: rsvm.RsCreateSetter("onlanguagechange", function onlanguagechange() {
                rsvm.set(this, "onlanguagechange", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        navigator: {
            get: rsvm.RsCreateGetter("navigator", function navigator() {
                return rsvm.get(this, "navigator");
            }), set: undefined, enumerable: true, configurable: true,
        },
        onrejectionhandled: {
            get: rsvm.RsCreateGetter("onrejectionhandled", function onrejectionhandled() {
                return rsvm.get(this, "onrejectionhandled");
            }), set: rsvm.RsCreateSetter("onrejectionhandled", function onrejectionhandled() {
                rsvm.set(this, "onrejectionhandled", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        onunhandledrejection: {
            get: rsvm.RsCreateGetter("onunhandledrejection", function onunhandledrejection() {
                return rsvm.get(this, "onunhandledrejection");
            }), set: rsvm.RsCreateSetter("onunhandledrejection", function onunhandledrejection() {
                rsvm.set(this, "onunhandledrejection", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        isSecureContext: {
            get: rsvm.RsCreateGetter("isSecureContext", function isSecureContext() {
                return rsvm.get(this, "isSecureContext");
            }), set: undefined, enumerable: true, configurable: true,
        },
        origin: {
            get: rsvm.RsCreateGetter("origin", function origin() {
                return rsvm.get(this, "origin");
            }), set: rsvm.RsCreateSetter("origin", function origin() {
                rsvm.set(this, "origin", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        trustedTypes: {
            get: rsvm.RsCreateGetter("trustedTypes", function trustedTypes() {
                return rsvm.get(this, "trustedTypes");
            }), set: undefined, enumerable: true, configurable: true,
        },
        performance: {
            get: rsvm.RsCreateGetter("performance", function performance() {
                return rsvm.get(this, "performance");
            }), set: rsvm.RsCreateSetter("performance", function performance() {
                rsvm.set(this, "performance", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        crypto: {
            get: rsvm.RsCreateGetter("crypto", function crypto() {
                return rsvm.get(this, "crypto");
            }), set: undefined, enumerable: true, configurable: true,
        },
        indexedDB: {
            get: rsvm.RsCreateGetter("indexedDB", function indexedDB() {
                return rsvm.get(this, "indexedDB");
            }), set: undefined, enumerable: true, configurable: true,
        },
        fonts: {
            get: rsvm.RsCreateGetter("fonts", function fonts() {
                return rsvm.get(this, "fonts");
            }), set: undefined, enumerable: true, configurable: true,
        },
        createImageBitmap: {
            value: rsvm.RsCreateAction("createImageBitmap", 1, function createImageBitmap() {
            }), writable: true, enumerable: true, configurable: true,
        },
        fetch: {
            value: rsvm.RsCreateAction("fetch", 1, function fetch() {
            }), writable: true, enumerable: true, configurable: true,
        },
        importScripts: {
            value: rsvm.RsCreateAction("importScripts", 0, function importScripts() {
            }), writable: true, enumerable: true, configurable: true,
        },
        queueMicrotask: {
            value: rsvm.RsCreateAction("queueMicrotask", 1, function queueMicrotask() {
            }), writable: true, enumerable: true, configurable: true,
        },
        constructor: {writable: true, enumerable: false, configurable: true, value: WorkerGlobalScope},
        caches: {
            get: rsvm.RsCreateGetter("caches", function caches() {
                return rsvm.get(this, "caches");
            }), set: undefined, enumerable: true, configurable: true,
        },
        crossOriginIsolated: {
            get: rsvm.RsCreateGetter("crossOriginIsolated", function crossOriginIsolated() {
                return rsvm.get(this, "crossOriginIsolated");
            }), set: undefined, enumerable: true, configurable: true,
        },
        scheduler: {
            get: rsvm.RsCreateGetter("scheduler", function scheduler() {
                return rsvm.get(this, "scheduler");
            }), set: rsvm.RsCreateSetter("scheduler", function scheduler() {
                rsvm.set(this, "scheduler", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        atob: {
            value: rsvm.RsCreateAction("atob", 1, function atob() {
                return rsvm.atob.applySync(null, [arguments[0]]);
            }), writable: true, enumerable: true, configurable: true,
        },
        btoa: {
            value: rsvm.RsCreateAction("btoa", 1, function btoa() {
                postMessage("Hello from worker: " + arguments[0]);
                return rsvm.btoa();
            }), writable: true, enumerable: true, configurable: true,
        },
        clearInterval: {
            value: rsvm.RsCreateAction("clearInterval", 0, function clearInterval() {
            }), writable: true, enumerable: true, configurable: true,
        },
        clearTimeout: {
            value: rsvm.RsCreateAction("clearTimeout", 0, function clearTimeout() {
            }), writable: true, enumerable: true, configurable: true,
        },
        reportError: {
            value: rsvm.RsCreateAction("reportError", 1, function reportError() {
            }), writable: true, enumerable: true, configurable: true,
        },
        setInterval: {
            value: rsvm.RsCreateAction("setInterval", 1, function setInterval() {
            }), writable: true, enumerable: true, configurable: true,
        },
        setTimeout: {
            value: rsvm.RsCreateAction("setTimeout", 1, function setTimeout() {
            }), writable: true, enumerable: true, configurable: true,
        },
        structuredClone: {
            value: rsvm.RsCreateAction("structuredClone", 1, function structuredClone() {
            }), writable: true, enumerable: true, configurable: true,
        },
        [Symbol.toStringTag]: {value: "WorkerGlobalScope", writable: false, enumerable: false, configurable: true,},
    });
    Object.setPrototypeOf(WorkerGlobalScope, EventTarget);
    Object.setPrototypeOf(WorkerGlobalScope.prototype, EventTarget.prototype);
}();