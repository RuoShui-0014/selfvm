const svm = require("../self-vm")

const isolate = new svm.Isolate();

const createContext = [];
const createContextAsync = [];

// 同步创建新context
let num = 1000;
console.time("createContext")
for (let i = 0; i < num; i++) {
    const ctx = isolate.createContext();
    createContext.push(ctx);
}
console.timeEnd("createContext")


// 异步创建新context
console.time("createContextAsync")
for (let i = 0; i < num; i++) {
    isolate.createContextAsync().then(ctx => {
        createContextAsync.push(ctx);
        if (createContextAsync.length === num) {
            console.timeEnd("createContextAsync")
        }
    });
}
