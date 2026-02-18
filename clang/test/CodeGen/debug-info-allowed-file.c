// Verify that -fdebug-info-allowed-file restricts rich debug-info emission to
// declarations from the listed files.

// RUN: rm -rf %t && mkdir -p %t
// RUN: echo 'struct allowed_struct {}; void allowed_fn(int);' > %t/allowed.h
// RUN: echo 'struct excluded_struct {}; void excluded_fn(int);' > %t/excluded.h

// With the filter, only decls from allowed.h appear in the debug output.
// RUN: %clang_cc1 -debug-info-kind=unused-types -emit-llvm -o - %s -I %t \
// RUN:   -fdebug-info-allowed-file=%t/allowed.h \
// RUN:   | FileCheck %s --check-prefix=FILTERED

// Without the filter, both decls are emitted.
// RUN: %clang_cc1 -debug-info-kind=unused-types -emit-llvm -o - %s -I %t \
// RUN:   | FileCheck %s --check-prefix=UNFILTERED

#include "allowed.h"
#include "excluded.h"

// FILTERED-DAG: name: "allowed_struct"
// FILTERED-DAG: name: "allowed_fn"
// FILTERED-NOT: name: "excluded_struct"
// FILTERED-NOT: name: "excluded_fn"

// UNFILTERED-DAG: name: "allowed_struct"
// UNFILTERED-DAG: name: "excluded_struct"
