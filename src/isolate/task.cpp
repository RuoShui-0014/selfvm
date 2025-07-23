#include "task.h"

#include "../isolate/isolate_holder.h"
#include "../module/context_handle.h"
#include "../module/isolate_handle.h"
#include "../utils/utils.h"
#include "external_data.h"

namespace svm {

AsyncInfo::AsyncInfo(IsolateHandle* isolate_handle,
                     v8::Isolate* isolate,
                     RemoteHandle<v8::Context> context,
                     RemoteHandle<v8::Promise::Resolver> resolver)
    : isolate_handle(isolate_handle),
      isolate(isolate),
      context(context),
      resolver(resolver) {
  isolate_handle->GetSchedulerPar()->KeepAlive();
}
AsyncInfo::~AsyncInfo() {
  isolate_handle->GetSchedulerPar()->WillDie();
}

}  // namespace svm
