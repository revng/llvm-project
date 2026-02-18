// Verify that a declaration-only function gets its DISubprogram retained
// and lowered to an LF_FUNC_ID record in the CodeView output.

// REQUIRES: x86-registered-target

// RUN: %clang_cc1 -triple x86_64-windows-msvc -gcodeview \
// RUN:   -debug-info-kind=unused-types -emit-obj -o %t.o %s
// RUN: llvm-readobj %t.o --codeview | FileCheck %s

void declared_fn(int);

// CHECK:      FuncId ({{.*}}) {
// CHECK-NEXT:   TypeLeafKind: LF_FUNC_ID
// CHECK:        FunctionType: void (int)
// CHECK-NEXT:   Name: declared_fn
// CHECK-NEXT: }
