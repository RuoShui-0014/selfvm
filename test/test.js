const svm = require("../index.js")

function sleep(time) {
    let start = Date.now()
    while (Date.now() - start <= time) {
    }
}

for (let i = 0; i < 10000; i++) {
    var isolate = new svm.Isolate();
    console.log(isolate[Symbol.toStringTag]);
    console.log(isolate.context);
    isolate.release();
}

sleep(5000)
svm.gc();

sleep(5000)