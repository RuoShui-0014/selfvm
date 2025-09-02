a = rsvm.atob.applySync(null, ["sdfsdf"]);

a = btoa('aaa')
b = atob(a)


// 创建一个包含 Worker 脚本的 Blob 对象
const workerBlob = new Blob([`
    onmessage = function(event) {
        console.log("Received message in worker", event.data);
        const ua = self.navigator.userAgent
        postMessage("Hello from worker: " + self.navigator.userAgent);
        postMessage(["Hello from worker: " + btoa("aaa")]);
    }
    postMessage(["Hello from worker: " + self.navigator.userAgent]);
`], { type: 'application/javascript' });

debugger;
const workerUrl = URL.createObjectURL(workerBlob);
const worker_threads = new Worker(workerUrl);

worker_threads.onmessage = function (event) {
    console.log("Received message from worker:", event.data);
    window.zhoujie = event.data;
};
// worker_threads.postMessage({test: "测试信息"});