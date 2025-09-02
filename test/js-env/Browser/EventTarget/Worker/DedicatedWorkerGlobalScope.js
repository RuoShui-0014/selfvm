!function () {
    rsvm.prototype.DedicatedWorkerGlobalScope = {
        memory: {},
        malloc(target) {
            rsvm.mallocBaseMemory("DedicatedWorkerGlobalScope", target);
        },
        new() {
            let obj = rsvm.RsCreate(DedicatedWorkerGlobalScope.prototype);
            return obj;
        },
    };

    Object.defineProperties(DedicatedWorkerGlobalScope, {
        prototype: {
            value: DedicatedWorkerGlobalScope.prototype,
            writable: false,
            enumerable: false,
            configurable: false
        },
        TEMPORARY: {value: 0, writable: false, enumerable: true, configurable: false},
        PERSISTENT: {value: 1, writable: false, enumerable: true, configurable: false},
    });

    Object.defineProperties(DedicatedWorkerGlobalScope.prototype, {
        TEMPORARY: {value: 0, writable: false, enumerable: true, configurable: false,},
        PERSISTENT: {value: 1, writable: false, enumerable: true, configurable: false,},
        constructor: {writable: true, enumerable: false, configurable: true, value: DedicatedWorkerGlobalScope},
        [Symbol.toStringTag]: {
            value: "DedicatedWorkerGlobalScope",
            writable: false,
            enumerable: false,
            configurable: true,
        },
    });
    Object.setPrototypeOf(DedicatedWorkerGlobalScope, WorkerGlobalScope);
    Object.setPrototypeOf(DedicatedWorkerGlobalScope.prototype, WorkerGlobalScope.prototype);
}();