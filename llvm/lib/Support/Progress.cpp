//===-- Progress - Generic infrastructure to report progress ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the core logic for progress reporting.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Progress.h"

using namespace llvm;

ManagedStatic<Progress> llvm::ProgressReport;

static thread_local TaskStack CurrentStack(*ProgressReport);

Progress::Progress() : MainThreadID(get_threadid()) {}

bool Progress::isMainThread() const { return MainThreadID == get_threadid(); }

void Progress::handleNewTask(const Task *T) {
  bool IsMainThread = isMainThread();
  for (auto &[AllThreads, Listener] : Registry)
    if (IsMainThread or AllThreads)
      Listener->handleNewTask(T);
}

void Progress::handleTaskCompleted(const Task *T) {
  bool IsMainThread = isMainThread();
  for (auto &[AllThreads, Listener] : Registry)
    if (IsMainThread or AllThreads)
      Listener->handleTaskCompleted(T);
}

void Progress::handleTaskAdvancement(const Task *T,
                                     StringRef PreviousStepName) {
  bool IsMainThread = isMainThread();
  for (auto &[AllThreads, Listener] : Registry)
    if (IsMainThread or AllThreads)
      Listener->handleTaskAdvancement(T, PreviousStepName);
}

Task::Task(std::optional<int64_t> TotalSteps, const Twine &TaskName)
    : Stack(CurrentStack), TotalSteps(TotalSteps), TaskName(TaskName.str()) {
  Stack.registerTask(this);
}

void Task::complete() {
  if (not Completed) {
    Completed = true;
    Stack.unregisterTask(this);
  }
}

Task::~Task() { complete(); }

void Task::advance(const Twine &NewStepName, bool SingleSubtask) {
  int64_t NewStepIndex = StepIndex + 1;
  if (TotalSteps.has_value()) {
    assert(NewStepIndex < *TotalSteps);
  }
  StepIndex = NewStepIndex;
  std::string PreviousStepName = std::move(StepName);
  StepName = NewStepName.str();
  SubtaskCreated = false;
  CurrentStepHasSingleSubtask = SingleSubtask;

  Stack.advanceTask(this, PreviousStepName);
}

void Task::setSubtaskCreated() { SubtaskCreated = true; }

size_t Task::index() const {
  auto &Stack = stack();
  auto Begin = Stack.Tasks.begin();
  auto End = Stack.Tasks.end();
  auto It = std::find(Begin, End, this);
  assert(It != End);
  return It - Begin;
}

void TaskStack::registerTask(Task *T) {
  if (isSuspended())
    return;

  if (Tasks.size() > 0) {
    if (T->currentStepHasSingleSubtask()) {
      assert(not Tasks.back()->subtaskCreated());
    }
    Tasks.back()->setSubtaskCreated();
  }

  Tasks.push_back(T);

  Parent.handleNewTask(T);
}

void TaskStack::unregisterTask(Task *T) {
  if (isSuspended())
    return;

  assert(Tasks.back() == T);
  Parent.handleTaskCompleted(T);
  Tasks.pop_back();
}

void TaskStack::advanceTask(Task *T, StringRef PreviousStepName) {
  if (isSuspended())
    return;

  Parent.handleTaskAdvancement(T, PreviousStepName);
}
