//===-- CommandLine.cpp - Command line parser implementation --------------===//
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

#include "llvm/Support/DebugInfoPreservation.h"

static constexpr llvm::StringRef Description = "Enable stricter debug "
                                               "information preservation. In "
                                               "particular avoid dropping it "
                                               "even if it means conventional "
                                               "debugger (gdb, lldb, etc) "
                                               "experience degrades.";
llvm::cl::opt<bool> llvm::EnableStrictDebugInformationPreservationStyle(
    "enable-strict-debug-information-preservation-style",
    llvm::cl::desc(Description));
