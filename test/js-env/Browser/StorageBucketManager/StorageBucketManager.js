!function () {
    Object.defineProperty(window, "StorageBucketManager", {
        value: rsvm.RsCreateConstructor("StorageBucketManager"), writable: true, enumerable: false, configurable: true
    });

    rsvm.prototype.StorageBucketManager = {
        memory: {},
        malloc(target) {
            rsvm.mallocBaseMemory("StorageBucketManager", target);
        },
        new() {
            let obj = rsvm.RsCreate(StorageBucketManager.prototype);
            return obj;
        },
    };

    Object.defineProperties(StorageBucketManager, {
        prototype: {value: StorageBucketManager.prototype, writable: false, enumerable: false, configurable: false},
    });

    Object.defineProperties(StorageBucketManager.prototype, {
        delete: {
            value: rsvm.RsCreateAction("delete", 1, function delete_() {
            }), writable: true, enumerable: true, configurable: true,
        },
        keys: {
            value: rsvm.RsCreateAction("keys", 0, function keys() {
            }), writable: true, enumerable: true, configurable: true,
        },
        open: {
            value: rsvm.RsCreateAction("open", 1, function open() {
            }), writable: true, enumerable: true, configurable: true,
        },
        constructor: {writable: true, enumerable: false, configurable: true, value: StorageBucketManager},
        [Symbol.toStringTag]: {value: "StorageBucketManager", writable: false, enumerable: false, configurable: true,},
    });
}();