!function () {
    Object.defineProperty(window, "WorkerNavigator", {
        value: rsvm.RsCreateConstructor("WorkerNavigator"), writable: true, enumerable: false, configurable: true
    });

    rsvm.prototype.WorkerNavigator = {
        memory: {
            hardwareConcurrency: 20,
            appCodeName: "Mozilla",
            appName: "Netscape",
            appVersion: "5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36",
            platform: "Win32",
            product: "Gecko",
            userAgent: "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36",
            language: "zh-CN",
            languages: ["zh-CN", "zh",],
            onLine: true,
            storageBuckets: rsvm.prototype.StorageBucketManager.new(),
            hid: rsvm.prototype.HID.new(),
            locks: rsvm.prototype.LockManager.new(),
            gpu: rsvm.prototype.GPU.new(),
            mediaCapabilities: rsvm.prototype.MediaCapabilities.new(),
            connection: rsvm.prototype.NetworkInformation.new(),
            permissions: rsvm.prototype.Permissions.new(),
            storage: rsvm.prototype.StorageManager.new(),
            deviceMemory: 8,
            userAgentData: rsvm.prototype.NavigatorUAData.new(),
            usb: rsvm.prototype.USB.new(),
            serial: rsvm.prototype.Serial.new(),
        },
        malloc(target) {
            rsvm.mallocBaseMemory("WorkerNavigator", target);
        },
        new() {
            let obj = rsvm.RsCreate(WorkerNavigator.prototype);
            return obj;
        },
    };

    Object.defineProperties(WorkerNavigator, {
        prototype: {value: WorkerNavigator.prototype, writable: false, enumerable: false, configurable: false},
    });

    Object.defineProperties(WorkerNavigator.prototype, {
        hardwareConcurrency: {
            get: rsvm.RsCreateGetter("hardwareConcurrency", function hardwareConcurrency() {
                return rsvm.get(this, "hardwareConcurrency");
            }), set: undefined, enumerable: true, configurable: true,
        },
        appCodeName: {
            get: rsvm.RsCreateGetter("appCodeName", function appCodeName() {
                return rsvm.get(this, "appCodeName");
            }), set: undefined, enumerable: true, configurable: true,
        },
        appName: {
            get: rsvm.RsCreateGetter("appName", function appName() {
                return rsvm.get(this, "appName");
            }), set: undefined, enumerable: true, configurable: true,
        },
        appVersion: {
            get: rsvm.RsCreateGetter("appVersion", function appVersion() {
                return rsvm.get(this, "appVersion");
            }), set: undefined, enumerable: true, configurable: true,
        },
        platform: {
            get: rsvm.RsCreateGetter("platform", function platform() {
                return rsvm.get(this, "platform");
            }), set: undefined, enumerable: true, configurable: true,
        },
        product: {
            get: rsvm.RsCreateGetter("product", function product() {
                return rsvm.get(this, "product");
            }), set: undefined, enumerable: true, configurable: true,
        },
        userAgent: {
            get: rsvm.RsCreateGetter("userAgent", function userAgent() {
                return rsvm.get(this, "userAgent");
            }), set: undefined, enumerable: true, configurable: true,
        },
        language: {
            get: rsvm.RsCreateGetter("language", function language() {
                return rsvm.get(this, "language");
            }), set: undefined, enumerable: true, configurable: true,
        },
        languages: {
            get: rsvm.RsCreateGetter("languages", function languages() {
                return rsvm.get(this, "languages");
            }), set: undefined, enumerable: true, configurable: true,
        },
        onLine: {
            get: rsvm.RsCreateGetter("onLine", function onLine() {
                return rsvm.get(this, "onLine");
            }), set: undefined, enumerable: true, configurable: true,
        },
        constructor: {writable: true, enumerable: false, configurable: true, value: WorkerNavigator},
        storageBuckets: {
            get: rsvm.RsCreateGetter("storageBuckets", function storageBuckets() {
                return rsvm.get(this, "storageBuckets");
            }), set: undefined, enumerable: true, configurable: true,
        },
        hid: {
            get: rsvm.RsCreateGetter("hid", function hid() {
                return rsvm.get(this, "hid");
            }), set: undefined, enumerable: true, configurable: true,
        },
        locks: {
            get: rsvm.RsCreateGetter("locks", function locks() {
                return rsvm.get(this, "locks");
            }), set: undefined, enumerable: true, configurable: true,
        },
        gpu: {
            get: rsvm.RsCreateGetter("gpu", function gpu() {
                return rsvm.get(this, "gpu");
            }), set: undefined, enumerable: true, configurable: true,
        },
        mediaCapabilities: {
            get: rsvm.RsCreateGetter("mediaCapabilities", function mediaCapabilities() {
                return rsvm.get(this, "mediaCapabilities");
            }), set: undefined, enumerable: true, configurable: true,
        },
        connection: {
            get: rsvm.RsCreateGetter("connection", function connection() {
                return rsvm.get(this, "connection");
            }), set: undefined, enumerable: true, configurable: true,
        },
        permissions: {
            get: rsvm.RsCreateGetter("permissions", function permissions() {
                return rsvm.get(this, "permissions");
            }), set: undefined, enumerable: true, configurable: true,
        },
        storage: {
            get: rsvm.RsCreateGetter("storage", function storage() {
                return rsvm.get(this, "storage");
            }), set: undefined, enumerable: true, configurable: true,
        },
        deviceMemory: {
            get: rsvm.RsCreateGetter("deviceMemory", function deviceMemory() {
                return rsvm.get(this, "deviceMemory");
            }), set: undefined, enumerable: true, configurable: true,
        },
        userAgentData: {
            get: rsvm.RsCreateGetter("userAgentData", function userAgentData() {
                return rsvm.get(this, "userAgentData");
            }), set: undefined, enumerable: true, configurable: true,
        },
        usb: {
            get: rsvm.RsCreateGetter("usb", function usb() {
                return rsvm.get(this, "usb");
            }), set: undefined, enumerable: true, configurable: true,
        },
        serial: {
            get: rsvm.RsCreateGetter("serial", function serial() {
                return rsvm.get(this, "serial");
            }), set: undefined, enumerable: true, configurable: true,
        },
        [Symbol.toStringTag]: {value: "WorkerNavigator", writable: false, enumerable: false, configurable: true,},
    });
}();