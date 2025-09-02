!function () {
    Object.defineProperty(window, "HTMLIFrameElement", {
        value: rsvm.RsCreateConstructor("HTMLIFrameElement"), writable: true, enumerable: false, configurable: true
    });

    rsvm.prototype.HTMLIFrameElement = {
        memory: {
            context: null,
            src: "",
            srcdoc: "",
            name: "",
            sandbox: rsvm.prototype.DOMTokenList.new(),
            allowFullscreen: false,
            width: "",
            height: "",
            contentDocument: null,
            contentWindow: null,
            referrerPolicy: "",
            csp: "",
            allow: "",
            featurePolicy: rsvm.prototype.FeaturePolicy.new(),
            align: "",
            scrolling: "",
            frameBorder: "",
            longDesc: "",
            marginHeight: "",
            marginWidth: "",
            loading: "auto",
            credentialless: false,
            allowPaymentRequest: false,
            privateToken: "",
            browsingTopics: false,
        },
        malloc(target) {
            rsvm.mallocBaseMemory("HTMLIFrameElement", target);
        },
        new(ownerDocument) {
            let obj = rsvm.RsCreate(HTMLIFrameElement.prototype);
            rsvm.set(obj, "nodeName", "iframe".toLocaleUpperCase());
            rsvm.set(obj, "ownerDocument", ownerDocument);
            rsvm.set(obj, "localName", "iframe");
            rsvm.set(obj, "tagName", "iframe".toLocaleUpperCase());
            return obj;
        },
    };

    Object.defineProperties(HTMLIFrameElement, {
        prototype: { value: HTMLIFrameElement.prototype, writable: false, enumerable: false, configurable: false },
    });

    Object.defineProperties(HTMLIFrameElement.prototype, {
        src: {
            get: rsvm.RsCreateGetter("src", function src() {
                return rsvm.get(this, "src");
            }), set: rsvm.RsCreateSetter("src", function src() {
                rsvm.set(this, "src", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        srcdoc: {
            get: rsvm.RsCreateGetter("srcdoc", function srcdoc() {
                return rsvm.get(this, "srcdoc");
            }), set: rsvm.RsCreateSetter("srcdoc", function srcdoc() {
                rsvm.set(this, "srcdoc", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        name: {
            get: rsvm.RsCreateGetter("name", function name() {
                return rsvm.get(this, "name");
            }), set: rsvm.RsCreateSetter("name", function name() {
                rsvm.set(this, "name", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        sandbox: {
            get: rsvm.RsCreateGetter("sandbox", function sandbox() {
                return rsvm.get(this, "sandbox");
            }), set: rsvm.RsCreateSetter("sandbox", function sandbox() {
                rsvm.set(this, "sandbox", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        allowFullscreen: {
            get: rsvm.RsCreateGetter("allowFullscreen", function allowFullscreen() {
                return rsvm.get(this, "allowFullscreen");
            }), set: rsvm.RsCreateSetter("allowFullscreen", function allowFullscreen() {
                rsvm.set(this, "allowFullscreen", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        width: {
            get: rsvm.RsCreateGetter("width", function width() {
                return rsvm.get(this, "width");
            }), set: rsvm.RsCreateSetter("width", function width() {
                rsvm.set(this, "width", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        height: {
            get: rsvm.RsCreateGetter("height", function height() {
                return rsvm.get(this, "height");
            }), set: rsvm.RsCreateSetter("height", function height() {
                rsvm.set(this, "height", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        contentDocument: {
            get: rsvm.RsCreateGetter("contentDocument", function contentDocument() {
                if (this.isConnected) {
                    let context = rsvm.get(this, "context");
                    if (context === null) {
                        context = rsvm.RsCreateContext(rsvm.get(this, "src"));
                        rsvm.get(this, "context", context);
                        context.global.atobb = atob;
                        context.global.btoaa = btoa;
                        context.global.windowCode = rsvm.windowCode;
                        context.global.workerCode = rsvm.workerCode;
                        context.eval(rsvm.windowCode + `rsvm.siteInfo.html = "<html>\\n\\t<head></head>\\n\\t<body></body>\\n</html>";\nrsvm.build(false);`, "");
                    }
                    return context.global.document;
                } else {
                    return null;
                }
            }), set: undefined, enumerable: true, configurable: true,
        },
        contentWindow: {
            get: rsvm.RsCreateGetter("contentWindow", function contentWindow() {
                if (this.isConnected) {
                    let context = rsvm.get(this, "context");
                    if (context == null) {
                        context = rsvm.RsCreateContext(rsvm.get(this, "src"));
                        rsvm.get(this, "context", context);
                        context.global.atobb = atob;
                        context.global.btoaa = btoa;
                        context.global.windowCode = rsvm.windowCode;
                        context.global.workerCode = rsvm.workerCode;
                        context.eval(rsvm.windowCode + `rsvm.siteInfo.html = "<html>\\n\\t<head></head>\\n\\t<body></body>\\n</html>";\nrsvm.build(false);`, "");
                    }
                    return context.global;
                } else {
                    return null;
                }
            }), set: undefined, enumerable: true, configurable: true,
        },
        referrerPolicy: {
            get: rsvm.RsCreateGetter("referrerPolicy", function referrerPolicy() {
                return rsvm.get(this, "referrerPolicy");
            }), set: rsvm.RsCreateSetter("referrerPolicy", function referrerPolicy() {
                rsvm.set(this, "referrerPolicy", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        csp: {
            get: rsvm.RsCreateGetter("csp", function csp() {
                return rsvm.get(this, "csp");
            }), set: rsvm.RsCreateSetter("csp", function csp() {
                rsvm.set(this, "csp", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        allow: {
            get: rsvm.RsCreateGetter("allow", function allow() {
                return rsvm.get(this, "allow");
            }), set: rsvm.RsCreateSetter("allow", function allow() {
                rsvm.set(this, "allow", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        featurePolicy: {
            get: rsvm.RsCreateGetter("featurePolicy", function featurePolicy() {
                return rsvm.get(this, "featurePolicy");
            }), set: undefined, enumerable: true, configurable: true,
        },
        align: {
            get: rsvm.RsCreateGetter("align", function align() {
                return rsvm.get(this, "align");
            }), set: rsvm.RsCreateSetter("align", function align() {
                rsvm.set(this, "align", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        scrolling: {
            get: rsvm.RsCreateGetter("scrolling", function scrolling() {
                return rsvm.get(this, "scrolling");
            }), set: rsvm.RsCreateSetter("scrolling", function scrolling() {
                rsvm.set(this, "scrolling", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        frameBorder: {
            get: rsvm.RsCreateGetter("frameBorder", function frameBorder() {
                return rsvm.get(this, "frameBorder");
            }), set: rsvm.RsCreateSetter("frameBorder", function frameBorder() {
                rsvm.set(this, "frameBorder", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        longDesc: {
            get: rsvm.RsCreateGetter("longDesc", function longDesc() {
                return rsvm.get(this, "longDesc");
            }), set: rsvm.RsCreateSetter("longDesc", function longDesc() {
                rsvm.set(this, "longDesc", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        marginHeight: {
            get: rsvm.RsCreateGetter("marginHeight", function marginHeight() {
                return rsvm.get(this, "marginHeight");
            }), set: rsvm.RsCreateSetter("marginHeight", function marginHeight() {
                rsvm.set(this, "marginHeight", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        marginWidth: {
            get: rsvm.RsCreateGetter("marginWidth", function marginWidth() {
                return rsvm.get(this, "marginWidth");
            }), set: rsvm.RsCreateSetter("marginWidth", function marginWidth() {
                rsvm.set(this, "marginWidth", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        getSVGDocument: {
            value: rsvm.RsCreateAction("getSVGDocument", 0, function getSVGDocument() {
            }), writable: true, enumerable: true, configurable: true,
        },
        loading: {
            get: rsvm.RsCreateGetter("loading", function loading() {
                return rsvm.get(this, "loading");
            }), set: rsvm.RsCreateSetter("loading", function loading() {
                rsvm.set(this, "loading", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        credentialless: {
            get: rsvm.RsCreateGetter("credentialless", function credentialless() {
                return rsvm.get(this, "credentialless");
            }), set: rsvm.RsCreateSetter("credentialless", function credentialless() {
                rsvm.set(this, "credentialless", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        allowPaymentRequest: {
            get: rsvm.RsCreateGetter("allowPaymentRequest", function allowPaymentRequest() {
                return rsvm.get(this, "allowPaymentRequest");
            }), set: rsvm.RsCreateSetter("allowPaymentRequest", function allowPaymentRequest() {
                rsvm.set(this, "allowPaymentRequest", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        constructor: { writable: true, enumerable: false, configurable: true, value: HTMLIFrameElement },
        privateToken: {
            get: rsvm.RsCreateGetter("privateToken", function privateToken() {
                return rsvm.get(this, "privateToken");
            }), set: rsvm.RsCreateSetter("privateToken", function privateToken() {
                rsvm.set(this, "privateToken", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        browsingTopics: {
            get: rsvm.RsCreateGetter("browsingTopics", function browsingTopics() {
                return rsvm.get(this, "browsingTopics");
            }), set: rsvm.RsCreateSetter("browsingTopics", function browsingTopics() {
                rsvm.set(this, "browsingTopics", arguments[0]);
            }), enumerable: true, configurable: true,
        },
        [Symbol.toStringTag]: { value: "HTMLIFrameElement", writable: false, enumerable: false, configurable: true, },
    });
    Object.setPrototypeOf(HTMLIFrameElement, HTMLElement);
    Object.setPrototypeOf(HTMLIFrameElement.prototype, HTMLElement.prototype);
}();