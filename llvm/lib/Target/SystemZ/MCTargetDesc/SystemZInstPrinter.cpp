//===- SystemZInstPrinter.cpp - Convert SystemZ MCInst to assembly syntax -===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SystemZInstPrinter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstdint>

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "SystemZGenAsmWriter.inc"

void SystemZInstPrinter::printAddress(unsigned Base, int64_t Disp,
                                      unsigned Index, raw_ostream &O,
                                      bool UseMarkup) {
  if (UseMarkup)
    O << "<imm:";
  O << Disp;
  if (UseMarkup)
    O << ">";
  if (Base || Index) {
    O << '(';
    if (Index) {
      if (UseMarkup)
        O << "<reg:";
      O << '%' << getRegisterName(Index);
      if (UseMarkup)
        O << ">";
      if (Base)
        O << ',';
    }
    if (Base) {
      if (UseMarkup)
        O << "<reg:";
      O << '%' << getRegisterName(Base);
      if (UseMarkup)
        O << ">";
    }
    O << ')';
  }
}

void SystemZInstPrinter::printOperand(const MCOperand &MO, const MCAsmInfo *MAI,
                                      raw_ostream &O, bool UseMarkup) {
  if (MO.isReg()) {
    if (!MO.getReg())
      O << '0';
    else {
      if (UseMarkup)
        O << "<reg:";
      O << '%' << getRegisterName(MO.getReg());
      if (UseMarkup)
        O << ">";
    }
  }
  else if (MO.isImm()) {
    if (UseMarkup)
      O << "<reg:";
    O << MO.getImm();
    if (UseMarkup)
      O << ">";
  } else if (MO.isExpr())
    MO.getExpr()->print(O, MAI);
  else
    llvm_unreachable("Invalid operand");
}

void SystemZInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                   StringRef Annot, const MCSubtargetInfo &STI,
                                   raw_ostream &O) {
  printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void SystemZInstPrinter::printRegName(raw_ostream &O, unsigned RegNo) const {
  O << markup("<reg:") << '%' << getRegisterName(RegNo) << markup(">");
}

template <unsigned N>
static void printUImmOperand(const MCInst *MI, int OpNum, raw_ostream &O,
                             bool UseMarkup) {
  int64_t Value = MI->getOperand(OpNum).getImm();
  assert(isUInt<N>(Value) && "Invalid uimm argument");
  if (UseMarkup)
    O << "<imm:";
  O << Value;
  if (UseMarkup)
    O << ">";
}

template <unsigned N>
static void printSImmOperand(const MCInst *MI, int OpNum, raw_ostream &O,
                             bool UseMarkup) {
  int64_t Value = MI->getOperand(OpNum).getImm();
  assert(isInt<N>(Value) && "Invalid simm argument");
  if (UseMarkup)
    O << "<imm:";
  O << Value;
  if (UseMarkup)
    O << ">";
}

void SystemZInstPrinter::printU1ImmOperand(const MCInst *MI, int OpNum,
                                           raw_ostream &O) {
  printUImmOperand<1>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printU2ImmOperand(const MCInst *MI, int OpNum,
                                           raw_ostream &O) {
  printUImmOperand<2>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printU3ImmOperand(const MCInst *MI, int OpNum,
                                           raw_ostream &O) {
  printUImmOperand<3>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printU4ImmOperand(const MCInst *MI, int OpNum,
                                           raw_ostream &O) {
  printUImmOperand<4>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printU6ImmOperand(const MCInst *MI, int OpNum,
                                           raw_ostream &O) {
  printUImmOperand<6>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printS8ImmOperand(const MCInst *MI, int OpNum,
                                           raw_ostream &O) {
  printSImmOperand<8>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printU8ImmOperand(const MCInst *MI, int OpNum,
                                           raw_ostream &O) {
  printUImmOperand<8>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printU12ImmOperand(const MCInst *MI, int OpNum,
                                            raw_ostream &O) {
  printUImmOperand<12>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printS16ImmOperand(const MCInst *MI, int OpNum,
                                            raw_ostream &O) {
  printSImmOperand<16>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printU16ImmOperand(const MCInst *MI, int OpNum,
                                            raw_ostream &O) {
  printUImmOperand<16>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printS32ImmOperand(const MCInst *MI, int OpNum,
                                            raw_ostream &O) {
  printSImmOperand<32>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printU32ImmOperand(const MCInst *MI, int OpNum,
                                            raw_ostream &O) {
  printUImmOperand<32>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printU48ImmOperand(const MCInst *MI, int OpNum,
                                            raw_ostream &O) {
  printUImmOperand<48>(MI, OpNum, O, UseMarkup);
}

void SystemZInstPrinter::printPCRelOperand(const MCInst *MI, int OpNum,
                                           raw_ostream &O) {
  const MCOperand &MO = MI->getOperand(OpNum);
  if (MO.isImm()) {
    O << markup("<imm:") << "0x";
    O.write_hex(MO.getImm());
    O << markup(">");
  } else
    MO.getExpr()->print(O, &MAI);
}

void SystemZInstPrinter::printPCRelTLSOperand(const MCInst *MI,
                                              uint64_t Address, int OpNum,
                                              raw_ostream &O) {
  // Output the PC-relative operand.
  printPCRelOperand(MI, OpNum, O);

  // Output the TLS marker if present.
  if ((unsigned)OpNum + 1 < MI->getNumOperands()) {
    const MCOperand &MO = MI->getOperand(OpNum + 1);
    const MCSymbolRefExpr &refExp = cast<MCSymbolRefExpr>(*MO.getExpr());
    switch (refExp.getKind()) {
      case MCSymbolRefExpr::VK_TLSGD:
        O << ":tls_gdcall:";
        break;
      case MCSymbolRefExpr::VK_TLSLDM:
        O << ":tls_ldcall:";
        break;
      default:
        llvm_unreachable("Unexpected symbol kind");
    }
    O << refExp.getSymbol().getName();
  }
}

void SystemZInstPrinter::printOperand(const MCInst *MI, int OpNum,
                                      raw_ostream &O) {
  printOperand(MI->getOperand(OpNum), &MAI, O, UseMarkup);
}

void SystemZInstPrinter::printBDAddrOperand(const MCInst *MI, int OpNum,
                                            raw_ostream &O) {
  printAddress(MI->getOperand(OpNum).getReg(),
               MI->getOperand(OpNum + 1).getImm(), 0, O, UseMarkup);
}

void SystemZInstPrinter::printBDXAddrOperand(const MCInst *MI, int OpNum,
                                             raw_ostream &O) {
  printAddress(MI->getOperand(OpNum).getReg(),
               MI->getOperand(OpNum + 1).getImm(),
               MI->getOperand(OpNum + 2).getReg(), O, UseMarkup);
}

void SystemZInstPrinter::printBDLAddrOperand(const MCInst *MI, int OpNum,
                                             raw_ostream &O) {
  unsigned Base = MI->getOperand(OpNum).getReg();
  uint64_t Disp = MI->getOperand(OpNum + 1).getImm();
  uint64_t Length = MI->getOperand(OpNum + 2).getImm();
  O << Disp << '(' << Length;
  if (Base)
    O << "," << markup("<reg:") << "%" << getRegisterName(Base) << markup(">");
  O << ')';
}

void SystemZInstPrinter::printBDRAddrOperand(const MCInst *MI, int OpNum,
                                             raw_ostream &O) {
  unsigned Base = MI->getOperand(OpNum).getReg();
  uint64_t Disp = MI->getOperand(OpNum + 1).getImm();
  unsigned Length = MI->getOperand(OpNum + 2).getReg();
  O << Disp << "(" << markup("<reg:") << "%" << getRegisterName(Length) 
    << markup(">");
  if (Base)
    O << "," << markup("<reg:") << "%" << getRegisterName(Base) << markup(">");
  O << ')';
}

void SystemZInstPrinter::printBDVAddrOperand(const MCInst *MI, int OpNum,
                                             raw_ostream &O) {
  printAddress(MI->getOperand(OpNum).getReg(),
               MI->getOperand(OpNum + 1).getImm(),
               MI->getOperand(OpNum + 2).getReg(), O, UseMarkup);
}

void SystemZInstPrinter::printCond4Operand(const MCInst *MI, int OpNum,
                                           raw_ostream &O) {
  static const char *const CondNames[] = {
    "o", "h", "nle", "l", "nhe", "lh", "ne",
    "e", "nlh", "he", "nl", "le", "nh", "no"
  };
  uint64_t Imm = MI->getOperand(OpNum).getImm();
  assert(Imm > 0 && Imm < 15 && "Invalid condition");
  O << CondNames[Imm - 1];
}
