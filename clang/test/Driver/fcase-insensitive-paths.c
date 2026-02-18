// RUN: %clang -### -fcase-insensitive-paths -c %s 2>&1 | FileCheck %s

// CHECK: "-fcase-insensitive-paths"
