declare module SelfVm {
    export function gc(): void;

    export function sessionDispatchMessage(port: number, message: string): void;

    export function sessionDispose(port: number): void;


    export class Isolate {
        /**
         * 创建一个新的 Isolate 实例
         */
        constructor(params?: { memoryLimit: 128 });

        /**
         * 获取默认创建的context
         */
        readonly context: Context;

        /**
         * 获取Session
         */
        readonly session: Session;

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
         * 创建Script同步获取结果
         */
        createScriptAsync(code: string, filename?: string): Promise<Script>;

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
         * 释放隔离实例
         */
        release(): void;
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

        /**
         * 运行代码忽略结果，即不必同步等待
         */
        evalIgnored(code: string, filename?: string): any;
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

        /**
         * 运行代码忽略结果，即不必同步等待
         */
        runIgnored(context: Context): any;
    }

    export class Session {
        /**
         * 连接至会话端口
         */
        connect(port: number): void

        /**
         * 添加需要调试的context
         */
        addContext(context: Context, session_name: string): void;

        /**
         * 运行代码异步获取结果
         */
        dispose(): void;
    }
}
export = SelfVm;