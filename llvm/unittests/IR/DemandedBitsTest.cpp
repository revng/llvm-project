//===- DemandedBitsTest.cpp - DemandedBits tests --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/DemandedBits.h"
#include "../Support/KnownBitsTest.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

template <typename Fn1, typename Fn2>
static void TestBinOpExhaustive(Fn1 PropagateFn, Fn2 EvalFn) {
  unsigned Bits = 4;
  unsigned Max = 1 << Bits;
  ForeachKnownBits(Bits, [&](const KnownBits &Known1) {
    ForeachKnownBits(Bits, [&](const KnownBits &Known2) {
      for (unsigned AOut_ = 0; AOut_ < Max; AOut_++) {
        APInt AOut(Bits, AOut_);
        APInt AB1 = PropagateFn(0, AOut, Known1, Known2);
        APInt AB2 = PropagateFn(1, AOut, Known1, Known2);
        {
          // If the propagator claims that certain known bits
          // didn't matter, check it doesn't change its mind
          // when they become unknown.
          KnownBits Known1Redacted;
          KnownBits Known2Redacted;
          Known1Redacted.Zero = Known1.Zero & AB1;
          Known1Redacted.One = Known1.One & AB1;
          Known2Redacted.Zero = Known2.Zero & AB2;
          Known2Redacted.One = Known2.One & AB2;

          APInt AB1R = PropagateFn(0, AOut, Known1Redacted, Known2Redacted);
          APInt AB2R = PropagateFn(1, AOut, Known1Redacted, Known2Redacted);
          EXPECT_EQ(AB1, AB1R);
          EXPECT_EQ(AB2, AB2R);
        }
        ForeachNumInKnownBits(Known1, [&](APInt Value1) {
          ForeachNumInKnownBits(Known2, [&](APInt Value2) {
            APInt ReferenceResult = EvalFn((Value1 & AB1), (Value2 & AB2));
            APInt Result = EvalFn(Value1, Value2);
            EXPECT_EQ(Result & AOut, ReferenceResult & AOut);
          });
        });
      }
    });
  });
}

TEST(DemandedBitsTest, Add) {
  TestBinOpExhaustive(DemandedBits::determineLiveOperandBitsAdd,
                      [](APInt N1, APInt N2) -> APInt { return N1 + N2; });
}

TEST(DemandedBitsTest, Sub) {
  TestBinOpExhaustive(DemandedBits::determineLiveOperandBitsSub,
                      [](APInt N1, APInt N2) -> APInt { return N1 - N2; });
}

class DemandedBitsRangeTest : public testing::Test {
protected:
  LLVMContext Context;
  std::unique_ptr<Module> M;
  Function *F = nullptr;
  std::unique_ptr<AssumptionCache> AC;
  DominatorTree DT;

  void parseAssembly(StringRef IR) {
    SMDiagnostic Error;
    M = parseAssemblyString(IR, Error, Context);
    std::string Message;
    raw_string_ostream Stream(Message);
    Error.print("DemandedBitsRangeTest", Stream);
    ASSERT_TRUE(M) << Stream.str();
    F = M->getFunction("f");
    ASSERT_TRUE(F);
    AC = std::make_unique<AssumptionCache>(*F);
    DT.recalculate(*F);
  }

  Instruction *instruction(StringRef Name) {
    for (Instruction &I : instructions(*F))
      if (I.getName() == Name)
        return &I;
    llvm_unreachable("Missing test instruction");
  }

  Use *operand(StringRef Name, unsigned Index) {
    return &instruction(Name)->getOperandUse(Index);
  }

  DemandedBits analyze(DemandedBits::RangeQuery GetRange = {}) {
    return DemandedBits(*F, *AC, DT, std::move(GetRange));
  }

  // Supply a range only for the selected operand occurrence.
  static DemandedBits::RangeQuery rangeFor(Use *Operand, ConstantRange Range) {
    return [Operand, Range](const Use &U) {
      return &U == Operand
                 ? Range
                 : ConstantRange::getFull(U->getType()->getScalarSizeInBits());
    };
  }
};

TEST_F(DemandedBitsRangeTest, RangesAndCachingArePerUse) {
  ASSERT_NO_FATAL_FAILURE(parseAssembly(R"(
    define i8 @f(i64 %x, i64 %y, i64 %amount) {
      %bounded = lshr i64 %x, %amount
      %unbounded = lshr i64 %y, %amount
      %left = trunc i64 %bounded to i8
      %right = trunc i64 %unbounded to i8
      %sum = add i8 %left, %right
      ret i8 %sum
    }
  )"));
  auto Range = ConstantRange(APInt(64, 1), APInt(64, 6));
  auto GetRange = rangeFor(operand("bounded", 1), Range);
  unsigned Queries = 0;
  auto DB = analyze([&](const Use &U) {
    ++Queries;
    return GetRange(U);
  });
  // Amounts 1..5 demand bits 1..12; known bits alone only prove amounts 0..7.
  APInt Expected = APInt::getBitsSet(64, 1, 13);
  EXPECT_EQ(DB.getDemandedBits(operand("bounded", 0)), Expected);
  // The other use of the same amount has no range information.
  EXPECT_EQ(DB.getDemandedBits(operand("unbounded", 0)), APInt::getAllOnes(64));
  EXPECT_EQ(Queries, 2U);
  EXPECT_EQ(DB.getDemandedBits(operand("bounded", 0)), Expected);
  EXPECT_EQ(Queries, 2U);
}

TEST_F(DemandedBitsRangeTest, CombinesKnownBitsAndRanges) {
  ASSERT_NO_FATAL_FAILURE(parseAssembly(R"(
    define i8 @f(i8 %x, i8 %input) {
      %value = add i8 %x, 1
      %mask = and i8 %input, 253
      %masked = and i8 %value, %mask
      ret i8 %masked
    }
  )"));
  auto Range = ConstantRange(APInt(8, 0), APInt(8, 128));
  auto DB = analyze(rangeFor(operand("masked", 1), Range));
  // GetRange clears bit 7; ValueTracking clears bit 1. Both must be preserved.
  EXPECT_EQ(DB.getDemandedBits(instruction("value")), APInt(8, 0x7d));
}

TEST_F(DemandedBitsRangeTest, Fallbacks) {
  ASSERT_NO_FATAL_FAILURE(parseAssembly(R"(
    define i8 @f(i64 %x, i64 %amount) {
      %bit = and i64 %amount, 1
      %bounded = or i64 %bit, 4
      %shift = lshr i64 %x, %bounded
      %low = trunc i64 %shift to i8
      ret i8 %low
    }
  )"));
  // Without GetRange, ValueTracking bounds the amount to [4, 6).
  APInt Expected = APInt::getBitsSet(64, 4, 13);
  EXPECT_EQ(analyze().getDemandedBits(operand("shift", 0)), Expected);
  for (ConstantRange Range :
       {ConstantRange::getFull(64), ConstantRange::getEmpty(64),
        ConstantRange(APInt(64, 0), APInt(64, 65)),
        ConstantRange(APInt(64, 8), APInt(64, 16)),
        ConstantRange(APInt(64, 6), APInt(64, 9))}) {
    // The last two ranges contradict known bits or have an empty intersection
    // without conflicting bits. Neither should discard ValueTracking's bounds.
    SCOPED_TRACE(Range);
    auto DB = analyze(rangeFor(operand("shift", 1), Range));
    EXPECT_EQ(DB.getDemandedBits(operand("shift", 0)), Expected);
  }
}

TEST_F(DemandedBitsRangeTest, SkipsUndefAndPoison) {
  ASSERT_NO_FATAL_FAILURE(parseAssembly(R"(
    define i8 @f(i64 %x, i64 %amount) {
      %shift = lshr i64 %x, %amount
      %low = trunc i64 %shift to i8
      ret i8 %low
    }
  )"));
  Use *Amount = operand("shift", 1);
  Value *Unknowns[] = {UndefValue::get(Amount->get()->getType()),
                       PoisonValue::get(Amount->get()->getType())};
  for (Value *Unknown : Unknowns) {
    SCOPED_TRACE(isa<PoisonValue>(Unknown) ? "poison" : "undef");
    Amount->set(Unknown);
    auto DB = analyze([](const Use &U) {
      ADD_FAILURE() << "Must not query an undef or poison operand";
      return ConstantRange::getFull(U->getType()->getIntegerBitWidth());
    });
    EXPECT_EQ(DB.getDemandedBits(operand("shift", 0)), APInt::getAllOnes(64));
  }
}

TEST_F(DemandedBitsRangeTest, SkipsVectors) {
  ASSERT_NO_FATAL_FAILURE(parseAssembly(R"(
    define <2 x i8> @f(<2 x i64> %x, <2 x i64> %amount) {
      %shift = lshr <2 x i64> %x, %amount
      %low = trunc <2 x i64> %shift to <2 x i8>
      ret <2 x i8> %low
    }
  )"));
  auto DB = analyze([](const Use &U) {
    ADD_FAILURE() << "Must not query a vector operand";
    return ConstantRange::getFull(U->getType()->getScalarSizeInBits());
  });
  EXPECT_EQ(DB.getDemandedBits(operand("shift", 0)), APInt::getAllOnes(64));
}

} // anonymous namespace
