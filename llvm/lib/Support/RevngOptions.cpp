//===- RevngOptions.cpp - Revng-specific CLI options --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/RevngOptions.h"
#include "llvm/ADT/StringRef.h"

namespace llvm::revng {

bool OverrideRevngDebugInformationPreservationStyle;

static constexpr llvm::StringRef Description = "Enable stricter debug "
                                               "information preservation. In "
                                               "particular avoid dropping it "
                                               "even if it means conventional "
                                               "debugger (gdb, lldb, etc) "
                                               "experience degrades.";
cl::opt<bool, true> RevngDebugInformationPreservationStyle(
    "use-revng-debug-information-preservation-style", cl::desc(Description),
    cl::location(OverrideRevngDebugInformationPreservationStyle));

} // namespace llvm::revng
