//===--- FileSystemOptions.h - File System Options --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Defines the clang::FileSystemOptions interface.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_FILESYSTEMOPTIONS_H
#define LLVM_CLANG_BASIC_FILESYSTEMOPTIONS_H

#include <string>

namespace clang {

/// Keeps track of options that affect how file operations are performed.
class FileSystemOptions {
public:
  /// If set, paths are resolved as if the working directory was
  /// set to the value of WorkingDir.
  std::string WorkingDir;

  /// If set, file lookups that fail are retried with a case-insensitive
  /// directory scan.  Useful for compiling against SDKs whose header paths
  /// use a different case than the #include directives.
  bool CaseInsensitivePaths = false;
};

} // end namespace clang

#endif
