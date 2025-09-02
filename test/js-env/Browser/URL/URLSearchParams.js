!function () {
    Object.defineProperty(window, "URLSearchParams", {
        value: rsvm.RsCreateConstructor("URLSearchParams", 0, function URLSearchParams() {
            return rsvm.prototype.URLSearchParams.new.apply(null, arguments);
        }), writable: true, enumerable: false, configurable: true
    });

    rsvm.prototype.URLSearchParams = {
        memory: {
            size: 0,
        },
        malloc(target) {
            rsvm.mallocBaseMemory("URLSearchParams", target);
        },
        new() {
            let obj = rsvm.RsCreate(URLSearchParams.prototype);
            return obj;
        },
    };

    Object.defineProperties(URLSearchParams, {
        prototype: {value: URLSearchParams.prototype, writable: false, enumerable: false, configurable: false},
    });

    Object.defineProperties(URLSearchParams.prototype, {
        size: {
            get: rsvm.RsCreateGetter("size", function size() {
                return rsvm.get(this, "size");
            }), set: undefined, enumerable: true, configurable: true,
        },
        append: {
            value: rsvm.RsCreateAction("append", 2, function append() {
            }), writable: true, enumerable: true, configurable: true,
        },
        delete: {
            value: rsvm.RsCreateAction("delete", 1, function delete_() {
            }), writable: true, enumerable: true, configurable: true,
        },
        get: {
            value: rsvm.RsCreateAction("get", 1, function get() {
            }), writable: true, enumerable: true, configurable: true,
        },
        getAll: {
            value: rsvm.RsCreateAction("getAll", 1, function getAll() {
            }), writable: true, enumerable: true, configurable: true,
        },
        has: {
            value: rsvm.RsCreateAction("has", 1, function has() {
            }), writable: true, enumerable: true, configurable: true,
        },
        set: {
            value: rsvm.RsCreateAction("set", 2, function set() {
            }), writable: true, enumerable: true, configurable: true,
        },
        sort: {
            value: rsvm.RsCreateAction("sort", 0, function sort() {
            }), writable: true, enumerable: true, configurable: true,
        },
        toString: {
            value: rsvm.RsCreateAction("toString", 0, function toString() {
            }), writable: true, enumerable: true, configurable: true,
        },
        entries: {
            value: rsvm.RsCreateAction("entries", 0, function entries() {
            }), writable: true, enumerable: true, configurable: true,
        },
        forEach: {
            value: rsvm.RsCreateAction("forEach", 1, function forEach() {
            }), writable: true, enumerable: true, configurable: true,
        },
        keys: {
            value: rsvm.RsCreateAction("keys", 0, function keys() {
            }), writable: true, enumerable: true, configurable: true,
        },
        values: {
            value: rsvm.RsCreateAction("values", 0, function values() {
            }), writable: true, enumerable: true, configurable: true,
        },
        constructor: {writable: true, enumerable: false, configurable: true, value: URLSearchParams},
        [Symbol.toStringTag]: {value: "URLSearchParams", writable: false, enumerable: false, configurable: true,},
        [Symbol.iterator]: {
            value: rsvm.RsCreateAction("entries", 0, function entries() {
            }), writable: true, enumerable: false, configurable: true,
        },
    });
}();