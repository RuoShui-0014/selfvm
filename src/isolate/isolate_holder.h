//
// Created by ruoshui on 25-7-3.
//

#pragma once
#include <v8-isolate.h>

#include <unordered_map>

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
  Scheduler* GetSchedulerPar() const { return scheduler_par_; }

  void PostTaskToSel(std::unique_ptr<v8::Task> task) const;
  void PostHandleTaskToSel(std::unique_ptr<v8::Task> task) const;
  void PostDelayedTaskToSel(std::unique_ptr<v8::Task> task,
                            double delay_in_seconds) const;

  void PostTaskToPar(std::unique_ptr<v8::Task> task) const;
  void PostHandleTaskToPar(std::unique_ptr<v8::Task> task) const;
  void PostDelayedTaskToPar(std::unique_ptr<v8::Task> task,
                            double delay_in_seconds) const;

  void PostInterruptTask(std::unique_ptr<v8::Task> task) const;

  ContextId CreateContext();
  v8::Local<v8::Context> GetContext(ContextId address);
  void ClearContext(ContextId address);

  void CreateScript(v8::Local<v8::UnboundScript> unbound_script);
  v8::Local<v8::UnboundScript> GetScript(ScriptId address);
  void ClearScript(ScriptId address);

 private:
  IsolateParams isolate_params_;

  v8::Isolate* isolate_par_{};
  v8::Isolate* isolate_sel_{};

  std::unique_ptr<UVSchedulerSel> scheduler_sel_;
  Scheduler* scheduler_par_{};

  std::unique_ptr<PerIsolateData> per_isolate_data_;

  std::mutex mutex_context_map_;
  std::mutex mutex_unbound_script_map_;
  std::unordered_map<ContextId, RemoteHandle<v8::Context>> context_map_;
  std::unordered_map<ScriptId, RemoteHandle<v8::UnboundScript>>
      unbound_script_map_;
};

}  // namespace svm
