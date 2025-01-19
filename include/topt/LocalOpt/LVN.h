#ifndef TOPT_LOCAL_OPT_LVN_H
#define TOPT_LOCAL_OPT_LVN_H

#include <llvm/IR/PassManager.h>
#include <string>
#include <unordered_map>

namespace llvm {
class Function;

namespace trainOpt {
/** Constants for operation types */
const std::string ID_OPERATION = "id";
const std::string NM_OPERATION = "nm";
const std::string CONSTANT_MARKER = "C";
const std::string VARIABLE_MARKER = "V";
const std::string UNARY_RIGHT_VAL = "0";

/**
 * UnionFind
 * 
 * This class implements the Union-Find structure for managing equivalence classes.
 * It supports efficient union and find operations, with path compression to optimize performance.
 */
class UnionFind {
private:
  std::unordered_map<std::string, std::string> parent; ///< Map storing parent pointers for Union-Find

public:
  /**
   * Finds the representative of the set containing the given element.
   * 
   * @param x The element whose set representative is to be found.
   * @return The representative of the set containing x.
   */
  std::string find(const std::string &x);
  /**
   * Unites the sets containing two elements.
   * 
   * @param x The first element.
   * @param y The second element.
   * @note The representative (parent) of the set becomes the representative of the second element (y).
   */
  void unite(const std::string &x, const std::string &y);
};

/**
 *  LVN - Local Value Numbering.
 *  This pass performs Local Value Numbering (LVN) to optimize basic blocks by identifying
 *  and eliminating redundant computations. It replaces equivalent instructions with a
 *  single representative and removes unnecessary instructions.
 */
class LVNPass : public PassInfoMixin<LVNPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};
} // namespace trainOpt
} // namespace llvm

#endif // TOPT_LOCAL_OPT_LVN_H
