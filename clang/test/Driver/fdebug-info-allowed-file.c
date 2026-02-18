// RUN: %clang -### -fdebug-info-allowed-file=a.h -fdebug-info-allowed-file=b.h -c %s 2>&1 | FileCheck %s

// CHECK: "-fdebug-info-allowed-file=a.h" "-fdebug-info-allowed-file=b.h"
