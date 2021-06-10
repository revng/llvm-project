; RUN: opt < %s -sroa --sroa-aggressive-phis-selects -dce -S | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-n8:16:32:64"

declare i32 @initx()

declare i32 @inity()

declare i32 @getcondition()

declare void @leak(i32*)

define i32 @selectphichain() {
; CHECK-LABEL: @selectphichain(
  %1 = alloca i32, align 16
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
  call void @leak(i32 *%1)
; CHECK: call void @leak(i32* %1)
  %3 = call i32 @initx()
; CHECK: %3 = call i32 @initx()
  store i32 %3, i32* %1, align 16
; CHECK: store i32 %3, i32* %1, align 16
  %4 = call i32 @inity()
; CHECK: %4 = call i32 @inity()
  call void @leak(i32 *%2)
; CHECK: call void @leak(i32* %2)
  store i32 %4, i32* %2, align 16
; CHECK: store i32 %4, i32* %2, align 16
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
