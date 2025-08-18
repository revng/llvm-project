; RUN: opt -disable-output -passes="print<demanded-bits>" < %s 2>&1 | FileCheck %s

; CHECK-DAG: DemandedBits: 0x7fff for %x in   %right = lshr i64 %x, %amount_right
define i8 @right(i64 %x, i64 %count) {
  %bounded_right = and i64 %count, 7
  %amount_right = or i64 %bounded_right, 0
  %right = lshr i64 %x, %amount_right
  %low = trunc i64 %right to i8
  ret i8 %low
}

; CHECK-DAG: DemandedBits: 0xff for %x in   %left = shl i64 %x, %amount_left
define i8 @left(i64 %x, i64 %count) {
  %bounded_left = and i64 %count, 7
  %amount_left = or i64 %bounded_left, 0
  %left = shl i64 %x, %amount_left
  %low = trunc i64 %left to i8
  ret i8 %low
}

; CHECK-DAG: DemandedBits: 0x7fffff00 for %x in   %ranged = lshr i64 %x, %amount_ranged
define i16 @ranged(i64 %x, i64 %count) {
  %bounded_ranged = and i64 %count, 7
  %amount_ranged = or i64 %bounded_ranged, 8
  %ranged = lshr i64 %x, %amount_ranged
  %low = trunc i64 %ranged to i16
  ret i16 %low
}

; CHECK-DAG: DemandedBits: 0x7fff for %x in   %exact = lshr exact i64 %x, %amount_exact
define i8 @exact(i64 %x, i64 %count) {
  %bounded_exact = and i64 %count, 3
  %amount_exact = or i64 %bounded_exact, 4
  %exact = lshr exact i64 %x, %amount_exact
  %low = trunc i64 %exact to i8
  ret i8 %low
}

; CHECK-DAG: DemandedBits: 0xff000000000000ff for %x in   %signed_wrap = shl nsw i64 %x, %amount_signed_wrap
define i8 @signed_wrap(i64 %x, i64 %count) {
  %bounded_signed_wrap = and i64 %count, 7
  %amount_signed_wrap = or i64 %bounded_signed_wrap, 0
  %signed_wrap = shl nsw i64 %x, %amount_signed_wrap
  %low = trunc i64 %signed_wrap to i8
  ret i8 %low
}

; CHECK-DAG: DemandedBits: 0xfe000000000000ff for %x in   %unsigned_wrap = shl nuw i64 %x, %amount_unsigned_wrap
define i8 @unsigned_wrap(i64 %x, i64 %count) {
  %bounded_unsigned_wrap = and i64 %count, 7
  %amount_unsigned_wrap = or i64 %bounded_unsigned_wrap, 0
  %unsigned_wrap = shl nuw i64 %x, %amount_unsigned_wrap
  %low = trunc i64 %unsigned_wrap to i8
  ret i8 %low
}

; CHECK-DAG: DemandedBits: 0xf000000000000000 for %x in   %arithmetic = ashr i64 %x, %amount_arithmetic
define i8 @arithmetic(i64 %x, i64 %count) {
  %bounded_arithmetic = and i64 %count, 3
  %amount_arithmetic = or i64 %bounded_arithmetic, 60
  %arithmetic = ashr i64 %x, %amount_arithmetic
  %low = trunc i64 %arithmetic to i8
  ret i8 %low
}

; CHECK-DAG: DemandedBits: 0xffffffffffffffff for %x in   %unknown = lshr i64 %x, %count
define i8 @unknown(i64 %x, i64 %count) {
  %unknown = lshr i64 %x, %count
  %low = trunc i64 %unknown to i8
  ret i8 %low
}
