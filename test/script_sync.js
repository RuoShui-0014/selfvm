const {svm} = require('../self-vm');
const measurePerformance = require('./times.js')

const isolate = new svm.Isolate();
const ctx = isolate.context

measurePerformance(() => {
    const script = isolate.createScript("this.a = {name: 'Jack', age: 18};", "filename.js");
    const result = script.run(ctx)
}, 10000);
