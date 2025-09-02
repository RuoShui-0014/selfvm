#pragma once

#include <v8-isolate.h>

#include <unordered_map>

#include "base/raw_ptr.h"
#include "isolate/per_isolate_data.h"
#include "isolate/scheduler.h"
#include "native/timer_manager.h"
#include "utils/utils.h"

namespace svm {

struct IsolateParams {
  v8::Isolate* isolate_par;
  size_t memory_limit;
};

class IsolateHolder {
 public:
  using ContextId = v8::Context*;
  using ScriptId = v8::UnboundScript*;

  explicit IsolateHolder(IsolateParams& params);
  ~IsolateHolder();

  v8::Isolate* GetIsolateSel() const { return isolate_sel_; }
  v8::Isolate* GetIsolatePar() const { return isolate_par_; }

  Scheduler* GetSchedulerSel() const { return scheduler_sel_.get(); }
  Scheduler* GetSchedulerPar() const { return scheduler_par_.get(); }

  TimerManager* GetTimerManagerSel() const {
    return scheduler_sel_->GetTimerManager();
  }
  TimerManager* GetTimerManagerPar() const {
    return scheduler_par_->GetTimerManager();
  }

  void PostTaskToSel(std::unique_ptr<v8::Task> task,
                     Scheduler::TaskType type) const;
  uint32_t PostDelayedTaskToSel(std::unique_ptr<v8::Task> task,
                                uint64_t ms,
                                Timer::Type type) const;

  void PostTaskToPar(std::unique_ptr<v8::Task> task,
                     Scheduler::TaskType type) const;
  void PostDelayedTaskToPar(std::unique_ptr<v8::Task> task,
                            double delay_in_seconds) const;

  ContextId CreateContext();
  v8::Local<v8::Context> GetContext(ContextId address);
  void ClearContext(ContextId address);

  void CreateScript(v8::Local<v8::UnboundScript> unbound_script);
  v8::Local<v8::UnboundScript> GetScript(ScriptId address);
  void ClearScript(ScriptId address);

  void Release();

 private:
  IsolateParams isolate_params_;

  v8::Isolate* isolate_par_{nullptr};
  v8::Isolate* isolate_sel_{nullptr};

  base::raw_ptr<Scheduler> scheduler_par_{nullptr};
  std::unique_ptr<UVSchedulerSel> scheduler_sel_;
  std::unique_ptr<PerIsolateData> per_isolate_data_;

  std::mutex mutex_context_map_;
  std::mutex mutex_unbound_script_map_;
  std::unordered_map<ContextId, RemoteHandle<v8::Context>> context_map_;
  std::unordered_map<ScriptId, RemoteHandle<v8::UnboundScript>>
      unbound_script_map_;
};

}  // namespace svm
