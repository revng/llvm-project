; RUN: opt < %s -sroa --sroa-aggressive-phis-selects -dce -S | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-n8:16:32:64"

; This tests have been produced with llvm-12 from the following C code with the
; commands:
;   clang -O0 file.c -S -emit-llvm -o file-O0.ll -Xclang -disable-O0-optnone
;   opt file-O0.ll -S -o test.ll -instcombine -simplifycfg -dce -dse -sroa -sroa -sroa
;
; After this, all debug info and all metadata have been removed.
;
; The three sroa invocations at the end are meant to rule out everything that
; regular old sroa is already able to optimize away. What's left should only be
; optimizeable by sroa with the new --sroa-aggressive-phis-selects.
;
; The selectphichain test has also been included in phi-and-select.ll, to make
; sure that regular sroa is not able to optimize it.
;
; If the regular sroa ever becomes powerful enough to handle the cases that
; previously required the aggressive version, the aggressive version can likely
; be dropped alltogether.
;
;
; #include <alloca.h>
; #include <stdio.h>
;
; extern int initx(void);
; extern int inity(void);
; extern void leak(int* leaked);
; extern int getcondition(void);
;
; int selectphichain()
; {
;   int* x = alloca(sizeof(int));
;   int* y = alloca(sizeof(int));
;   *x = initx();
;   *y = inity();
;   int* p = NULL;
;   if (getcondition() > 3) {
;     *x = 100;
;     if (getcondition() > 8) {
;       p = x;
;     } else {
;       p = y;
;     }
;     *x = 200;
;   } else {
;     p = x;
;   }
;   return *p;
; }
;
; int leaking()
; {
;   int* x = alloca(sizeof(int));
;   int* y = alloca(sizeof(int));
;   *x = initx();
;   leak(x);
;   *y = inity();
;   leak(y);
;   int* p = NULL;
;   if (getcondition() > 3) {
;     *x = 100;
;     if (getcondition() > 8) {
;       p = x;
;     } else {
;       p = y;
;     }
;     *x = 200;
;   } else {
;     p = x;
;   }
;   return *p;
; }

declare i32 @initx()

declare i32 @inity()

declare i32 @getcondition()

declare void @leak(i32*)

define i32 @selectphichain() {
; CHECK-LABEL: @selectphichain(
  %1 = alloca i32, align 16
; The first alloca could be optimized away in principle, but that would require
; running sroa in fixed-point fashion. This would require major intrusive
; changes to the SROA pass, that we want to avoid at the moment.
; Another option would be to execute the sroa pass with
; --sroa-aggressive-phis-selects twice, but at the moment we are not doing this.
; The fact that one of the 2 allocas is optimized away with
; --sroa-aggressive-phis-selects is already enough to prove that do something
; strictly more aggressive than regular sroa. This is already enough for our
; purposes for now. If we ever find a way to make this more aggressive (or we
; end up needing it to be) we might want to redesign this test.
; CHECK: %1 = alloca i32, align 16
; CHECK-NOT: alloca
  %2 = alloca i32, align 16
  %3 = call i32 @initx()
  store i32 %3, i32* %1, align 16
; CHECK: %2 = call i32 @initx()
; CHECK: %3 = call i32 @inity()
  %4 = call i32 @inity()
  store i32 %4, i32* %2, align 16
; CHECK: store i32 %3, i32* %1, align 16
  %5 = call i32 @getcondition()
  %6 = icmp sgt i32 %5, 3
  br i1 %6, label %7, label %10

7:                                                ; preds = %0
  %8 = call i32 @getcondition()
  %9 = icmp sgt i32 %8, 8
  %s = select i1 %9, i32* %1, i32* %2
  br label %11

10:                                               ; preds = %0
  br label %11

11:                                               ; preds = %10, %7
  %thephi = phi i32* [ %s, %7 ], [ %1, %10 ]
  %12 = load i32, i32* %thephi, align 4
  ret i32 %12
; CHECK: ret i32 %thephi.sroa.speculated
}

define i32 @leaking() {
; CHECK-LABEL: @leaking(
  %1 = alloca i32, align 16
; CHECK: %1 = alloca i32, align 16
  %2 = alloca i32, align 16
; CHECK: %2 = alloca i32, align 16
  %3 = call i32 @initx()
; CHECK: %3 = call i32 @initx()
  store i32 %3, i32* %1, align 16
; CHECK: store i32 %3, i32* %1, align 16
  call void @leak(i32 *%1)
; CHECK: call void @leak(i32* %1)
  %4 = call i32 @inity()
; CHECK: %4 = call i32 @inity()
  store i32 %4, i32* %2, align 16
; CHECK: store i32 %4, i32* %2, align 16
  call void @leak(i32 *%2)
; CHECK: call void @leak(i32* %2)
  %5 = call i32 @getcondition()
  %6 = icmp sgt i32 %5, 3
  br i1 %6, label %7, label %10

7:                                                ; preds = %0
  %8 = call i32 @getcondition()
  %9 = icmp sgt i32 %8, 8
  %s = select i1 %9, i32* %1, i32* %2
; CHECK: %s = select i1 %9, i32* %1, i32* %2
  br label %11

10:                                               ; preds = %0
  br label %11

11:                                               ; preds = %10, %7
  %thephi = phi i32* [ %s, %7 ], [ %1, %10 ]
; CHECK: %thephi = phi i32* [ %s, %7 ], [ %1, %10 ]
  %12 = load i32, i32* %thephi, align 4
  ret i32 %12
; CHECK: ret i32 %12
}
