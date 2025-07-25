//
// Created by ruoshui on 25-7-3.
//

#pragma once
#include <v8-isolate.h>

#include "../utils/utils.h"
#include "per_isolate_data.h"
#include "scheduler.h"

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

  void PostTaskToSel(std::unique_ptr<v8::Task> task) const;
  void PostTaskToPar(std::unique_ptr<v8::Task> task) const;
  void PostDelayedTaskToSel(std::unique_ptr<v8::Task> task,
                            double delay_in_seconds) const;
  void PostDelayedTaskToPar(std::unique_ptr<v8::Task> task,
                            double delay_in_seconds) const;

  void PostInspectorTask(std::unique_ptr<v8::Task> task) const;

  ContextId CreateContext();
  void ClearContext(v8::Context* address);
  v8::Local<v8::Context> GetContext(ContextId address);

  void CreateUnboundScript(v8::Local<v8::UnboundScript> unbound_script);
  v8::Local<v8::UnboundScript> GetUnboundScript(ScriptId address);

 private:
  IsolateParams isolate_params_;

  v8::Isolate* isolate_par_{nullptr};
  v8::Isolate* isolate_sel_{nullptr};

  std::unique_ptr<Scheduler> scheduler_sel_;
  std::unique_ptr<Scheduler> scheduler_par_;

  std::unique_ptr<PerIsolateData> per_isolate_data_;

  std::map<ContextId const, RemoteHandle<v8::Context>> context_map_;
  std::map<ScriptId const, RemoteHandle<v8::UnboundScript>> unbound_script_map_;
};

}  // namespace svm
