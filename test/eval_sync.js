const {svm} = require('../self-vm');
const measurePerformance = require('./times.js')

const isolate = new svm.Isolate();
const ctx = isolate.context

measurePerformance(() => {
    ctx.eval(`
        this.a = {name: 'Jack', age: 18};
        JSON.stringify(this.a);`, "filename.js");
}, 100);
