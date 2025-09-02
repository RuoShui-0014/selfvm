!function () {
    Object.defineProperty(window, "Blob", {
        value: rsvm.RsCreateConstructor("Blob", 0, function Blob() {
            const obj = rsvm.prototype.Blob.new.apply(this, arguments);
            if (arguments[0] instanceof Array) {
                rsvm.set(obj, "content", arguments[0][0]);
            }
            if (arguments[0] instanceof Array) {
                rsvm.set(obj, "size", ("" + arguments[0][0]).length);
            }
            if (arguments[1] instanceof Object) {
                rsvm.set(obj, "type", arguments[1].type);
            }
            return obj;
        }), writable: true, enumerable: false, configurable: true
    });

    rsvm.prototype.Blob = {
        memory: {
            content: null,
            size: 0,
            type: "",
        },
        malloc(target) {
            rsvm.mallocBaseMemory("Blob", target);
        },
        new() {
            let obj = rsvm.RsCreate(Blob.prototype);
            return obj;
        },
    };

    Object.defineProperties(Blob, {
        prototype: {value: Blob.prototype, writable: false, enumerable: false, configurable: false},
    });

    Object.defineProperties(Blob.prototype, {
        size: {
            get: rsvm.RsCreateGetter("size", function size() {
                return rsvm.get(this, "size");
            }), set: undefined, enumerable: true, configurable: true,
        },
        type: {
            get: rsvm.RsCreateGetter("type", function type() {
                return rsvm.get(this, "type");
            }), set: undefined, enumerable: true, configurable: true,
        },
        arrayBuffer: {
            value: rsvm.RsCreateAction("arrayBuffer", 0, function arrayBuffer() {
            }), writable: true, enumerable: true, configurable: true,
        },
        slice: {
            value: rsvm.RsCreateAction("slice", 0, function slice() {
            }), writable: true, enumerable: true, configurable: true,
        },
        stream: {
            value: rsvm.RsCreateAction("stream", 0, function stream() {
            }), writable: true, enumerable: true, configurable: true,
        },
        text: {
            value: rsvm.RsCreateAction("text", 0, function text() {
            }), writable: true, enumerable: true, configurable: true,
        },
        constructor: {writable: true, enumerable: false, configurable: true, value: Blob},
        [Symbol.toStringTag]: {value: "Blob", writable: false, enumerable: false, configurable: true,},
    });
}();