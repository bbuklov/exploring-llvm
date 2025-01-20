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

#include "topt/LocalOpt/LVN.h"

#include <vector>
#include <string>
#include <unordered_map>

using namespace llvm;

#define DEBUG_TYPE "lvn"

namespace llvm {

namespace trainOpt {

std::string UnionFind::find(const std::string &x) {
  if (parent.find(x) == parent.end()) {
    parent[x] = x;
  }
  if (parent[x] != x) {
    parent[x] = find(parent[x]); // Path compression
  }
  return parent[x];
}

void UnionFind::unite(const std::string &x, const std::string &y) {
  std::string rootX = find(x);
  std::string rootY = find(y);
  // TODO: possible pseudo-generator attack?
  if (std::rand() % 2 == 0)
    std::swap(rootX, rootY);
  parent[rootX] = rootY;
}

PreservedAnalyses LVNPass::run(Function &F, FunctionAnalysisManager &AM) {
  for (BasicBlock &BB : F) {
    std::vector<std::tuple<std::string, std::vector<std::string>, std::vector<std::string>>> DAGTable;
    UnionFind UF;

    outs() << "DAG Table for Basic Block\n";
    BB.print(outs());
    outs() << ":\n";

    // Handle function arguments as leaf nodes
    for (Argument &Arg : F.args()) {
      std::string Operation = ID_OPERATION; 
      std::vector<std::string> Operands = {UNARY_RIGHT_VAL};
      std::string Result = VARIABLE_MARKER + std::to_string(reinterpret_cast<uintptr_t>(&Arg));
      DAGTable.emplace_back(Operation, Operands, std::vector<std::string>{Result});
      UF.unite(Result, Result);
    }

    for (auto I = BB.begin(); I != BB.end();) {
      Instruction &Inst = *I;
      std::string Operation;
      std::vector<std::string> Operands;
      std::string Result;

      // Handle constants and variables as leaf nodes
      if (auto *C = dyn_cast<Constant>(&Inst)) {
        Operation = NM_OPERATION;
        Operands.push_back(UNARY_RIGHT_VAL);
        Result = CONSTANT_MARKER + std::to_string(C->getUniqueInteger().getSExtValue());
      } else if (Inst.getNumOperands() == 0) {
        Operation = ID_OPERATION;
        Operands.push_back(UNARY_RIGHT_VAL);
        Result = VARIABLE_MARKER + std::to_string(reinterpret_cast<uintptr_t>(&Inst));
      } else {
        Operation = Inst.getOpcodeName();
        for (Value *Op : Inst.operands()) {
          std::string OperandKey;
          if (auto *ConstOp = dyn_cast<Constant>(Op)) {
            OperandKey = CONSTANT_MARKER + std::to_string(ConstOp->getUniqueInteger().getSExtValue());
          } else {
            OperandKey = VARIABLE_MARKER + std::to_string(reinterpret_cast<uintptr_t>(Op));
            OperandKey = UF.find(OperandKey);
          }
          Operands.push_back(OperandKey);
        }
        Result = VARIABLE_MARKER + std::to_string(reinterpret_cast<uintptr_t>(&Inst));
      }

      // Check if operation and operands already exist in the table
      bool Found = false;
      for (auto &Entry : DAGTable) {
        if (std::get<0>(Entry) == Operation && std::get<1>(Entry) == Operands) {
          std::get<2>(Entry).push_back(Result);
          Found = true;
          
          UF.unite(Result, std::get<2>(Entry).front());
          
          Inst.replaceAllUsesWith(reinterpret_cast<Value *>(std::stoull(std::get<2>(Entry).front().substr(1))));

          I = Inst.eraseFromParent();
          break;
        }
      }

      // If not found, add a new entry
      if (!Found) {
        DAGTable.emplace_back(Operation, Operands, std::vector<std::string>{Result});
        UF.unite(Result, Result);
        ++I;
      }
    }

    for (const auto &Entry : DAGTable) {
      const std::string &Operation = std::get<0>(Entry);
      const std::vector<std::string> &Operands = std::get<1>(Entry);
      const std::vector<std::string> &Results = std::get<2>(Entry);

      outs() << "Operation: " << Operation << ", Operands: [";
      for (size_t i = 0; i < Operands.size(); ++i) {
        outs() << Operands[i];
        if (i < Operands.size() - 1)
          outs() << ", ";
      }
      outs() << "], Results: [";
      for (size_t i = 0; i < Results.size(); ++i) {
        outs() << Results[i];
        if (i < Results.size() - 1)
          outs() << ", ";
      }
      outs() << "]\n";
    }
  }
  return PreservedAnalyses::none();
}
} // namespace trainOpt
} // namespace llvm
