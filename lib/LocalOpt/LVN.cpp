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
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/ADT/DenseMap.h>
#include <optional>

#include "topt/LocalOpt/LVN.h"

using namespace llvm;

#define DEBUG_TYPE "lvn"

namespace llvm {

namespace trainOpt {

// сигнатура инструкции
struct Signature {
  StringRef op;  // название операции
  SmallVector<StringRef> operands;  // операнды инструкции
  SmallVector<StringRef> variables;  // ассоциированные переменные

  bool operator==(const Signature &other) const {
    return op == other.op && operands == other.operands;
  }

  bool operator!=(const Signature &other) const {
    return !(*this == other);
  }

  // для использования в качестве ключа
  struct Hash {
    std::size_t operator()(const Signature &sig) const {
      return computeHash(sig);
    }

    static unsigned getHashValue(const Signature &sig) {
      return computeHash(sig);
    }

    static Signature getEmptyKey() {
      return {"", {}, {}};
    }

    static Signature getTombstoneKey() {
      return {"<tombstone>", {}, {}};
    }

    static bool isEqual(const Signature &lhs, const Signature &rhs) {
      if (lhs.op == getEmptyKey().op || lhs.op == getTombstoneKey().op ||
        rhs.op == getEmptyKey().op || rhs.op == getTombstoneKey().op) {
        return lhs.op == rhs.op;
      }
      return lhs == rhs;
    }

  private:
    static std::size_t computeHash(const Signature &sig) {
      std::size_t hashVal = hash_value(sig.op);
      for (StringRef operand : sig.operands) {
        hashVal ^= hash_value(operand);
      }
      return hashVal;
    }
  };
};

/** 
 * @brief Линейный поиск ассоциированной сигнатуры по имени переменной в таблице ОАГ.
 */
Signature findSignatureByVariable(DenseMap<Signature, unsigned, Signature::Hash> DAG, StringRef variable) {
  for (auto &pair : DAG) {
    const Signature &sig = pair.first;
    if (llvm::is_contained(sig.variables, variable))
      return sig;
  }

  llvm_unreachable("Met operand in expression must exist in DAG table already.");
}

/** 
 * @brief Линейный поиск ассоциированной переменной по имени операнда в таблице ОАГ.
 * 
 * Возвращает первую в списке ассоциированную переменную.
 */
StringRef findAssotiatedVariable(DenseMap<Signature, unsigned, Signature::Hash> DAG, StringRef operand) {
  auto foundSignature = findSignatureByVariable(DAG, operand);
  return foundSignature.variables[0];
}

bool _updateSignature(DenseMap<Signature, unsigned, Signature::Hash> DAG, Signature sig) {
  auto it = DAG.find(sig);
  if (it != DAG.end()) {
    if (llvm::is_contained(sig.variables, sig.variables[0]))
      llvm_unreachable("Assotiated variables of a signature must be unique.");
    it->first.variables.push_back(sig.variables[0]);
    return true;
  }
  
  return false;
}

/** 
 * @brief Пытается обновить соответсвующую для данной сигнатуры запись в таблице ОАГ.
 * 
 * Пытается найти запись в таблице ОАГ и добавить связанную с ней переменную.
 * Если такой записи нет, то пытается найти запись с подставленными связанными операндами.
 * Если обновление происходит успешно, возвращает true, иначе false.
 */
bool updateSignature(DenseMap<Signature, unsigned, Signature::Hash> DAG, Signature sig) {
  if (!_updateSignature(DAG, sig)) {
    // пытаемся подставить вместо встреченных операндов, ассоциированные с ними в таблице ОАГ
    // и таким образом найти неявные дупликаты
    Signature tempSig;
    tempSig.op = sig.op;
    for (unsigned i = 0; i < tempSig.operands.size(); ++i) {
      StringRef operand = tempSig.operands[i];
      StringRef assotiatedVariable = findAssotiatedVariable(DAG, operand);
      tempSig.operands.push_back(assotiatedVariable);
    }

    if (!_updateSignature(DAG, tempSig))
      return false;
  }

  return true;
}

PreservedAnalyses LVNPass::run(Function &F, FunctionAnalysisManager &AM) {
  for (BasicBlock &BB : F) {
    // оптимизация LVN работает в рамках кажого ББ
    DenseMap<Signature, unsigned, Signature::Hash> DAG;  // таблица ОАГ
    unsigned idx = 0;

    for (Instruction &Inst : BB) {
      if (isSafeToSpeculativelyExecute(&Inst)) {
        // пропускаем небезопасные для LVN-оптимизации инструкции
        // по хорошему нужно их запоминать для дальнейшего восстановления ББ по ОАГ
        idx++;
        continue;
      }

      Signature sig;
      sig.op = Inst.getOpcodeName();  // имя операции

      // заполняем операнды
      for (unsigned i = 0; i < Inst.getNumOperands(); ++i) {
          Value *operand = Inst.getOperand(i);
          sig.operands.push_back(operand->getName());
      }

      if (!updateSignature(DAG, sig))
        // если не удается обновить значение в таблице, добавляем новое
        DAG[sig] = idx;
      
      idx++;
    }

    outs() << "For basic block:" << "\n";
    BB.print(outs());
    outs() << "Generated DAG table:" << "\n";
    outs() << "#NUMBER\t#CODE\t#OPERANDS\t#VARIABLES" << "\n";
    for (const auto &pair : DAG) {
      const Signature &key = pair.first;
      unsigned value = pair.second;
      outs() << value << "\t";
      outs() << key.op << "\t";
      for (unsigned i = 0; i < key.operands.size(); ++i) {
        if (i < key.operands.size() - 1) {
          outs() << key.operands[i] << ",";
        } else {
          outs() << key.operands[i] << "\t";
        }
      }
      for (unsigned i = 0; i < key.variables.size(); ++i) {
        if (i < key.variables.size() - 1) {
          outs() << key.variables[i] << ",";
        } else {
          outs() << key.variables[i] << "\t";
        }
      }
    }
  }

  return PreservedAnalyses::none();
}

}  // namespace trainOpt
}  // namespace llvm
