#include "task.h"

#include "../isolate/isolate_holder.h"
#include "../utils/utils.h"

namespace svm {

AsyncInfo::AsyncInfo(std::shared_ptr<IsolateHolder> isolate_holder,
                     RemoteHandle<v8::Context> context,
                     RemoteHandle<v8::Promise::Resolver> resolver)
    : isolate_holder_(isolate_holder),
      context(context),
      resolver(resolver) {
  isolate_holder_->GetSchedulerPar()->KeepAlive();
}
AsyncInfo::~AsyncInfo() {
  isolate_holder_->GetSchedulerPar()->WillDie();
}
v8::Isolate* AsyncInfo::GetIsolateSel() const {
  return isolate_holder_->GetIsolateSel();
}
v8::Isolate* AsyncInfo::GetIsolatePar() const {
  return isolate_holder_->GetIsolatePar();
}
void AsyncInfo::PostHandleTaskToPar(std::unique_ptr<v8::Task> task) const {
  isolate_holder_->PostHandleTaskToPar(std::move(task));
}

}  // namespace svm
