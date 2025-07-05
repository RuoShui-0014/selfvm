//
// Created by ruoshui on 25-7-5.
//

#pragma once

#include <v8.h>

#include "external_data.h"

namespace svm {

class ScriptTask : public v8::Task {
 public:
  struct CallInfo {
    v8::Isolate* isolate;
    v8::Local<v8::Context> context;
    std::string script;
  };
  struct ResultInfo {
    v8::Isolate* isolate;
    v8::Local<v8::Context> context;
    v8::Local<v8::Value>* result;
  };
  ScriptTask(CallInfo& source, ResultInfo& target);
  ~ScriptTask() override;
  void Run() override;

 private:
  CallInfo& source_;
  ResultInfo& target_;
};

class AsyncManager;
class ScriptTaskAsync : public v8::Task {
 public:
  struct CallInfo {
    v8::Isolate* isolate;
    v8::Global<v8::Context> context;
    std::string script;
  };
  struct ResultInfo {
    AsyncManager* async_manager;
    v8::Isolate* isolate;
    v8::Global<v8::Context> context;
    v8::Global<v8::Promise::Resolver> resolver;
  };
  ScriptTaskAsync(CallInfo& call_info, ResultInfo& result_info);
  ~ScriptTaskAsync() override;
  void Run() override;

 private:
  CallInfo call_info_;
  ResultInfo result_info_;
};

class DeserializeTaskAsync : public v8::Task {
public:
  DeserializeTaskAsync(ScriptTaskAsync::ResultInfo& result_info,
                       std::pair<uint8_t*, size_t>& buff);
  ~DeserializeTaskAsync() override;

  void Run() override;

private:
  ScriptTaskAsync::ResultInfo result_info_;
  std::pair<uint8_t*, size_t> buff_;
};

}  // namespace svm
