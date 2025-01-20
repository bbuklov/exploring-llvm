#include <llvm/ADT/Statistic.h>
#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/ValueLattice.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/User.h>
#include <llvm/InitializePasses.h>
#include <llvm/Pass.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Local.h>
#include "llvm/Analysis/InstructionSimplify.h"

#include "topt/DataFlow/SCCPSolver.h"

#define DEBUG_TYPE "SCCPSolver"

namespace llvm {
namespace trainOpt {
void Solver::solve() {
  while (!BBWorkList.empty() || !InstWorkList.empty() ||
         !OverdefinedInstWorkList.empty()) {

    while (!OverdefinedInstWorkList.empty()) {
      auto *I = OverdefinedInstWorkList.pop_back_val();
      markUsersAsChanged(I);
      outs() << "Marked users of overdefined instruction ";
      I->print(outs());
      outs() << " as changed." << "\n";
    }

    while (!InstWorkList.empty()) {
      auto *I = InstWorkList.pop_back_val();
      
      if (I->getType()->isStructTy() || !getValueState(I).isOverdefined())
        markUsersAsChanged(I);
        outs() << "Marked users of ";
        I->print(outs());
        outs() << " as changed.\n";
    }

    while (!BBWorkList.empty()) {
      auto *BB = BBWorkList.pop_back_val();
      
      visit(BB);
      outs() << "Visited basic block ";
      BB->print(outs());
      outs() << "\n";
    }
  }
}

bool Solver::markBlockExecutable(BasicBlock *BB) {
  if (!BBExecutable.insert(BB).second) {
    outs() << "Basic block ";
    BB->print(outs());
    outs() << " is not marked as executable (already is).\n";
    return false;
  }
  BBWorkList.push_back(BB); // Add the block to the worklist!
  outs() << "Marked basic block ";
  BB->print(outs());
  outs() << " as executable.\n";
  return true;
}

void Solver::markOverdefined(Value *V) {
  if (auto *STy = dyn_cast<StructType>(V->getType())) {
    assert(false && "StructType is unsupported!");
    outs() << "StructType is unsupported!\n";
  } else {
    markOverdefined(ValueState[V], V);
    outs() << "Marked value ";
    V->print(outs());
    outs() << " as overdefined.\n";
  }
}

bool Solver::isBlockExecutable(BasicBlock *BB) {
  if (BBExecutable.count(BB)) {
    outs() << "Basic block ";
    BB->print(outs());
    outs() << " is executable.\n";
    return true;
  }
  outs() << "Basic block ";
  BB->print(outs());
  outs() << " is not executable.\n";
  return false;
}

const LatticeVal &Solver::getLatticeValueFor(Value *V) const {
  const auto I = ValueState.find(V);
  if (I == ValueState.end()) {
    V->print(outs());
  }
  outs() << "Got lattice value " << I->getSecond().getConstantInt() << " for value ";
  V->print(outs());
  outs() << "\n";
  assert(I != ValueState.end() && "V is not found in ValueState");
  return I->second;
}

/**
 *  Private methods!
 */

void Solver::visitBinaryOperator(Instruction &I) {
  outs() << "Visited binary operator instruction ";
  I.print(outs());
  outs() << "\n";
  LatticeVal V1State = getValueState(I.getOperand(0));
  LatticeVal V2State = getValueState(I.getOperand(1));

  LatticeVal &IV = ValueState[&I];
  if (IV.isOverdefined()) {
    outs() << "Instruction already marked as overdefined\n";
    // Fast exit
    return;
  }

  if (V1State.isConstant() && V2State.isConstant()) {
    // Try to calculate value from two constants
    Value *R = simplifyBinOp(
      I.getOpcode(),
      V1State.getConstant(),
      V2State.getConstant(),\
      SimplifyQuery(DL, &I)
    );
    Constant *C = dyn_cast<Constant>(R);
    markConstant(&I, C);
    outs() << "Calculated new constant value ";
    C->print(outs());
    outs() << " for instruction.\n";

    return;
  }

  if (!V1State.isOverdefined() && !V2State.isOverdefined()) {
    // One of operand is a constant, another is unknown
    // Treat resulting value as constant then
    if (V1State.isConstant()) {
      markConstant(&I, V1State.getConstant());
      outs() << "Marked instruction as constant " << V1State.getConstantInt() << "\n";
    } else {
      markConstant(&I, V2State.getConstant());
      outs() << "Marked instruction as constant " << V2State.getConstantInt() << "\n";
    }

    return;
  }
  // One of operands is overdefined
  // Resultring value is overdefined also then
  markOverdefined(&I);
  outs() << "Marked instruction as overdefined.\n";
}

void Solver::visitCmpInst(CmpInst &I) {
  outs() << "Visited cmp operator instruction ";
  I.print(outs());
  outs() << ".\n";

  LatticeVal V1State = getValueState(I.getOperand(0));
  LatticeVal V2State = getValueState(I.getOperand(1));

  LatticeVal &IV = ValueState[&I];
  if (IV.isOverdefined()) {
    outs() << "Instruction already marked as overdefined.\n";
    // Fast exit
    return;
  }
 
  if (V1State.isConstant() && V2State.isConstant()) {
    // Try to calculate instruction from 2 constants
    Value *R = simplifyCmpInst(
      I.getPredicate(),
      V1State.getConstant(),
      V2State.getConstant(),\
      SimplifyQuery(DL, &I)
    );
    Constant *C = dyn_cast<Constant>(R);
    markConstant(&I, C);
    outs() << "Calculated new constant value ";
    C->print(outs());
    outs() << " for instruction.\n";

    return;
  }
 
  if (!V1State.isOverdefined() && !V2State.isOverdefined()) {
    // One of operand is a constant, another is unknown
    // Treat resulting value as constant then
    if (V1State.isConstant()) {
      markConstant(&I, V1State.getConstant());
      outs() << "Marked instruction as constant " << V1State.getConstantInt() << "\n";
    } else {
      markConstant(&I, V2State.getConstant());
      outs() << "Marked instruction as constant " << V2State.getConstantInt() << "\n";
    }

    return;
  }

  // One of operands is overdefined
  // Resultring value is overdefined also then
  markOverdefined(&I);
  outs() << "Marked instruction as overdefined.\n";
}

void Solver::visitTerminator(Instruction &I) {
  outs() << "Visited terminator instruction ";
  I.print(outs());
  outs() << "\n";
  SmallVector<bool, 16> SuccFeasible;
  getFeasibleSuccessors(I, SuccFeasible);
 
  BasicBlock *BB = I.getParent();
 
  // Mark all feasible successors executable.
  for (unsigned i = 0, e = SuccFeasible.size(); i != e; ++i)
    if (SuccFeasible[i]) {
      markEdgeExecutable(BB, I.getSuccessor(i));
      outs() << "Marked edge ";
      I.getSuccessor(i)->print(outs());
      outs() << " as executable.\n";
    }
}

void Solver::visitPHINode(PHINode &PN) {
  outs() << "Visited phi instruction ";
  PN.print(outs());
  outs() << ".\n";

  // Structs are not supported
  if (PN.getType()->isStructTy()) {
    outs() << "Structs are not supported.";
    return (void)markOverdefined(&PN);
  }
  
  // Fast exit
  if (getValueState(&PN).isOverdefined()) {
    outs() << "Instruction already marked as overdefined\n";
    return;
  }
  
  // Super-extra-high-degree PHI nodes are unlikely to ever be marked constant,
  // and slow us down a lot.  Just mark them overdefined. (Taken from orig llvm code)
  if (PN.getNumIncomingValues() > 64)
    return (void)markOverdefined(&PN);
 
  LatticeVal PhiState = getValueState(&PN);
  for (unsigned i = 0; i < PN.getNumIncomingValues(); i++) {
    LatticeVal &IV = getValueState(PN.getIncomingValue(i));
    if (IV.isUnknown()) {
      outs() << "Skipped edge ";
      PN.getIncomingBlock(i)->print(outs());
      outs() << " as it's value is unknown.";
      continue;
    }
    // Skip all not executable operands
    if (!isEdgeFeasible(PN.getIncomingBlock(i), PN.getParent())) {
      outs() << "Skipped edge ";
      PN.getIncomingBlock(i)->print(outs());
      outs() << " as it is not feasible.";
      continue;
    }

    // If some of incoming value is overdefined -
    // then the phi-node value is also overdefined (conservative way)
    // Stop calculation - we know for sure it is not a constant
    if (IV.isOverdefined()) {
      PhiState.markOverdefined();
      outs() << "Marked instruction as overdefined.\n";
      break;
    }

    // If incoming value is constant -
    // then mark phi-node value is constant also
    if (IV.isConstant()) {
      PhiState.markConstant(IV.getConstant());
      outs() << "Marked instruction as constant " << IV.getConstantInt() << "\n";
    }
  }
}

void Solver::visitReturnInst(ReturnInst &I) {}

bool Solver::isEdgeFeasible(BasicBlock *From, BasicBlock *To) {
  if (KnownFeasibleEdges.count(Edge(From, To))) {
    return true;
  }
  return false;
}

void Solver::getFeasibleSuccessors(Instruction &I,
                                   SmallVector<bool, 16> &Succs) {
  Succs.resize(I.getNumSuccessors());
  if (auto *BI = dyn_cast<BranchInst>(&I)) {
    if (BI->isUnconditional()) {
      Succs[0] = true;
      return;
    }

    LatticeVal BCValue = getValueState(BI->getCondition());
    ConstantInt *CI = BCValue.getConstantInt();
    if (!CI) {
      if (!BCValue.isUnknown()) {
        Succs[0] = Succs[1] = true;
      }
      return;
    }
    Succs[CI->isZero()] = true;
    return;
  }
  assert(false && "Unsupported Terminator!");
}

bool Solver::markEdgeExecutable(BasicBlock *Source, BasicBlock *Dest) {
  if (!KnownFeasibleEdges.insert(Edge(Source, Dest)).second) {
    return false;
  }
  if (!markBlockExecutable(Dest)) {
    for (PHINode &PN : Dest->phis()) {
      visitPHINode(PN);
    }
  }
  return true;
}

void Solver::pushToWorkList(LatticeVal &IV, Value *V) {
  if (IV.isOverdefined()) {
    OverdefinedInstWorkList.push_back(V);
    return;
  }
  InstWorkList.push_back(V);
}

bool Solver::markConstant(LatticeVal &IV, Value *V, Constant *C) {
  if (!IV.markConstant(C)) {
    return false;
  }
  pushToWorkList(IV, V);
  return true;
}

bool Solver::markConstant(Value *V, Constant *C) {
  return markConstant(ValueState[V], V, C);
}

bool Solver::markOverdefined(LatticeVal &IV, Value *V) {
  if (!IV.markOverdefined()) {
    return false;
  }
  // Only instructions get into the work list
  pushToWorkList(IV, V);
  return true;
}

void Solver::markUsersAsChanged(Value *I) {
  for (User *U : I->users()) {
    if (auto *UI = dyn_cast<Instruction>(U)) {
      if (BBExecutable.count(UI->getParent())) {
        visit(*UI);
      }
    }
  }
}

LatticeVal &Solver::getValueState(Value *V) {
  assert(!V->getType()->isStructTy() && "StructType is unsupported");

  std::pair<DenseMap<Value *, LatticeVal>::iterator, bool> I =
      ValueState.insert(std::make_pair(V, LatticeVal()));
  LatticeVal &LV = I.first->second;

  if (!I.second) {
    return LV; // Common case, already in the map.
  }

  if (auto *C = dyn_cast<Constant>(V)) {
    // Undef values remain unknown.
    if (!isa<UndefValue>(V)) {
      LV.markConstant(C);
    }
  }

  // All others are underdefined by default.
  return LV;
}
} // namespace trainOpt
} // namespace llvm
