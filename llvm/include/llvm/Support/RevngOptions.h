#ifndef LLVM_SUPPORT_REVNGOPTIONS_H
#define LLVM_SUPPORT_REVNGOPTIONS_H

#include "llvm/Support/CommandLine.h"

namespace llvm::revng {

extern bool OverrideRevngDebugInformationPreservationStyle;
extern cl::opt<bool, true> RevngDebugInformationPreservationStyle;

} // namespace llvm::revng

#endif
