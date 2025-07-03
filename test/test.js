const svm = require("../index.js")

this.a = new svm.test();
console.log('1 + 2 =', this.a);

var a = new svm.test();

for (let i = 0; i < 999; i++) {
    var isolate = new svm.Isolate();
    // console.log(isolate[Symbol.toStringTag]);
    console.log(isolate.context);
    svm.gc();
}
