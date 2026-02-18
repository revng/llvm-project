// Verify that -fcase-insensitive-paths retries failed header lookups with a
// case-insensitive directory scan.  Only meaningful on a case-sensitive FS.

// UNSUPPORTED: case-insensitive-filesystem

// RUN: rm -rf %t && mkdir -p %t/inc
// RUN: echo 'int from_header;' > %t/inc/Header.h

// Without the flag, the case-mismatched include fails.
// RUN: not %clang_cc1 -fsyntax-only -I %t/inc %s 2>&1 | FileCheck --check-prefix=OFF %s
// OFF: 'header.h' file not found

// With the flag, the lookup succeeds.
// RUN: %clang_cc1 -fsyntax-only -fcase-insensitive-paths -I %t/inc %s

#include "header.h"
int use_it(void) { return from_header; }
