#include "tools.h"

#include "utils.h"

namespace svm {

void NameGetter(v8::Local<v8::Name> property,
                const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook(info);

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("getter"))
                         .ToLocalChecked()
                         .As<v8::Function>()};

  v8::Local<v8::Value> params[]{info.This(), property};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      info.GetReturnValue().Set(
          intercept->Get(context, v8_str("value")).ToLocalChecked());
    }
  }
}
void NameSetter(v8::Local<v8::Name> property,
                v8::Local<v8::Value> value,
                const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook(info);

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("setter"))
                         .ToLocalChecked()
                         .As<v8::Function>()};

  v8::Local<v8::Value> params[]{info.This(), property, value};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      info.GetReturnValue().Set(
          intercept->Get(context, v8_str("value")).ToLocalChecked());
    }
  }
}
void NameQuery(v8::Local<v8::Name> property,
               const v8::PropertyCallbackInfo<v8::Integer>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook(info);

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("query"))
                         .ToLocalChecked()
                         .As<v8::Function>()};

  v8::Local<v8::Value> params[]{info.This(), property};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      v8::Local value{
          intercept->Get(context, v8_str("value")).ToLocalChecked()};
      int32_t rc{0};
      if (value->IsObject()) {
        v8::Local obj{value.As<v8::Object>()};
        if (obj->Get(context, v8_str("writable")).ToLocalChecked()->IsFalse()) {
          rc |= v8::PropertyAttribute::ReadOnly;
        }
        if (obj->Get(context, v8_str("enumerable"))
                .ToLocalChecked()
                ->IsFalse()) {
          rc |= v8::PropertyAttribute::DontEnum;
        }
        if (obj->Get(context, v8_str("configurable"))
                .ToLocalChecked()
                ->IsFalse()) {
          rc |= v8::PropertyAttribute::DontDelete;
        }
      }
      info.GetReturnValue().Set(rc);
    }
  }
}
void NameDefiner(v8::Local<v8::Name> property,
                 const v8::PropertyDescriptor& desc,
                 const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook(info);

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("definer"))
                         .ToLocalChecked()
                         .As<v8::Function>()};

  v8::Local obj{v8::Object::New(isolate)};
  if (desc.has_value()) {
    obj->Set(context, v8_str("value"), desc.value());
  }
  if (desc.has_get()) {
    obj->Set(context, v8_str("get"), desc.get());
  }
  if (desc.has_set()) {
    obj->Set(context, v8_str("set"), desc.set());
  }
  if (desc.has_writable()) {
    obj->Set(context, v8_str("writable"),
             v8::Boolean::New(isolate, desc.writable()));
  }
  if (desc.has_configurable()) {
    obj->Set(context, v8_str("configurable"),
             v8::Boolean::New(isolate, desc.configurable()));
  }
  if (desc.has_enumerable()) {
    obj->Set(context, v8_str("enumerable"),
             v8::Boolean::New(isolate, desc.enumerable()));
  }

  v8::Local<v8::Value> params[]{info.This(), property, obj};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      info.GetReturnValue().Set(
          intercept->Get(context, v8_str("value")).ToLocalChecked());
    }
  }
}
void NameDeleter(v8::Local<v8::Name> property,
                 const v8::PropertyCallbackInfo<v8::Boolean>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook(info);

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("deleter"))
                         .ToLocalChecked()
                         .As<v8::Function>()};

  v8::Local<v8::Value> params[]{info.This(), property};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      v8::Local value{
          intercept->Get(context, v8_str("value")).ToLocalChecked()};
      info.GetReturnValue().Set(v8::Boolean::New(isolate, value->IsTrue()));
    }
  }
}
void NameDescriptor(v8::Local<v8::Name> property,
                    const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook(info);

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("descriptor"))
                         .ToLocalChecked()
                         .As<v8::Function>()};

  v8::Local<v8::Value> params[]{info.This(), property};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      v8::Local obj{intercept->Get(context, v8_str("value")).ToLocalChecked()};
      if (!obj->IsUndefined()) {
        info.GetReturnValue().Set(obj);
      }
    }
  }
}
void NameEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook(info);

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("enumerator"))
                         .ToLocalChecked()
                         .As<v8::Function>()};

  v8::Local<v8::Value> params[]{info.This()};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      v8::Local obj{intercept->Get(context, v8_str("value")).ToLocalChecked()};
      if (obj->IsArray()) {
        v8::Local list{obj.As<v8::Array>()};
        v8::Local ary{v8::Array::New(isolate)};
        for (uint32_t i{0}; i < list->Length(); i++) {
          v8::Local value{list->Get(context, i).ToLocalChecked()};
          if (value->IsName()) {
            ary->Set(context, i, value);
          }
        }
        info.GetReturnValue().Set(ary);
      }
    }
  }
}

void IndexGetter(uint32_t index,
                 const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook(info);

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("getter"))
                         .ToLocalChecked()
                         .As<v8::Function>()};
  v8::Local<v8::Value> params[]{info.This(), v8::Number::New(isolate, index)};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      info.GetReturnValue().Set(
          intercept->Get(context, v8_str("value")).ToLocalChecked());
    }
  }
}
void IndexSetter(uint32_t index,
                 v8::Local<v8::Value> value,
                 const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook{info};

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("setter"))
                         .ToLocalChecked()
                         .As<v8::Function>()};
  v8::Local<v8::Value> params[]{info.This(), v8::Number::New(isolate, index),
                                value};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      info.GetReturnValue().Set(
          intercept->Get(context, v8_str("value")).ToLocalChecked());
    }
  }
}
void IndexQuery(uint32_t index,
                const v8::PropertyCallbackInfo<v8::Integer>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook{info};

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("query"))
                         .ToLocalChecked()
                         .As<v8::Function>()};

  v8::Local<v8::Value> params[]{info.This(), v8::Number::New(isolate, index)};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      v8::Local value{
          intercept->Get(context, v8_str("value")).ToLocalChecked()};
      int32_t rc{0};
      if (value->IsObject()) {
        v8::Local obj{value.As<v8::Object>()};
        if (obj->Get(context, v8_str("writable")).ToLocalChecked()->IsFalse()) {
          rc |= v8::PropertyAttribute::ReadOnly;
        }
        if (obj->Get(context, v8_str("enumerable"))
                .ToLocalChecked()
                ->IsFalse()) {
          rc |= v8::PropertyAttribute::DontEnum;
        }
        if (obj->Get(context, v8_str("configurable"))
                .ToLocalChecked()
                ->IsFalse()) {
          rc |= v8::PropertyAttribute::DontDelete;
        }
      }
      info.GetReturnValue().Set(rc);
    }
  }
}
void IndexDefiner(uint32_t index,
                  const v8::PropertyDescriptor& desc,
                  const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook{info};

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("definer"))
                         .ToLocalChecked()
                         .As<v8::Function>()};

  v8::Local obj{v8::Object::New(isolate)};
  if (desc.has_value()) {
    obj->Set(context, v8_str("value"), desc.value());
  }
  if (desc.has_get()) {
    obj->Set(context, v8_str("get"), desc.get());
  }
  if (desc.has_set()) {
    obj->Set(context, v8_str("set"), desc.set());
  }
  if (desc.has_writable()) {
    obj->Set(context, v8_str("writable"),
             v8::Boolean::New(isolate, desc.writable()));
  }
  if (desc.has_configurable()) {
    obj->Set(context, v8_str("configurable"),
             v8::Boolean::New(isolate, desc.configurable()));
  }
  if (desc.has_enumerable()) {
    obj->Set(context, v8_str("enumerable"),
             v8::Boolean::New(isolate, desc.enumerable()));
  }

  v8::Local<v8::Value> params[]{info.This(), v8::Number::New(isolate, index),
                                obj};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      info.GetReturnValue().Set(
          intercept->Get(context, v8_str("value")).ToLocalChecked());
    }
  }
}
void IndexDeleter(uint32_t index,
                  const v8::PropertyCallbackInfo<v8::Boolean>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook{info};

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("deleter"))
                         .ToLocalChecked()
                         .As<v8::Function>()};

  v8::Local<v8::Value> params[]{info.This(), v8::Number::New(isolate, index)};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      v8::Local value{
          intercept->Get(context, v8_str("value")).ToLocalChecked()};
      info.GetReturnValue().Set(v8::Boolean::New(isolate, value->IsTrue()));
    }
  }
}
void IndexDescriptor(uint32_t index,
                     const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook{info};

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("descriptor"))
                         .ToLocalChecked()
                         .As<v8::Function>()};

  v8::Local<v8::Value> params[]{info.This(), v8::Number::New(isolate, index)};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local intercept{result.As<v8::Object>()};
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      v8::Local obj{intercept->Get(context, v8_str("value")).ToLocalChecked()};
      if (!obj->IsUndefined()) {
        info.GetReturnValue().Set(obj);
      }
    }
  }
}
void IndexEnumerator(const v8::PropertyCallbackInfo<v8::Array>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  v8::HandleScope scope{isolate};
  InterceptHook get_hook{info};

  v8::Local handler{info.Data().As<v8::Object>()};
  v8::Local callback{handler->Get(context, v8_str("enumerator"))
                         .ToLocalChecked()
                         .As<v8::Function>()};

  v8::Local<v8::Value> params[]{info.This()};
  v8::MaybeLocal maybe_result{
      callback->Call(context, info.This(),
                     sizeof(params) / sizeof(v8::Local<v8::Value>), params)};
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result) && result->IsObject()) {
    v8::Local<v8::Object> intercept = result.As<v8::Object>();
    if (intercept->Get(context, v8_str("intercept"))
            .ToLocalChecked()
            ->IsTrue()) {
      v8::Local obj{intercept->Get(context, v8_str("value")).ToLocalChecked()};
      if (obj->IsArray()) {
        v8::Local list{obj.As<v8::Array>()};
        v8::Local ary{v8::Array::New(isolate)};
        for (uint32_t i{0}; i < list->Length(); i++) {
          v8::Local value{list->Get(context, i).ToLocalChecked()};
          if (value->IsNumber()) {
            ary->Set(context, i, value);
          }
        }
        info.GetReturnValue().Set(ary);
      }
    }
  }
}

void RsCreateNameInterceptor(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  if (info.Length() == 1 && info[0]->IsObject()) {
    v8::HandleScope scope{isolate};

    v8::Local handle{info[0].As<v8::Object>()};
    v8::NamedPropertyHandlerConfiguration nameHandler{
        static_cast<v8::GenericNamedPropertyGetterCallback>(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        handle,
        v8::PropertyHandlerFlags::kNone};
    if (handle->HasOwnProperty(context, v8_str("getter")).ToChecked()) {
      nameHandler.getter =
          reinterpret_cast<decltype(nameHandler.getter)>(NameGetter);
    }
    if (handle->HasOwnProperty(context, v8_str("setter")).ToChecked()) {
      nameHandler.setter =
          reinterpret_cast<decltype(nameHandler.setter)>(NameSetter);
    }
    if (handle->HasOwnProperty(context, v8_str("query")).ToChecked()) {
      nameHandler.query =
          reinterpret_cast<decltype(nameHandler.query)>(NameQuery);
    }
    if (handle->HasOwnProperty(context, v8_str("definer")).ToChecked()) {
      nameHandler.definer =
          reinterpret_cast<decltype(nameHandler.definer)>(NameDefiner);
    }
    if (handle->HasOwnProperty(context, v8_str("deleter")).ToChecked()) {
      nameHandler.deleter =
          reinterpret_cast<decltype(nameHandler.deleter)>(NameDeleter);
    }
    if (handle->HasOwnProperty(context, v8_str("descriptor")).ToChecked()) {
      nameHandler.descriptor =
          reinterpret_cast<decltype(nameHandler.descriptor)>(NameDescriptor);
    }
    if (handle->HasOwnProperty(context, v8_str("enumerator")).ToChecked()) {
      nameHandler.enumerator =
          reinterpret_cast<decltype(nameHandler.enumerator)>(NameEnumerator);
    }

    v8::Local obj_temp{v8::ObjectTemplate::New(isolate)};
    obj_temp->SetHandler(nameHandler);
    info.GetReturnValue().Set(obj_temp->NewInstance(context).ToLocalChecked());
    return;
  }
  isolate->ThrowException(v8::Exception::TypeError(v8_str(
      "arguments number is not 1 or arguments[0] type is not v8::Object")));
}
void RsCreateIndexInterceptor(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  if (info.Length() == 1 && info[0]->IsObject()) {
    v8::HandleScope scope{isolate};

    v8::Local handle{info[0].As<v8::Object>()};
    v8::IndexedPropertyHandlerConfiguration indexHandler{
        static_cast<v8::IndexedPropertyGetterCallback>(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        info[0],
        v8::PropertyHandlerFlags::kNone};
    if (handle->HasOwnProperty(context, v8_str("getter")).ToChecked()) {
      indexHandler.getter =
          reinterpret_cast<decltype(indexHandler.getter)>(IndexGetter);
    }
    if (handle->HasOwnProperty(context, v8_str("setter")).ToChecked()) {
      indexHandler.setter =
          reinterpret_cast<decltype(indexHandler.setter)>(IndexSetter);
    }
    if (handle->HasOwnProperty(context, v8_str("query")).ToChecked()) {
      indexHandler.query =
          reinterpret_cast<decltype(indexHandler.query)>(IndexQuery);
    }
    if (handle->HasOwnProperty(context, v8_str("definer")).ToChecked()) {
      indexHandler.definer =
          reinterpret_cast<decltype(indexHandler.definer)>(IndexDefiner);
    }
    if (handle->HasOwnProperty(context, v8_str("deleter")).ToChecked()) {
      indexHandler.deleter =
          reinterpret_cast<decltype(indexHandler.deleter)>(IndexDeleter);
    }
    if (handle->HasOwnProperty(context, v8_str("descriptor")).ToChecked()) {
      indexHandler.descriptor =
          reinterpret_cast<decltype(indexHandler.descriptor)>(IndexDescriptor);
    }
    if (handle->HasOwnProperty(context, v8_str("enumerator")).ToChecked()) {
      indexHandler.enumerator =
          reinterpret_cast<decltype(indexHandler.enumerator)>(IndexEnumerator);
    }

    v8::Local obj_temp{v8::ObjectTemplate::New(isolate)};
    obj_temp->SetHandler(indexHandler);
    info.GetReturnValue().Set(obj_temp->NewInstance(context).ToLocalChecked());
    return;
  }
  isolate->ThrowException(v8::Exception::TypeError(v8_str(
      "arguments number is not 1 or arguments[0] type is not v8::Object")));
}
void RsCreateInterceptor(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};

  if (info.Length() == 1 && info[0]->IsObject()) {
    v8::HandleScope scope{isolate};

    v8::Local handle{info[0].As<v8::Object>()};
    v8::NamedPropertyHandlerConfiguration nameHandler{
        static_cast<v8::GenericNamedPropertyGetterCallback>(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        handle,
        v8::PropertyHandlerFlags::kNone};
    v8::IndexedPropertyHandlerConfiguration indexHandler{
        static_cast<v8::IndexedPropertyGetterCallback>(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        handle,
        v8::PropertyHandlerFlags::kNone};
    if (handle->HasOwnProperty(context, v8_str("getter")).ToChecked()) {
      nameHandler.getter =
          reinterpret_cast<decltype(nameHandler.getter)>(NameGetter);
      indexHandler.getter =
          reinterpret_cast<decltype(indexHandler.getter)>(IndexGetter);
    }
    if (handle->HasOwnProperty(context, v8_str("setter")).ToChecked()) {
      nameHandler.setter =
          reinterpret_cast<decltype(nameHandler.setter)>(NameSetter);
      indexHandler.setter =
          reinterpret_cast<decltype(indexHandler.setter)>(IndexSetter);
    }
    if (handle->HasOwnProperty(context, v8_str("query")).ToChecked()) {
      nameHandler.query =
          reinterpret_cast<decltype(nameHandler.query)>(NameQuery);
      indexHandler.query =
          reinterpret_cast<decltype(indexHandler.query)>(IndexQuery);
    }
    if (handle->HasOwnProperty(context, v8_str("definer")).ToChecked()) {
      nameHandler.definer =
          reinterpret_cast<decltype(nameHandler.definer)>(NameDefiner);
      indexHandler.definer =
          reinterpret_cast<decltype(indexHandler.definer)>(IndexDefiner);
    }
    if (handle->HasOwnProperty(context, v8_str("deleter")).ToChecked()) {
      nameHandler.deleter =
          reinterpret_cast<decltype(nameHandler.deleter)>(NameDeleter);
      indexHandler.deleter =
          reinterpret_cast<decltype(indexHandler.deleter)>(IndexDeleter);
    }
    if (handle->HasOwnProperty(context, v8_str("descriptor")).ToChecked()) {
      nameHandler.descriptor =
          reinterpret_cast<decltype(nameHandler.descriptor)>(NameDescriptor);
      indexHandler.descriptor =
          reinterpret_cast<decltype(indexHandler.descriptor)>(IndexDescriptor);
    }
    if (handle->HasOwnProperty(context, v8_str("enumerator")).ToChecked()) {
      nameHandler.enumerator =
          reinterpret_cast<decltype(nameHandler.enumerator)>(NameEnumerator);
      indexHandler.enumerator =
          reinterpret_cast<decltype(indexHandler.enumerator)>(IndexEnumerator);
    }

    v8::Local obj_temp{v8::ObjectTemplate::New(isolate)};
    obj_temp->SetHandler(nameHandler);
    obj_temp->SetHandler(indexHandler);

    info.GetReturnValue().Set(obj_temp->NewInstance(context).ToLocalChecked());
    return;
  }
  isolate->ThrowException(v8::Exception::TypeError(v8_str(
      "arguments number is not 1 or arguments[0] type is not v8::Object")));
}

// 创建不可检测对象
void DocumentAll(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::Local context{isolate->GetCurrentContext()};
  v8::HandleScope scope{isolate};

  v8::Local handle{info.Data().As<v8::Object>()};
  v8::Local fun{handle->Get(context, v8_str("callAsv8::Function"))
                    .ToLocalChecked()
                    .As<v8::Function>()};
  v8::Local<v8::Value> params[10];
  for (int i{0}; i < info.Length(); i++) {
    params[i] = info[i];
  }
  v8::MaybeLocal result{fun->Call(context, info.This(), info.Length(), params)};
  info.GetReturnValue().Set(result.ToLocalChecked());
}
void RsCreateDocumentAll(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  const v8::Local context{isolate->GetCurrentContext()};

  if (info.Length() < 1) {
    const v8::Local error{v8::Exception::TypeError(
        v8_str("Failed to create HTMLAllCollection instance: 1 argument "
               "required, but only 0 present."))};
    isolate->ThrowException(error);
    return;
  }
  if (!info[0]->IsObject()) {
    const v8::Local error{v8::Exception::TypeError(
        v8_str("First argument type must be 'v8::Object' handle."))};
    isolate->ThrowException(error);
    return;
  }

  v8::HandleScope scope{isolate};

  v8::Local handle{info[0].As<v8::Object>()};
  v8::NamedPropertyHandlerConfiguration nameHandler{
      static_cast<v8::GenericNamedPropertyGetterCallback>(nullptr),
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      handle,
      v8::PropertyHandlerFlags::kNone};
  v8::IndexedPropertyHandlerConfiguration indexHandler{
      static_cast<v8::IndexedPropertyGetterCallback>(nullptr),
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      handle,
      v8::PropertyHandlerFlags::kNone};
  if (handle->HasOwnProperty(context, v8_str("getter")).ToChecked()) {
    nameHandler.getter =
        reinterpret_cast<decltype(nameHandler.getter)>(NameGetter);
    indexHandler.getter =
        reinterpret_cast<decltype(indexHandler.getter)>(IndexGetter);
  }
  if (handle->HasOwnProperty(context, v8_str("setter")).ToChecked()) {
    nameHandler.setter =
        reinterpret_cast<decltype(nameHandler.setter)>(NameSetter);
    indexHandler.setter =
        reinterpret_cast<decltype(indexHandler.setter)>(IndexSetter);
  }
  if (handle->HasOwnProperty(context, v8_str("query")).ToChecked()) {
    nameHandler.query =
        reinterpret_cast<decltype(nameHandler.query)>(NameQuery);
    indexHandler.query =
        reinterpret_cast<decltype(indexHandler.query)>(IndexQuery);
  }
  if (handle->HasOwnProperty(context, v8_str("definer")).ToChecked()) {
    nameHandler.definer =
        reinterpret_cast<decltype(nameHandler.definer)>(NameDefiner);
    indexHandler.definer =
        reinterpret_cast<decltype(indexHandler.definer)>(IndexDefiner);
  }
  if (handle->HasOwnProperty(context, v8_str("deleter")).ToChecked()) {
    nameHandler.deleter =
        reinterpret_cast<decltype(nameHandler.deleter)>(NameDeleter);
    indexHandler.deleter =
        reinterpret_cast<decltype(indexHandler.deleter)>(IndexDeleter);
  }
  if (handle->HasOwnProperty(context, v8_str("descriptor")).ToChecked()) {
    nameHandler.descriptor =
        reinterpret_cast<decltype(nameHandler.descriptor)>(NameDescriptor);
    indexHandler.descriptor =
        reinterpret_cast<decltype(indexHandler.descriptor)>(IndexDescriptor);
  }
  if (handle->HasOwnProperty(context, v8_str("enumerator")).ToChecked()) {
    nameHandler.enumerator =
        reinterpret_cast<decltype(nameHandler.enumerator)>(NameEnumerator);
    indexHandler.enumerator =
        reinterpret_cast<decltype(indexHandler.enumerator)>(IndexEnumerator);
  }

  const v8::Local obj_temp{v8::ObjectTemplate::New(isolate)};
  obj_temp->MarkAsUndetectable();
  obj_temp->SetCallAsFunctionHandler(DocumentAll, info[0]);
  obj_temp->SetHandler(nameHandler);
  obj_temp->SetHandler(indexHandler);

  info.GetReturnValue().Set(obj_temp->NewInstance(context).ToLocalChecked());
}

// 创建不可检测对象
void ConstructorErrorCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::HandleScope scope{isolate};
  v8::Local context{isolate->GetCurrentContext()};

  v8::MaybeLocal has{info.This()->GetPrivate(
      context, v8::Private::ForApi(isolate, v8_str(isolate, "__memory__")))};
  if (has.IsEmpty() || has.ToLocalChecked()->IsUndefined()) {
    isolate->ThrowException(
        v8::Exception::TypeError(v8_str(isolate, "Illegal constructor")));
  }
}
void ConstructorNoErrorCallback(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::HandleScope scope{isolate};

  if (!info.IsConstructCall()) {
    isolate->ThrowException(v8::Exception::TypeError(
        ExceptionMessages::ConstructorCalledAsFunction()));
    return;
  }

  v8::Local context{isolate->GetCurrentContext()};
  v8::Local global{context->Global()};

  LocalDOMWindow* node_global{ScriptWrappable::Unwrap<LocalDOMWindow>(global)};
  bool log{node_global->log};
  bool glog{node_global->glog};
  node_global->log = false;
  node_global->glog = false;

  v8::TryCatch tryCatch{isolate};

  v8::Local handle{info.Data().As<v8::Object>()};
  v8::Local callback{
      handle->Get(context, v8_str(isolate, "callback")).ToLocalChecked()};
  v8::Local<v8::Value> params[10]{};
  for (int i{0}; i < info.Length(); i++) {
    params[i] = info[i];
  }
  v8::Local result{Undefined(isolate).As<v8::Value>()};
  v8::MaybeLocal maybe_result{callback.As<v8::Function>()->Call(
      context, info.This(), info.Length(), params)};

  bool hasCaught{tryCatch.HasCaught()};
  if (hasCaught) {
    result = tryCatch.Exception();
    tryCatch.Reset();
  } else {
    if (maybe_result.ToLocal(&result)) {
      info.GetReturnValue().Set(result);
    }
  }

  v8::Local rsvm{
      global->Get(context, v8_str(isolate, "rsvm")).ToLocalChecked()};
  if (rsvm->IsObject() && log) {
    v8::Local logFunction{rsvm.As<v8::Object>()
                              ->Get(context, v8_str(isolate, "logFunction"))
                              .ToLocalChecked()};
    if (logFunction->IsObject()) {
      v8::Local func{logFunction.As<v8::Object>()
                         ->Get(context, v8_str(isolate, "constructor"))
                         .ToLocalChecked()};
      if (func->IsFunction()) {
        v8::Local paramsAry{v8::Array::New(isolate, info.Length())};
        for (int i{0}; i < info.Length(); i++) {
          paramsAry->Set(context, i, info[i]);
        }
        v8::Local<v8::Value> ps[]{
            handle->Get(context, v8_str(isolate, "name")).ToLocalChecked(),
            paramsAry, result};
        func.As<v8::Function>()->Call(context, Null(isolate),
                                      sizeof(ps) / sizeof(v8::Local<v8::Value>),
                                      ps);
      }
    }

    node_global->log = log;
    node_global->glog = glog;
  }

  if (hasCaught) {
    isolate->ThrowException(result);
    if (tryCatch.HasCaught()) {
      tryCatch.ReThrow();
    }
  }
}
void RsCreateConstructor(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  v8::HandleScope scope(isolate);
  v8::Local context{isolate->GetCurrentContext()};

  if (info.Length() == 1 && info[0]->IsString()) {
    v8::Local fun_temp{
        v8::FunctionTemplate::New(isolate, ConstructorErrorCallback, {}, {}, 0,
                                  v8::ConstructorBehavior::kAllow)};
    fun_temp->SetClassName(info[0].As<v8::String>());
    info.GetReturnValue().Set(fun_temp->GetFunction(context).ToLocalChecked());
  } else if (info.Length() == 3 && info[0]->IsString() && info[1]->IsNumber() &&
             info[2]->IsFunction()) {
    v8::Local handle{v8::Object::New(isolate)};
    handle->Set(context, v8_str(isolate, "name"), info[0]);
    handle->Set(context, v8_str(isolate, "length"), info[1]);
    handle->Set(context, v8_str(isolate, "callback"), info[2]);

    v8::Local fun_temp{v8::FunctionTemplate::New(
        isolate, ConstructorNoErrorCallback, handle, {},
        info[1].As<v8::Number>()->Value(), v8::ConstructorBehavior::kAllow)};
    fun_temp->SetClassName(info[0].As<v8::String>());

    info.GetReturnValue().Set(fun_temp->GetFunction(context).ToLocalChecked());
  } else {
    info.GetReturnValue().SetNull();
  }
}

// getter
void GetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::HandleScope scope(isolate);

  v8::Local<v8::Private> memory =
      v8::Private::ForApi(isolate, v8_str(isolate, "__memory__"));
  if (info.This()->IsNullOrUndefined() ||
      !info.This()->HasPrivate(context, memory).ToChecked()) {
    isolate->ThrowException(
        v8::Exception::TypeError(v8_str(isolate, "Illegal invocation")));
    return;
  }

  Hook get_hook(info);

  v8::Local<v8::Object> handle = info.Data().As<v8::Object>();
  v8::Local<v8::Value> callback =
      handle->Get(context, v8_str(isolate, "callback")).ToLocalChecked();
  v8::MaybeLocal<v8::Value> maybe_result =
      callback.As<v8::Function>()->Call(context, info.This(), 0, nullptr);
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result)) {
    info.GetReturnValue().Set(result);
  }
}
void RsCreateGetter(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::HandleScope scope(isolate);

  if (info.Length() == 2 && info[0]->IsString() && info[1]->IsFunction()) {
    v8::Local<v8::Object> handle = v8::Object::New(isolate);
    handle->Set(context, v8_str(isolate, "name"), info[0]);
    handle->Set(context, v8_str(isolate, "length"),
                v8::Number::New(isolate, 0));
    handle->Set(context, v8_str(isolate, "callback"), info[1]);

    v8::Local<v8::FunctionTemplate> fun_temp =
        v8::FunctionTemplate::NewWithCFunctionOverloads(
            isolate, GetterCallback, handle, v8::Local<v8::Signature>(), 0,
            v8::ConstructorBehavior::kThrow);
    fun_temp->SetClassName(v8::String::Concat(isolate, v8_str(isolate, "get "),
                                              info[0].As<v8::String>()));
    fun_temp->SetAcceptAnyReceiver(false);

    info.GetReturnValue().Set(fun_temp->GetFunction(context).ToLocalChecked());
  } else {
    info.GetReturnValue().SetUndefined();
  }
}

// setter
void SetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  //
  v8::Local<v8::Private> memory =
      v8::Private::ForApi(isolate, v8_str(isolate, "__memory__"));
  if (info.This()->IsNullOrUndefined() ||
      !info.This()->HasPrivate(context, memory).ToChecked()) {
    isolate->ThrowException(
        v8::Exception::TypeError(v8_str(isolate, "Illegal invocation")));
    return;
  }

  Hook set_hook(info);

  v8::Local<v8::Object> handle = info.Data().As<v8::Object>();
  v8::Local<v8::Function> callback =
      handle->Get(context, v8_str(isolate, "callback"))
          .ToLocalChecked()
          .As<v8::Function>();
  v8::Local<v8::Value> params[]{info[0]};
  v8::MaybeLocal<v8::Value> maybe_result =
      callback->Call(context, info.This(), 1, params);
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result)) {
    info.GetReturnValue().Set(result);
  }
}
void RsCreateSetter(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::HandleScope scope(isolate);

  if (info.Length() >= 2 && info[0]->IsString() && info[1]->IsFunction()) {
    v8::Local<v8::Object> handle = v8::Object::New(isolate);
    handle->Set(context, v8_str(isolate, "name"), info[0]);
    handle->Set(context, v8_str(isolate, "length"),
                v8::Number::New(isolate, 1));
    handle->Set(context, v8_str(isolate, "callback"), info[1]);

    v8::Local<v8::FunctionTemplate> fun_temp = v8::FunctionTemplate::New(
        isolate, SetterCallback, handle, v8::Local<v8::Signature>(), 1,
        v8::ConstructorBehavior::kThrow);
    fun_temp->SetClassName(v8::String::Concat(isolate, v8_str(isolate, "set "),
                                              info[0].As<v8::String>()));

    info.GetReturnValue().Set(fun_temp->GetFunction(context).ToLocalChecked());
  } else {
    info.GetReturnValue().SetUndefined();
  }
}

// action
void ActionCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  // Local<Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Context> context = info.This()->GetCreationContextChecked();

  // Local<Private> memory = Private::ForApi(isolate,
  // v8_str(isolate,"__memory__")); if (info.This()->IsNullOrUndefined() ||
  //   !info.This()->HasPrivate(context, memory).ToChecked()) {
  //     isolate->ThrowException(Exception::TypeError(v8_str(isolate,"Illegal
  //     invocation"))); return;
  // }

  Hook operation_hook(info);

  v8::Local<v8::Object> handle = info.Data().As<v8::Object>();
  v8::Local<v8::Value> callback =
      handle->Get(context, v8_str(isolate, "callback")).ToLocalChecked();
  v8::Local<v8::Value> params[10]{};
  for (int i = 0; i < info.Length(); i++) {
    params[i] = info[i];
  }

  v8::TryCatch tryCatch(isolate);
  v8::MaybeLocal<v8::Value> maybe_result = callback.As<v8::Function>()->Call(
      context, info.This(), info.Length(), params);

  v8::Local<v8::Value> result;
  if (tryCatch.HasCaught()) {
    result = tryCatch.Exception();
    tryCatch.Reset();
    operation_hook.Operation(handle, result);
    isolate->ThrowException(result);
    if (tryCatch.HasCaught()) {
      tryCatch.ReThrow();
    }
  } else {
    result = maybe_result.FromMaybe(Undefined(isolate).As<v8::Value>());
    info.GetReturnValue().Set(result);
    operation_hook.Operation(handle, result);
  }
}
void RsCreateAction(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::HandleScope scope(isolate);

  if (info.Length() == 3 && info[0]->IsString() && info[1]->IsNumber() &&
      info[2]->IsFunction()) {
    v8::Local<v8::Object> handle = v8::Object::New(isolate);
    handle->Set(context, v8_str(isolate, "name"), info[0]);
    handle->Set(context, v8_str(isolate, "length"), info[1]);
    handle->Set(context, v8_str(isolate, "callback"), info[2]);

    v8::Local<v8::FunctionTemplate> fun_temp = v8::FunctionTemplate::New(
        isolate, ActionCallback, handle, v8::Local<v8::Signature>(),
        info[1].As<v8::Number>()->Value(), v8::ConstructorBehavior::kThrow);
    fun_temp->SetClassName(info[0].As<v8::String>());

    info.GetReturnValue().Set(fun_temp->GetFunction(context).ToLocalChecked());
  } else {
    info.GetReturnValue().SetUndefined();
  }
}

// window getter
void WindowGetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::HandleScope scope(isolate);

  Hook get_hook(info);

  v8::Local<v8::Object> handle = info.Data().As<v8::Object>();
  v8::Local<v8::Function> callback =
      handle->Get(context, v8_str(isolate, "callback"))
          .ToLocalChecked()
          .As<v8::Function>();
  v8::MaybeLocal<v8::Value> maybe_result =
      callback->Call(context, info.This(), 0, nullptr);
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result)) {
    info.GetReturnValue().Set(result);
  }
}
void RsCreateWindowGetter(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::HandleScope scope(isolate);

  if (info.Length() == 2 && info[0]->IsString() && info[1]->IsFunction()) {
    v8::Local<v8::Object> handle = v8::Object::New(isolate);
    handle->Set(context, v8_str(isolate, "name"), info[0]);
    handle->Set(context, v8_str(isolate, "length"),
                v8::Number::New(isolate, 0));
    handle->Set(context, v8_str(isolate, "callback"), info[1]);

    v8::Local<v8::FunctionTemplate> fun_temp = v8::FunctionTemplate::New(
        isolate, WindowGetterCallback, handle, v8::Local<v8::Signature>(), 0,
        v8::ConstructorBehavior::kThrow);
    fun_temp->SetClassName(v8::String::Concat(isolate, v8_str(isolate, "get "),
                                              info[0].As<v8::String>()));

    info.GetReturnValue().Set(fun_temp->GetFunction(context).ToLocalChecked());
  } else {
    info.GetReturnValue().SetUndefined();
  }
}

// window setter
void WindowSetterCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  Hook set_hook(info);

  v8::Local<v8::Object> handle = info.Data().As<v8::Object>();
  v8::Local<v8::Function> callback =
      handle->Get(context, v8_str(isolate, "callback"))
          .ToLocalChecked()
          .As<v8::Function>();
  v8::Local<v8::Value> params[]{info[0]};
  v8::MaybeLocal<v8::Value> maybe_result =
      callback->Call(context, info.This(), 1, params);
  v8::Local<v8::Value> result;
  if (maybe_result.ToLocal(&result)) {
    info.GetReturnValue().Set(result);
  }
}
void RsCreateWindowSetter(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::HandleScope scope(isolate);

  if (info.Length() == 2 && info[0]->IsString() && info[1]->IsFunction()) {
    v8::Local<v8::Object> handle =
        v8::ObjectTemplate::New(isolate)->NewInstance(context).ToLocalChecked();
    handle->Set(context, v8_str(isolate, "name"), info[0]);
    handle->Set(context, v8_str(isolate, "length"),
                v8::Number::New(isolate, 1));
    handle->Set(context, v8_str(isolate, "callback"), info[1]);

    v8::Local<v8::FunctionTemplate> fun_temp = v8::FunctionTemplate::New(
        isolate, WindowSetterCallback, handle, v8::Local<v8::Signature>(), 1,
        v8::ConstructorBehavior::kThrow);
    fun_temp->SetClassName(v8::String::Concat(isolate, v8_str(isolate, "set "),
                                              info[0].As<v8::String>()));

    info.GetReturnValue().Set(fun_temp->GetFunction(context).ToLocalChecked());
  } else {
    info.GetReturnValue().SetUndefined();
  }
}

// window action
void WindowActionCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  Hook operation_hook(info);

  v8::Local<v8::Object> handle = info.Data().As<v8::Object>();
  v8::Local<v8::Function> callback =
      handle->Get(context, v8_str(isolate, "callback"))
          .ToLocalChecked()
          .As<v8::Function>();
  v8::Local<v8::Value> params[10]{};
  for (int i = 0; i < info.Length(); i++) {
    params[i] = info[i];
  }

  v8::TryCatch tryCatch(isolate);
  v8::MaybeLocal<v8::Value> maybe_result = callback.As<v8::Function>()->Call(
      context, info.This(), info.Length(), params);
  v8::Local<v8::Value> result;
  if (tryCatch.HasCaught()) {
    result = tryCatch.Exception();
    tryCatch.Reset();
    operation_hook.Operation(handle, result);
    isolate->ThrowException(result);
    if (tryCatch.HasCaught()) {
      tryCatch.ReThrow();
    }
  } else {
    result = maybe_result.FromMaybe(Undefined(isolate).As<v8::Value>());
    info.GetReturnValue().Set(result);
    operation_hook.Operation(handle, result);
  }
}
void RsCreateWindowAction(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::HandleScope scope(isolate);

  if (info.Length() == 3 && info[0]->IsString() && info[1]->IsNumber() &&
      info[2]->IsFunction()) {
    v8::Local<v8::Object> handle = v8::Object::New(isolate);
    handle->Set(context, v8_str(isolate, "name"), info[0]);
    handle->Set(context, v8_str(isolate, "length"), info[1]);
    handle->Set(context, v8_str(isolate, "callback"), info[2]);

    v8::Local<v8::FunctionTemplate> fun_temp = v8::FunctionTemplate::New(
        isolate, WindowActionCallback, handle, v8::Local<v8::Signature>(),
        info[1].As<v8::Number>()->Value(), v8::ConstructorBehavior::kThrow);
    fun_temp->SetClassName(info[0].As<v8::String>());

    info.GetReturnValue().Set(fun_temp->GetFunction(context).ToLocalChecked());
  } else {
    info.GetReturnValue().SetUndefined();
  }
}

// 可创建一般函数和构造函数
void RsCreateFunctionCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  const v8::Local context{isolate->GetCurrentContext()};
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Value> params[10]{};
  for (int i{0}; i < info.Length(); i++) {
    params[i] = info[i];
  }
  const v8::Local func_callback{info.Data()};
  const v8::MaybeLocal maybe_result{func_callback.As<v8::Function>()->Call(
      context, info.This(), info.Length(), params)};
  v8::Local<v8::Value> result;
  if (!maybe_result.ToLocal(&result)) {
    return;
  }

  if (info.IsConstructCall()) {
    info.GetReturnValue().Set(info.This());
  } else {
    info.GetReturnValue().Set(result);
  }
}
void RsCreateFunction(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  if (info.Length() == 4 && info[0]->IsBoolean() && info[1]->IsString() &&
      info[2]->IsNumber() && info[3]->IsFunction()) {
    const v8::Local context{isolate->GetCurrentContext()};

    const int32_t length{
        info[2].As<v8::Number>()->Int32Value(context).ToChecked()};
    const v8::Local func_temp{v8::FunctionTemplate::New(
        isolate, RsCreateFunctionCallback, info[3], {}, length,
        info[0]->IsTrue() ? v8::ConstructorBehavior::kAllow
                          : v8::ConstructorBehavior::kThrow)};
    func_temp->SetClassName(info[1].As<v8::String>());
    info.GetReturnValue().Set(func_temp->GetFunction(context).ToLocalChecked());
  } else {
    isolate->ThrowException(
        v8::Exception::TypeError(v8_str(isolate, R"(此函数需要4个参数.
第一个为bool值，true为创建构造函数，反之为一般函数.
第二个为string值，为函数名.
第三个为number值，为函数参数数量.
第四个为v8::Function，为函数需要执行的逻辑.)")));
  }
}

// 给js对象在c++层面设置/获取属性
void RsSetPrivateProperty(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();

  if (info.Length() == 3 && info[0]->IsObject() && info[1]->IsString()) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Maybe<bool> isok = info[0].As<v8::Object>()->SetPrivate(
        context, v8::Private::ForApi(isolate, info[1].As<v8::String>()),
        info[2]);
    if (isok.ToChecked()) {
      info.GetReturnValue().Set(info[0].As<v8::Object>());
      return;
    }
  }

  isolate->ThrowException(v8::Exception::TypeError(
      v8_str(isolate, "Need 3 args, args[0] is object, args[1] is string!!!")));
}
void RsGetPrivateProperty(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();

  if (info.Length() == 2 && info[0]->IsObject() && info[1]->IsString()) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> value =
        info[0]
            .As<v8::Object>()
            ->GetPrivate(context,
                         v8::Private::ForApi(isolate, info[1].As<v8::String>()))
            .ToLocalChecked();
    info.GetReturnValue().Set(value);
    return;
  }
  isolate->ThrowException(v8::Exception::TypeError(
      v8_str(isolate, "Need 2 args, args[0] is object, args[1] is string!!!")));
}

// 同 eval
void RsRunHTMLScript(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info[0]->IsString()) {
    v8::Isolate* isolate = info.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::HandleScope scope(isolate);

    auto source = info[0].As<v8::String>();
    v8::Local<v8::Script> script =
        v8::Script::Compile(context, source).ToLocalChecked();
    if (!script.IsEmpty()) {
      v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
      info.GetReturnValue().Set(result);
    }
  }
}
void RsRunMicrotask(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate{info.GetIsolate()};
  const v8::Local context{isolate->GetCurrentContext()};
  context->GetMicrotaskQueue()->PerformCheckpoint(isolate);
}
void RsSetWorker(const v8::FunctionCallbackInfo<v8::Value>& info) {}

void RsPostMessageToWorker(const v8::FunctionCallbackInfo<v8::Value>& info) {}
void RsWorkerPostMessageToParent(
    const v8::FunctionCallbackInfo<v8::Value>& info) {}

void CtxEval(const v8::FunctionCallbackInfo<v8::Value>& info) {}
void CtxPostMessage(const v8::FunctionCallbackInfo<v8::Value>& info) {}
void CtxGetterGlobal(const v8::FunctionCallbackInfo<v8::Value>& info) {}

void RsCreateContext(const v8::FunctionCallbackInfo<v8::Value>& info) {}
void RsPostMessage(const v8::FunctionCallbackInfo<v8::Value>& info) {}
void RsCreateStaticFunction(const v8::FunctionCallbackInfo<v8::Value>& info) {
  if (info.Length() == 3 && info[0]->IsString() && info[1]->IsNumber() &&
      info[2]->IsFunction()) {
    v8::Isolate* isolate = info.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    int length = info[1].As<v8::Number>()->Value();
    v8::Local<v8::Function> fun_temp =
        v8::Function::New(context, RsCreateFunctionCallback, info[2], length,
                          v8::ConstructorBehavior::kThrow)
            .ToLocalChecked();
    fun_temp->SetName(info[0].As<v8::String>());
    info.GetReturnValue().Set(fun_temp);
  }
}

v8::Local<v8::ObjectTemplate> CreateRsVM(v8::Isolate* isolate, bool intercept) {
  v8::Local instance_template{v8::ObjectTemplate::New(isolate)};

  if (intercept) {
    AttributeItem attributeItem[]{
        {"log", LocalDOMWindow::LogGetCallback, LocalDOMWindow::LogSetCallback,
         v8::PropertyAttribute::DontDelete},
        {"glog", LocalDOMWindow::GlogGetCallback,
         LocalDOMWindow::GlogSetCallback, v8::PropertyAttribute::DontDelete},
    };
    v8_defineAttributes(isolate, instance_template, attributeItem);
  } else {
    AttributeItem attributeItem[]{
        {"log", LocalDOMWindow::LogGetCallback, LocalDOMWindow::LogSetCallback,
         v8::PropertyAttribute::DontDelete},
    };
    v8_defineAttributes(isolate, instance_template, attributeItem);
  }

  OperationItem operationTable[] = {
      {"RsCreateDocumentAll", 1, RsCreateDocumentAll,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateInterceptor", 1, RsCreateInterceptor,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateNameInterceptor", 1, RsCreateNameInterceptor,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateIndexInterceptor", 1, RsCreateIndexInterceptor,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateGetter", 2, RsCreateGetter,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateSetter", 2, RsCreateSetter,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateAction", 3, RsCreateAction,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateFunction", 3, RsCreateFunction,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateConstructor", 2, RsCreateConstructor,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateWindowGetter", 2, RsCreateWindowGetter,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateWindowSetter", 1, RsCreateWindowSetter,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateWindowAction", 3, RsCreateWindowAction,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsSetPrivateProperty", 3, RsSetPrivateProperty,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsGetPrivateProperty", 2, RsGetPrivateProperty,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsRunHTMLScript", 1, RsRunHTMLScript,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsRunMicrotask", 0, RsRunMicrotask,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateContext", 0, RsCreateContext,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsPostMessage", 1, RsPostMessage,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsSetWorker", 1, RsSetWorker,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsPostMessageToWorker", 1, RsPostMessageToWorker,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsWorkerPostMessageToParent", 1, RsWorkerPostMessageToParent,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      {"RsCreateStaticFunction", 3, RsCreateStaticFunction,
       static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::DontEnum |
                                          v8::PropertyAttribute::DontDelete)},
      //{"RsRequest", 3, RsRequest,
      // static_cast<PropertyAttribute>(PropertyAttribute::DontEnum |
      //                                PropertyAttribute::DontDelete)},
  };
  v8_defineOperations(isolate, instance_template, operationTable);

  return instance_template;
}

}  // namespace svm
