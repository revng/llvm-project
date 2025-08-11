//===- llvm/Support/CommandLine.h - Command line handler --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides an extra command line option that changes debug
// information preservation logic accross multiple transformation utilities.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_DEBUGINFOPRESERVATION_H
#define LLVM_SUPPORT_DEBUGINFOPRESERVATION_H

#include "llvm/Support/CommandLine.h"

namespace llvm {

// This enables stricter debug information preservation.
// In particular, when this is set, debug information should never be dropped,
// even if it leads to worse debugger (gdb, lldb, etc) experience.
extern llvm::cl::opt<bool> EnableStrictDebugInformationPreservationStyle;

} // end namespace llvm

#endif // LLVM_SUPPORT_DEBUGINFOPRESERVATION_H
