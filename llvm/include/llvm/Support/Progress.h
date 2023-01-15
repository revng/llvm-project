//===-- Progress - Generic infrastructure to report progress ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_PROGRESS_H
#define LLVM_SUPPORT_PROGRESS_H

extern "C" {
#include <unistd.h>
}

#include <algorithm>
#include <chrono>
#include <optional>
#include <set>
#include <vector>

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Threading.h"

namespace llvm {

class Progress;
class TaskStack;

/// Track advancement of a task
///
/// A task can have a name and optionally a set of maximum steps.
class Task {
protected:
  TaskStack &Stack;
  int64_t StepIndex = -1;
  std::optional<int64_t> TotalSteps;

private:
  std::string TaskName;
  std::string StepName;
  bool SubtaskCreated = false;
  bool Completed = false;
  bool CurrentStepHasSingleSubtask = false;

public:
  Task(std::optional<int64_t> TotalSteps, const llvm::Twine &TaskName = "");

  Task(const Task &) = delete;
  Task(Task &&) = delete;

  ~Task();

public:
  llvm::StringRef name() const { return TaskName; }
  llvm::StringRef stepName() const { return StepName; }
  /// \return -1 if no step (i.e., call to advance) has been initiated,
  ///         the index of the last initiated step otherwise.
  int64_t stepIndex() const { return StepIndex; }
  std::optional<int64_t> totalSteps() const { return TotalSteps; }
  bool subtaskCreated() const { return SubtaskCreated; }
  bool currentStepHasSingleSubtask() const {
    return CurrentStepHasSingleSubtask;
  }
  bool completed() const { return Completed; }
  size_t index() const;

  TaskStack &stack() { return Stack; }
  const TaskStack &stack() const { return Stack; }

public:
  /// \param NewStepName the name of the new step starting after this advance
  ///        invocation.
  /// \param SingleSubtask whether we can expect 0 or 1 subtask being created
  ///        during this step. If true, will assert in case multiple subtasks
  ///        are create during this step.
  void advance(const llvm::Twine &NewStepName = "", bool SingleSubtask = false);

  void setSubtaskCreated();

  void complete();
};

/// Represents the set of tasks started but not completed in an execution thread
class TaskStack {
private:
  Progress &Parent;
  unsigned SuspendRequests = 0;

public:
  llvm::SmallVector<Task *, 4> Tasks;

public:
  TaskStack(Progress &Parent) : Parent(Parent) {}

  TaskStack(TaskStack &&) = delete;
  TaskStack(const TaskStack &) = delete;
  TaskStack &operator=(const TaskStack &) = delete;
  TaskStack &operator=(TaskStack &&) = delete;

public:
  void registerTask(Task *T);
  void unregisterTask(Task *T);
  void advanceTask(Task *T, llvm::StringRef PreviousStepName);

public:
  bool isSuspended() const { return SuspendRequests > 0; }
  void resumeTracking() {
    assert(isSuspended());
    --SuspendRequests;
  }

  void suspendTracking() { ++SuspendRequests; }
};

/// Class to monitor progress of a Progress instance
class ProgressListener {
public:
  virtual ~ProgressListener() = default;

  /// Invoked to notify the creation of a new task
  ///
  /// \note \p T will always be the last task of its stack.
  ///
  /// \note This method could be called by multiple threads in parallel.
  virtual void handleNewTask(const Task *T) = 0;

  /// Invoked to notify the completion of a task
  ///
  /// \note \p T will always be the last task of its stack.
  ///
  /// \note This method could be called by multiple threads in parallel.
  virtual void handleTaskCompleted(const Task *T) = 0;

  /// Invoked to notify of the advancement of a certain task to a new step
  ///
  /// \note \p T will always be the last task of its stack.
  ///
  /// \note This method could be called by multiple threads in parallel.
  virtual void handleTaskAdvancement(const Task *T,
                                     llvm::StringRef PreviousStepName) = 0;
};

/// Class to monitor progress of task staks over multiple threads
class Progress {
private:
  struct RegistryEntry {
    bool AllThreads = false;
    std::unique_ptr<ProgressListener> Listener;
  };

private:
  uint64_t MainThreadID;
  llvm::SmallVector<RegistryEntry, 2> Registry;

public:
  Progress();

public:
  /// \note call this method from the main thread only.
  template <typename T, typename... Types>
  void registerListener(Types &&...Args) {
    assert(isMainThread());
    Registry.push_back({T::AllThreads, std::make_unique<T>(Args...)});
  }

public:
  bool isMainThread() const;

public:
  void handleNewTask(const Task *T);
  void handleTaskCompleted(const Task *T);
  void handleTaskAdvancement(const Task *T, llvm::StringRef PreviousStepName);
};

/// The default instance of Progress each Task will use
extern llvm::ManagedStatic<Progress> ProgressReport;

/// Similar to Task, but gets a list of objects at construction times that
/// represent elements the task iterates on.
/// Each invocation of advance accepts a similar object: if the passed object is
/// not in the list of expected objects, the monitor of progress will be
/// suspended until the next step.
template <typename ValueType,
          typename Set = std::conditional_t<std::is_pointer_v<ValueType>,
                                            llvm::SmallPtrSet<ValueType, 16>,
                                            std::set<ValueType>>>
class TaskOnSet : public Task {
private:
  Set Predicted;
  bool Suspended = false;

public:
  template <typename R>
  TaskOnSet(R &&Elements, const llvm::Twine &TaskName = "")
      : Task({}, TaskName) {
    unsigned Count = 0;
    for (const auto &Element : Elements) {
      Predicted.insert(Element);
      ++Count;
    }

    TotalSteps = Predicted.size();

    assert(TotalSteps == Count);
  }

  ~TaskOnSet() {
    if (Suspended)
      Stack.resumeTracking();
  }

public:
  void advance(const ValueType &Element, const llvm::Twine &NewStepName = "",
               bool SingleSubtask = false) {

    if (Predicted.count(Element) != 0) {
      if (Suspended) {
        Stack.resumeTracking();
        Suspended = false;
      }

      // This is an expected element, proceed
      Task::advance(NewStepName, SingleSubtask);

    } else {
      if (not Suspended) {
        // Ignore all unexpected elements
        Stack.suspendTracking();
        Suspended = true;
      }
    }
  }
};

template <typename R>
auto make_task_on_set(R &&Elements, const llvm::Twine &TaskName = "") {
  return TaskOnSet<std::decay_t<decltype(*std::declval<R>().begin())>>(
      Elements, TaskName);
}

template <typename R>
auto make_task_on_set(TaskStack &Stack, R &&Elements,
                      const llvm::Twine &TaskName = "") {
  return TaskOnSet<std::decay_t<decltype(*std::declval<R>().begin())>>(
      Stack, Elements, TaskName);
}

} // namespace llvm

#endif
