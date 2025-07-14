//
// Created by ruoshui on 25-7-3.
//

#pragma once
#include <v8-isolate.h>

#include "../utils/utils.h"
#include "per_isolate_data.h"
#include "scheduler.h"

namespace svm {

class IsolateHolder {
 public:
  explicit IsolateHolder(v8::Isolate* isolate_parent,
                         size_t memory_limit = 128);
  ~IsolateHolder();

  v8::Isolate* GetIsolateSel() const { return isolate_sel_; }
  v8::Isolate* GetIsolatePar() const { return isolate_par_; }

  Scheduler* GetSchedulerSel() const { return scheduler_sel_.get(); }
  Scheduler* GetSchedulerPar() const { return scheduler_par_.get(); }

  void PostTaskToSel(std::unique_ptr<v8::Task> task);
  void PostTaskToPar(std::unique_ptr<v8::Task> task);
  void PostDelayedTaskToSel(std::unique_ptr<v8::Task> task,
                            double delay_in_seconds);
  void PostDelayedTaskToPar(std::unique_ptr<v8::Task> task,
                            double delay_in_seconds);

  void PostInspectorTask(std::unique_ptr<v8::Task> task);

  uint32_t NewContext();
  void ClearContext(uint32_t id);
  v8::Local<v8::Context> GetContext(uint32_t id);

 private:
  v8::Isolate* isolate_par_{nullptr};
  v8::Isolate* isolate_sel_{nullptr};

  std::unique_ptr<Scheduler> scheduler_sel_;
  std::unique_ptr<Scheduler> scheduler_par_;

  uint32_t index_{0};
  std::map<uint32_t, RemoteHandle<v8::Context>> context_map_;

  std::unique_ptr<PerIsolateData> per_isolate_data_;

  size_t memory_limit = 128;
};

}  // namespace svm
