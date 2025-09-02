const fs = require("fs")
const Browser = require(`${__dirname}/../Browser/BrowserCode.js`);

function GetToolCode() {
    return "" + fs.readFileSync(`${__dirname}/rsvm.js`) + '\r\n';
}

function GetWindowCode() {
    let code = "";
    code += fs.readFileSync(`${__dirname}/rsvm.js`) + '\r\n';
    code += fs.readFileSync(`${__dirname}/parser.js`) + '\r\n';
    code += Browser.GetWindowCode();
    code += fs.readFileSync(`${__dirname}/fingerprint.js`) + '\r\n';
    return code;
}

function GetWorkerCode() {
    let code = "";
    code += fs.readFileSync(`${__dirname}/rsvm.js`) + '\r\n';
    code += fs.readFileSync(`${__dirname}/parser.js`) + '\r\n';
    code += Browser.GetWorkerCode();
    code += fs.readFileSync(`${__dirname}/fingerprint.js`) + '\r\n';
    return code;
}

module.exports = {
    GetWindowCode, GetWorkerCode, GetToolCode
}