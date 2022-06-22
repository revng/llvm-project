//===-- PerfHelperTest.cpp --------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PerfHelper.h"
#include "llvm/Config/config.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace llvm {
namespace exegesis {
namespace pfm {
namespace {

using ::testing::IsEmpty;
using ::testing::Not;

TEST(PerfHelperTest, FunctionalTest) {
  ASSERT_TRUE(true);
}

} // namespace
} // namespace pfm
} // namespace exegesis
} // namespace llvm
