!function () {
    Object.defineProperty(window, "URL", {
        value: rsvm.RsCreateConstructor("URL", 1, function URL() {

        }), writable: true, enumerable: false, configurable: true
    });

    rsvm.prototype.URL = {
        memory: {
            origin: "http://www.baidu.com",
            protocol: "http:",
            username: "",
            password: "",
            host: "www.baidu.com",
            hostname: "www.baidu.com",
            port: "",
            pathname: "/",
            search: "",
            searchParams: rsvm.prototype.URLSearchParams.new(),
            hash: "",
            href: "http://www.baidu.com/",
        },
        malloc(target) {
            rsvm.mallocBaseMemory("URL", target);
        },
        new() {
            let obj = rsvm.RsCreate(URL.prototype);
            return obj;
        },
    };

    Object.defineProperties(URL, {
        prototype: {value: URL.prototype, writable: false, enumerable: false, configurable: false},
        canParse: {
            value: rsvm.RsCreateAction("canParse", 1, function canParse() {
            }), writable: true, enumerable: true, configurable: true
        },
        parse: {
            value: rsvm.RsCreateAction("parse", 1, function parse() {
            }), writable: true, enumerable: true, configurable: true
        },
        createObjectURL: {
            value: rsvm.RsCreateAction("createObjectURL", 1, function createObjectURL() {
                if (arguments[0] instanceof Blob) {
                    // 假设blob地址为
                    const url = 'blob:http://127.0.0.1:8848/27912115-5086-4d15-9b64-ad60b6b0e7e5';
                    rsvm.blobMap[url] = {
                        content: rsvm.get(arguments[0], "content"),
                        type: rsvm.get(arguments[0], "type"),
                    }
                    return url;
                }
            }), writable: true, enumerable: true, configurable: true
        },
        revokeObjectURL: {
            value: rsvm.RsCreateAction("revokeObjectURL", 1, function revokeObjectURL() {
            }), writable: true, enumerable: true, configurable: true
        },
    });

    Object.defineProperties(URL.prototype, {
        origin: {
            get: rsvm.RsCreateGetter("origin", function origin() {
                return rsvm.get(this, "origin");
            }), set: undefined, enumerable: true, configurable: true,
        },
        protocol: {
            get: rsvm.RsCreateGetter("protocol", function protocol() {
                return rsvm.get(this, "protocol");
            }), set: rsvm.RsCreateSetter("protocol", function protocol() {
                rsvm.set(this, "protocol", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        username: {
            get: rsvm.RsCreateGetter("username", function username() {
                return rsvm.get(this, "username");
            }), set: rsvm.RsCreateSetter("username", function username() {
                rsvm.set(this, "username", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        password: {
            get: rsvm.RsCreateGetter("password", function password() {
                return rsvm.get(this, "password");
            }), set: rsvm.RsCreateSetter("password", function password() {
                rsvm.set(this, "password", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        host: {
            get: rsvm.RsCreateGetter("host", function host() {
                return rsvm.get(this, "host");
            }), set: rsvm.RsCreateSetter("host", function host() {
                rsvm.set(this, "host", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        hostname: {
            get: rsvm.RsCreateGetter("hostname", function hostname() {
                return rsvm.get(this, "hostname");
            }), set: rsvm.RsCreateSetter("hostname", function hostname() {
                rsvm.set(this, "hostname", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        port: {
            get: rsvm.RsCreateGetter("port", function port() {
                return rsvm.get(this, "port");
            }), set: rsvm.RsCreateSetter("port", function port() {
                rsvm.set(this, "port", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        pathname: {
            get: rsvm.RsCreateGetter("pathname", function pathname() {
                return rsvm.get(this, "pathname");
            }), set: rsvm.RsCreateSetter("pathname", function pathname() {
                rsvm.set(this, "pathname", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        search: {
            get: rsvm.RsCreateGetter("search", function search() {
                return rsvm.get(this, "search");
            }), set: rsvm.RsCreateSetter("search", function search() {
                rsvm.set(this, "search", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        searchParams: {
            get: rsvm.RsCreateGetter("searchParams", function searchParams() {
                return rsvm.get(this, "searchParams");
            }), set: undefined, enumerable: true, configurable: true,
        },
        hash: {
            get: rsvm.RsCreateGetter("hash", function hash() {
                return rsvm.get(this, "hash");
            }), set: rsvm.RsCreateSetter("hash", function hash() {
                rsvm.set(this, "hash", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        href: {
            get: rsvm.RsCreateGetter("href", function href() {
                return rsvm.get(this, "href");
            }), set: rsvm.RsCreateSetter("href", function href() {
                rsvm.set(this, "href", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        toJSON: {
            value: rsvm.RsCreateAction("toJSON", 0, function toJSON() {
            }), writable: true, enumerable: true, configurable: true,
        },
        toString: {
            value: rsvm.RsCreateAction("toString", 0, function toString() {
            }), writable: true, enumerable: true, configurable: true,
        },
        constructor: {writable: true, enumerable: false, configurable: true, value: URL},
        [Symbol.toStringTag]: {value: "URL", writable: false, enumerable: false, configurable: true,},
    });
}();