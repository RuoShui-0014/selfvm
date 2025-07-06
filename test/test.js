const svm = require("../index.js")

function sleep(time) {
    let start = Date.now()
    while (Date.now() - start <= time) {
    }
}

const isolate = new svm.Isolate();

// isolate有一个默认的context
const default_ctx = isolate.context

// context可通过eval进行代码的同步运行
const result = default_ctx.evalSync("this.a = {name: 'Jack', age: 18}")
console.log(`result = `, result);

// context可通过evalSync进行代码的异步运行
async function test(i = 0) {
    for (let i = 0; i < 10000; i++) {
        var a = {
            name: "Jack",
            age: 999,
            ary: new Array(10000)
        };
        a.ary[i] = a.ary;
    }
    const result = await default_ctx.evalAsync(`this.a = {name: 'Jack', age: 18}
    for (let i = 0; i < 10000; i++) {
        var a = {
            name: "Jack",
            age: 999,
            ary: new Array(10000)
        };
        a.ary[i] = a.ary;
    }`)
    console.log(Date.now());
    // console.log(`${i} -> result = ${result}`);
    // result.then((e) => {
    //     console.log(`success e = `, e);
    // }, (e) => {
    //     console.log(`failed e = `, e);
    // })
}

// console.log(process.memoryUsage())
// let start = Date.now()
// for (let i = 0; i < 100; i++) {
test();
// test();
// test();
// test();
// test();
// }
// console.log(Date.now() - start, "ms")
console.log(Date.now());
let all = 20;
let a = setInterval(function () {
    console.log("--------------------");
    test();
    console.log(Date.now());
}, 200)

let b = setInterval(function () {
    isolate.gc();
    svm.gc();
}, 3000)


let c = setTimeout(function () {
    clearInterval(a);
    clearInterval(b);
}, 20000)

// sleep(10000)
//
// // 释放isolate隔离实例的相关资源
// isolate.release();
// svm.gc();
//
// sleep(10000)
