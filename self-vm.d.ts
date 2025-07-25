declare module SelfVm {
    export function gc(): void;

    export class Isolate {
        /**
         * 创建一个新的 Isolate 实例
         */
        constructor();

        /**
         * 获取默认创建的context
         */
        readonly context: Context;

        /**
         * 低内存通知垃圾回收
         */
        gc(): void;


        /**
         * 获取isolate静态堆内存数据
         */
        getHeapStatistics(): {
            total_heap_size: number;
            total_heap_size_executable: number;
            total_physical_size: number;
            total_available_size: number;
            used_heap_size: number;
            heap_size_limit: number;
            malloced_memory: number;
            peak_malloced_memory: number;
            does_zap_garbage: number;
        };

        /**
         * 创建Context同步获取结果
         */
        createContext(): Context;

        /**
         * 创建Context异步获取结果
         */
        createContextAsync(): Promise<Context>;

        /**
         * 创建Script同步获取结果
         */
        createScript(code: string, filename?: string): Script;

        /**
         * 获取Session
         */
        readonly session : Session;
    }

    export class Context {
        /**
         * 运行代码同步获取结果
         */
        eval(code: string, filename?: string): any;

        /**
         * 运行代码异步获取结果
         */
        evalAsync(code: string, filename?: string): Promise<any>;
    }

    export class Script {
        /**
         * 运行代码同步获取结果
         */
        run(context: Context): any;

        /**
         * 运行代码异步获取结果
         */
        runAsync(context: Context): any;
    }

    export class Session {
        /**
         * 添加需要调试的context
         */
        addContext(context: Context): void;

        /**
         * 分发从客户端接收到的协议数据
         */
        dispatchMessage(msg: string): void;

        /**
         * 发送协议数据到客户端的回调
         */
        onResponse: (msg: string) => {};

        /**
         * 发送协议数据到客户端的回调
         */
        onNotification: (msg: string) => {};

        /**
         * 运行代码异步获取结果
         */
        dispose(): void;
    }
}
export = SelfVm;