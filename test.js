// const addon = require('./build/Release/self-vm.node');
const svm = require('./cmake-build-debug/self-vm.node');

this.a = new svm.test();
console.log('1 + 2 =', this.a);

var a = new svm.test();

var isolate = new svm.Isolate();
console.log(isolate[Symbol.toStringTag]);
console.log(isolate.value);
