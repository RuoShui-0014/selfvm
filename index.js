const path = require('path');
const bindings = require('bindings');

// 使用 bindings 加载原生模块
const addon = bindings('my-native-addon');

// 或者直接指定路径
// const addon = require('./build/Release/self-vm');

module.exports = addon;