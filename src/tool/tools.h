#pragma once

#include "utils.h"
#include "web/local_dom_window.h"

namespace svm {

class ExceptionMessages {
 public:
  static v8::Local<v8::String> ConstructorCalledAsFunction() {
    return v8_str(
        "Please use the 'new' operator, this DOM object constructor cannot be "
        "called as a function.");
  }
  static v8::Local<v8::String> ConstructorNew() {
    return v8_str("Illegal constructor");
  }
};

template <typename T>
class Hook {
 public:
  explicit Hook(T info) : info{info} {
    v8::Isolate* isolate{info.GetIsolate()};
    v8::Local context{isolate->GetCurrentContext()};
    node_global = ScriptWrappable::Unwrap<LocalDOMWindow>(context->Global());

    log = node_global->log;
    node_global->log = false;
    glog = node_global->glog;
    node_global->glog = false;
  }
  ~Hook() {
    node_global->log = log;
    node_global->glog = glog;
  }

  void Operation(v8::Local<v8::Object> handle, v8::Local<v8::Value> result) {
    if (log) {
      v8::Isolate* isolate{info.GetIsolate()};
      v8::Local context{isolate->GetCurrentContext()};

      v8::Local<v8::Value> source{};
      if (!(context->Global()
                ->Get(context, v8_str(isolate, "rsvm"))
                .ToLocal(&source) &&
            source->IsObject())) {
        return;
      }

      v8::Local<v8::Value> logFunction{};
      if (!(source.As<v8::Object>()
                ->Get(context, v8_str("logFunction"))
                .ToLocal(&logFunction) &&
            logFunction->IsObject())) {
        return;
      }

      v8::Local<v8::Value> callback{};
      if (!(logFunction.As<v8::Object>()
                ->Get(context, v8_str("operation"))
                .ToLocal(&callback) &&
            callback->IsFunction())) {
        return;
      }

      v8::Local<v8::Array> paramsArray{v8::Array::New(isolate, info.Length())};
      for (int i{0}; i < info.Length(); i++) {
        paramsArray->Set(context, i, info[i]);
      }
      v8::Local<v8::Value> params[]{
          info.This(), handle->Get(context, v8_str("name")).ToLocalChecked(),
          paramsArray, result};
      callback.As<v8::Function>()->Call(
          context, Null(isolate), sizeof(params) / sizeof(v8::Local<v8::Value>),
          params);
    }
  }
  void GlobalGet(v8::Local<v8::Value> property) {
    if (glog) {
      v8::Isolate* isolate{info.GetIsolate()};
      v8::Local context{isolate->GetCurrentContext()};
      v8::Local global{context->Global()};

      v8::Local<v8::Object> source{};
      if (!(global->Get(context, v8_str(isolate, "rsvm")).ToLocal(&source) &&
            source->IsObject())) {
        return;
      }

      v8::Local<v8::Object> logFunction{};
      if (!(source->Get(context, v8_str("logFunction")).ToLocal(&logFunction) &&
            logFunction->IsObject())) {
        return;
      }

      v8::Local<v8::Function> getter{};
      if (!(logFunction->Get(context, v8_str("windowGetter"))
                .ToLocal(&getter) &&
            getter->IsFunction())) {
        return;
      }

      v8::Local<v8::Value> params[]{
          v8_str("window"), property,
          global->Get(context, property).ToLocalChecked()};
      getter.As<v8::Function>()->Call(
          context, info.Holder(), sizeof(params) / sizeof(v8::Local<v8::Value>),
          params);
    }
  }
  void GlobalSet(v8::Local<v8::Value> property, v8::Local<v8::Value> value) {
    if (glog) {
      v8::Isolate* isolate{info.GetIsolate()};
      v8::Local context{isolate->GetCurrentContext()};
      v8::Local global{context->Global()};

      v8::Local<v8::Object> source{};
      if (!(global->Get(context, v8_str(isolate, "rsvm")).ToLocal(&source) &&
            source->IsObject())) {
        return;
      }

      v8::Local<v8::Object> logFunction{};
      if (!(source->Get(context, v8_str("logFunction")).ToLocal(&logFunction) &&
            logFunction->IsObject())) {
        return;
      }

      v8::Local<v8::Function> setter{};
      if (!(logFunction->Get(context, v8_str("windowSetter"))
                .ToLocal(&setter) &&
            setter->IsFunction())) {
        return;
      }

      v8::Local<v8::Value> params[]{
          v8_str("window"), property, value,
          global->Get(context, property).ToLocalChecked()};
      setter.As<v8::Function>()->Call(
          context, info.Holder(), sizeof(params) / sizeof(v8::Local<v8::Value>),
          params);
    }
  }

  T info;
  bool log{false};
  bool glog{false};
  LocalDOMWindow* node_global{nullptr};
};

template <typename T>
class InterceptHook {
 public:
  explicit InterceptHook(T info) {
    v8::Local<v8::Context> context{info.This()->GetCreationContextChecked()};
    node_global = ScriptWrappable::Unwrap<LocalDOMWindow>(context->Global());
    glog = node_global->glog;
    node_global->glog = false;
  }
  ~InterceptHook() { node_global->glog = glog; }

  bool glog{false};
  LocalDOMWindow* node_global{nullptr};
};

void RsCreateNameInterceptor(const v8::FunctionCallbackInfo<v8::Value>& info);
void RsCreateIndexInterceptor(const v8::FunctionCallbackInfo<v8::Value>& info);
void RsCreateInterceptor(const v8::FunctionCallbackInfo<v8::Value>& info);

void RsCreateDocumentAll(const v8::FunctionCallbackInfo<v8::Value>& info);
void RsCreateConstructor(const v8::FunctionCallbackInfo<v8::Value>& info);
void RsCreateFunction(const v8::FunctionCallbackInfo<v8::Value>& info);

void RsSetPrivateProperty(const v8::FunctionCallbackInfo<v8::Value>& info);
void RsGetPrivateProperty(const v8::FunctionCallbackInfo<v8::Value>& info);

v8::Local<v8::ObjectTemplate> CreateRsVM(v8::Isolate* isolate, bool intercept);

}  // namespace svm
