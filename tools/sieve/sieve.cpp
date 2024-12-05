#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;

static std::unique_ptr<LLVMContext> Context;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;

void verify() {
  bool Broken = verifyModule(*TheModule, &errs());
  if (Broken) {
    errs() << "The module is broken!\n";
  }
}

void emitExternalFunctions() {
  FunctionType *PrintfType = FunctionType::get(Builder->getInt64Ty(), {Builder->getInt8Ty()->getPointerTo()}, true);
  Function::Create(PrintfType, Function::ExternalLinkage, "printf", *TheModule);

  FunctionType *AtoiType = FunctionType::get(Builder->getInt64Ty(), {Builder->getInt8Ty()->getPointerTo()}, false);
  Function::Create(AtoiType, Function::ExternalLinkage, "atoi", *TheModule);

  FunctionType *MallocType = FunctionType::get(Builder->getInt8Ty()->getPointerTo(), {Builder->getInt64Ty()}, false);
  Function::Create(MallocType, Function::ExternalLinkage, "malloc", *TheModule);

  FunctionType *FreeType = FunctionType::get(Builder->getVoidTy(), {Builder->getInt8Ty()->getPointerTo()}, false);
  Function::Create(FreeType, Function::ExternalLinkage, "free", *TheModule);

  FunctionType *ExitType = FunctionType::get(Builder->getVoidTy(), {Builder->getInt64Ty()}, false);
  Function::Create(ExitType, Function::ExternalLinkage, "exit", *TheModule);

  verify();
}

void emitConstants() {
  Constant *StrArraySize = ConstantDataArray::getString(*Context, "Initializing array with size of: %llu bytes \n\00");
  GlobalVariable *StrArraySizeGV = new GlobalVariable(*TheModule, StrArraySize->getType(), true, GlobalValue::PrivateLinkage, StrArraySize, ".str_array_size");
  StrArraySizeGV->setAlignment(Align(1));

  Constant *StrVal = ConstantDataArray::getString(*Context, "%llu \n\00");
  GlobalVariable *StrValGV = new GlobalVariable(*TheModule, StrVal->getType(), true, GlobalValue::PrivateLinkage, StrVal, ".str_val");
  StrValGV->setAlignment(Align(1));

  Constant *StrNewline = ConstantDataArray::getString(*Context, "\n\00");
  GlobalVariable *StrNewlineGV = new GlobalVariable(*TheModule, StrNewline->getType(), true, GlobalValue::PrivateLinkage, StrNewline, ".str_newline");
  StrNewlineGV->setAlignment(Align(1));

  Constant *StrAllocFail = ConstantDataArray::getString(*Context, "Unable to allocate memory.\n\00");
  GlobalVariable *StrAllocFailGV = new GlobalVariable(*TheModule, StrAllocFail->getType(), true, GlobalValue::PrivateLinkage, StrAllocFail, ".str_alloc_fail");
  StrAllocFailGV->setAlignment(Align(1));

  Constant *StrWrongInput = ConstantDataArray::getString(*Context, "Given value must be greater then 2.\n\00");
  GlobalVariable *StrWrongInputGV = new GlobalVariable(*TheModule, StrWrongInput->getType(), true, GlobalValue::PrivateLinkage, StrWrongInput, ".str_wrong_input");
  StrWrongInputGV->setAlignment(Align(1));

  verify();
}

void emitSetBitFunction() {
  FunctionType *SetBitType = FunctionType::get(Builder->getVoidTy(), {Builder->getInt64Ty(), Builder->getInt64Ty(), Builder->getInt8Ty()->getPointerTo(), Builder->getInt1Ty()}, false);
  Function *SetBitFunc = Function::Create(SetBitType, Function::ExternalLinkage, "set_bit", *TheModule);

  Function::arg_iterator Args = SetBitFunc->arg_begin();
  Value *ByteIdx = Args++;
  ByteIdx->setName("byte_idx");
  Value *BitIdx = Args++;
  BitIdx->setName("bit_idx");
  Value *Primes = Args++;
  Primes->setName("primes");
  Value *Val = Args++;
  Val->setName("val");

  BasicBlock *Entry = BasicBlock::Create(*Context, "entry", SetBitFunc);
  BasicBlock *SetTo1 = BasicBlock::Create(*Context, "set_to_1", SetBitFunc);
  BasicBlock *SetTo0 = BasicBlock::Create(*Context, "set_to_0", SetBitFunc);
  BasicBlock *Return = BasicBlock::Create(*Context, "return", SetBitFunc);
  
  Builder->SetInsertPoint(Entry);
  Value *BytePtr = Builder->CreateGEP(Builder->getInt8Ty(), Primes, ByteIdx, "byte_ptr");
  Value *ByteVal = Builder->CreateLoad(Builder->getInt8Ty(), BytePtr, "byte_val");
  Value *BitIdxCast = Builder->CreateTrunc(BitIdx, Builder->getInt8Ty(), "bit_idx_cast");
  Value *Mask = Builder->CreateShl(Builder->getInt8(1), BitIdxCast, "mask");
  Value *Cmp = Builder->CreateICmpEQ(Val, Builder->getInt1(true), "cmp");
  Builder->CreateCondBr(Cmp, SetTo1, SetTo0);

  Builder->SetInsertPoint(SetTo1);
  Value *NewByte1 = Builder->CreateOr(ByteVal, Mask, "new_byte_1");
  Builder->CreateStore(NewByte1, BytePtr);
  Builder->CreateBr(Return);

  Builder->SetInsertPoint(SetTo0);
  Value *InvertedMask = Builder->CreateXor(Mask, Builder->getInt8(-1), "inverted_mask");
  Value *NewByte0 = Builder->CreateAnd(ByteVal, InvertedMask, "new_byte_0");
  Builder->CreateStore(NewByte0, BytePtr);
  Builder->CreateBr(Return);

  Builder->SetInsertPoint(Return);
  Builder->CreateRetVoid();

  verify();
}

void emitCheckBitFunction() {
  FunctionType *CheckBitType = FunctionType::get(
    Builder->getInt1Ty(),
    {
      Builder->getInt64Ty(),
      Builder->getInt64Ty(),
      Builder->getInt8Ty()->getPointerTo()
    },
    false);
  Function *CheckBitFunc = Function::Create(CheckBitType, Function::ExternalLinkage, "check_bit", *TheModule);

  BasicBlock *Entry = BasicBlock::Create(*Context, "entry", CheckBitFunc);
  Builder->SetInsertPoint(Entry);

  Function::arg_iterator Args = CheckBitFunc->arg_begin();
  Value *ByteIdx = Args++;
  ByteIdx->setName("byte_idx");
  Value *BitIdx = Args++;
  BitIdx->setName("bit_idx");
  Value *Primes = Args++;
  Primes->setName("primes");

  Value *BytePtr = Builder->CreateGEP(Builder->getInt8Ty(), Primes, ByteIdx, "byte_ptr");
  Value *ByteVal = Builder->CreateLoad(Builder->getInt8Ty(), BytePtr, "byte_val");
  Value *BitIdxCast = Builder->CreateTrunc(BitIdx, Builder->getInt8Ty(), "bit_idx_cast");
  Value *Mask = Builder->CreateShl(Builder->getInt8(1), BitIdxCast, "mask");
  Value *Result = Builder->CreateAnd(ByteVal, Mask, "result");
  Value *ResultShifted = Builder->CreateLShr(Result, BitIdxCast, "result_shifted");
  Value *IsSet = Builder->CreateICmpEQ(ResultShifted, Builder->getInt8(1), "is_set");
  Builder->CreateRet(IsSet);

  verify();
}

void emitInitArrayFunction() {
  FunctionType *InitArrayType = FunctionType::get(
    Builder->getInt8Ty()->getPointerTo(),
    {Builder->getInt64Ty()},
    false
  );

  Function *InitArrayFunc = Function::Create(
    InitArrayType, Function::ExternalLinkage, "init_array", *TheModule);

  BasicBlock *Entry = BasicBlock::Create(*Context, "entry", InitArrayFunc);
  BasicBlock *AllocFail = BasicBlock::Create(*Context, "alloc_fail", InitArrayFunc);
  BasicBlock *Init = BasicBlock::Create(*Context, "init", InitArrayFunc);
  BasicBlock *Init0 = BasicBlock::Create(*Context, "init0", InitArrayFunc);
  BasicBlock *Init1 = BasicBlock::Create(*Context, "init1", InitArrayFunc);
  BasicBlock *Init2 = BasicBlock::Create(*Context, "init2", InitArrayFunc);
  BasicBlock *InitLoop = BasicBlock::Create(*Context, "init_loop", InitArrayFunc);
  BasicBlock *InitLoopCond = BasicBlock::Create(*Context, "init_loop_cond", InitArrayFunc);
  BasicBlock *InitLoopBody = BasicBlock::Create(*Context, "init_loop_body", InitArrayFunc);
  BasicBlock *Return = BasicBlock::Create(*Context, "return", InitArrayFunc);

  Builder->SetInsertPoint(Entry);
  Function::arg_iterator Args = InitArrayFunc->arg_begin();
  Value *N = Args++;
  N->setName("n");

  Value *I = Builder->CreateAlloca(Builder->getInt64Ty(), nullptr, "i");
  Value *SizeInBytes1 = Builder->CreateUDiv(N, Builder->getInt64(8), "size_in_bytes_1");
  Value *Size = Builder->CreateAdd(SizeInBytes1, Builder->getInt64(1), "size");
  Function *PrintfFunc = TheModule->getFunction("printf");
  Value *StrArraySize = TheModule->getNamedGlobal(".str_array_size");
  Builder->CreateCall(PrintfFunc, {StrArraySize, Size});
  Function *MallocFunc = TheModule->getFunction("malloc");
  Value *Primes = Builder->CreateCall(MallocFunc, {Size}, "primes");
  Value *AllocCheck = Builder->CreateICmpEQ(Primes, ConstantPointerNull::get(Builder->getInt8Ty()->getPointerTo()), "alloc_check");
  Builder->CreateCondBr(AllocCheck, AllocFail, Init);

  Builder->SetInsertPoint(AllocFail);
  Value *StrAllocFail = TheModule->getNamedGlobal(".str_alloc_fail");
  Builder->CreateCall(PrintfFunc, {StrAllocFail});
  Function *ExitFunc = TheModule->getFunction("exit");
  Builder->CreateCall(ExitFunc, {Builder->getInt64(-1)});
  Builder->CreateUnreachable();

  Builder->SetInsertPoint(Init);
  Value *Cmp1 = Builder->CreateICmpSGT(N, Builder->getInt64(0), "cmp1");
  Builder->CreateCondBr(Cmp1, Init0, Init1);

  Builder->SetInsertPoint(Init0);
  Function *SetBitFunc = TheModule->getFunction("set_bit");
  Builder->CreateCall(SetBitFunc, {Builder->getInt64(0), Builder->getInt64(0), Primes, Builder->getInt1(false)});
  Builder->CreateBr(Init1);

  Builder->SetInsertPoint(Init1);
  Value *Cmp2 = Builder->CreateICmpSGT(N, Builder->getInt64(1), "cmp2");
  Builder->CreateCondBr(Cmp2, Init2, InitLoop);

  Builder->SetInsertPoint(Init2);
  Builder->CreateCall(SetBitFunc, {Builder->getInt64(0), Builder->getInt64(1), Primes, Builder->getInt1(false)});
  Builder->CreateBr(InitLoop);

  Builder->SetInsertPoint(InitLoop);
  Builder->CreateStore(Builder->getInt64(2), I);
  Builder->CreateBr(InitLoopCond);

  Builder->SetInsertPoint(InitLoopCond);
  Value *IVal = Builder->CreateLoad(Builder->getInt64Ty(), I, "i_val");
  Value *Cmp3 = Builder->CreateICmpSLT(IVal, N, "cmp3");
  Builder->CreateCondBr(Cmp3, InitLoopBody, Return);

  Builder->SetInsertPoint(InitLoopBody);
  Value *ByteIdx = Builder->CreateUDiv(IVal, Builder->getInt64(8), "byte_idx");
  Value *BitIdx = Builder->CreateURem(IVal, Builder->getInt64(8), "bit_idx");
  Builder->CreateCall(SetBitFunc, {ByteIdx, BitIdx, Primes, Builder->getInt1(true)});
  Value *INext = Builder->CreateAdd(IVal, Builder->getInt64(1), "i_next");
  Builder->CreateStore(INext, I);
  Builder->CreateBr(InitLoopCond);

  Builder->SetInsertPoint(Return);
  Builder->CreateRet(Primes);

  verify();
}

void emitSieveFunction() {
  FunctionType *SieveType = FunctionType::get(
      Builder->getVoidTy(),
      {Builder->getInt64Ty(),
        Builder->getInt8Ty()->getPointerTo(),
        Builder->getInt64Ty()},
      false
  );

  Function *SieveFunc = Function::Create(
      SieveType, Function::ExternalLinkage, "sieve", *TheModule);

  BasicBlock *Entry = BasicBlock::Create(*Context, "entry", SieveFunc);
  BasicBlock *SieveLoopCond = BasicBlock::Create(*Context, "sieve_loop_cond", SieveFunc);
  BasicBlock *SieveLoopBody = BasicBlock::Create(*Context, "sieve_loop_body", SieveFunc);
  BasicBlock *SieveDone = BasicBlock::Create(*Context, "sieve_done", SieveFunc);

  Builder->SetInsertPoint(Entry);
  Function::arg_iterator Args = SieveFunc->arg_begin();
  Value *Prime = Args++;
  Prime->setName("prime");
  Value *Primes = Args++;
  Primes->setName("primes");
  Value *UpperBound = Args++;
  UpperBound->setName("upper_bound");
  Value *I = Builder->CreateAlloca(Builder->getInt64Ty(), nullptr, "i");
  Builder->CreateStore(Builder->getInt64(2), I);
  Value *RealUpperBound1 = Builder->CreateUDiv(UpperBound, Prime, "real_upper_bound_1");
  Value *RealUpperBound = Builder->CreateAdd(RealUpperBound1, Builder->getInt64(1), "real_upper_bound");
  Builder->CreateBr(SieveLoopCond);

  Builder->SetInsertPoint(SieveLoopCond);
  Value *IVal = Builder->CreateLoad(Builder->getInt64Ty(), I, "i_val");
  Value *Cmp1 = Builder->CreateICmpSLT(IVal, RealUpperBound, "cmp1");
  Builder->CreateCondBr(Cmp1, SieveLoopBody, SieveDone);

  Builder->SetInsertPoint(SieveLoopBody);
  Value *Product = Builder->CreateMul(IVal, Prime, "product");
  Value *ByteIdx = Builder->CreateUDiv(Product, Builder->getInt64(8), "byte_idx");
  Value *BitIdx = Builder->CreateURem(Product, Builder->getInt64(8), "bit_idx");
  Function *SetBitFunc = TheModule->getFunction("set_bit");
  Builder->CreateCall(SetBitFunc, {ByteIdx, BitIdx, Primes, Builder->getInt1(false)});
  Value *INext = Builder->CreateAdd(IVal, Builder->getInt64(1), "i_next");
  Builder->CreateStore(INext, I);
  Builder->CreateBr(SieveLoopCond);

  Builder->SetInsertPoint(SieveDone);
  Builder->CreateRetVoid();

  verify();
}

void emitNextPrimeFunction() {
  FunctionType *NextPrimeType = FunctionType::get(
      Builder->getInt64Ty(),
      {Builder->getInt64Ty(),
        Builder->getInt8Ty()->getPointerTo(),
        Builder->getInt64Ty()},
      false
  );

  Function *NextPrimeFunc = Function::Create(
      NextPrimeType, Function::ExternalLinkage, "next_prime", *TheModule);

  BasicBlock *Entry = BasicBlock::Create(*Context, "entry", NextPrimeFunc);
  BasicBlock *NextPrimeLoopCond = BasicBlock::Create(*Context, "next_prime_loop_cond", NextPrimeFunc);
  BasicBlock *NextPrimeLoopBody = BasicBlock::Create(*Context, "next_prime_loop_body", NextPrimeFunc);
  BasicBlock *NextPrimeIter = BasicBlock::Create(*Context, "next_prime_iter", NextPrimeFunc);
  BasicBlock *NextPrimeNotFound = BasicBlock::Create(*Context, "next_prime_not_found", NextPrimeFunc);
  BasicBlock *NextPrimeFound = BasicBlock::Create(*Context, "next_prime_found", NextPrimeFunc);

  Builder->SetInsertPoint(Entry);
  Function::arg_iterator Args = NextPrimeFunc->arg_begin();
  Value *Prime = Args++;
  Prime->setName("prime");
  Value *Primes = Args++;
  Primes->setName("primes");
  Value *UpperBound = Args++;
  UpperBound->setName("upper_bound");
  Value *I = Builder->CreateAlloca(Builder->getInt64Ty(), nullptr, "i");
  Value *NextPrime = Builder->CreateAlloca(Builder->getInt64Ty(), nullptr, "next_prime");
  Value *StartIndex = Builder->CreateAdd(Prime, Builder->getInt64(1), "start_index");
  Builder->CreateStore(StartIndex, I);
  Builder->CreateStore(Prime, NextPrime);
  Builder->CreateBr(NextPrimeLoopCond);

  Builder->SetInsertPoint(NextPrimeLoopCond);
  Value *IVal = Builder->CreateLoad(Builder->getInt64Ty(), I, "i_val");
  Value *Cmp1 = Builder->CreateICmpSLT(IVal, UpperBound, "cmp1");
  Builder->CreateCondBr(Cmp1, NextPrimeLoopBody, NextPrimeNotFound);

  Builder->SetInsertPoint(NextPrimeLoopBody);
  Value *ByteIdx = Builder->CreateUDiv(IVal, Builder->getInt64(8), "byte_idx");
  Value *BitIdx = Builder->CreateURem(IVal, Builder->getInt64(8), "bit_idx");
  Function *CheckBitFunc = TheModule->getFunction("check_bit");
  Value *IsSet = Builder->CreateCall(CheckBitFunc, {ByteIdx, BitIdx, Primes}, "is_set");
  Builder->CreateCondBr(IsSet, NextPrimeFound, NextPrimeIter);

  Builder->SetInsertPoint(NextPrimeIter);
  Value *INext = Builder->CreateAdd(IVal, Builder->getInt64(1), "i_next");
  Builder->CreateStore(INext, I);
  Builder->CreateBr(NextPrimeLoopCond);

  Builder->SetInsertPoint(NextPrimeNotFound);
  Value *NextPrimeVal = Builder->CreateLoad(Builder->getInt64Ty(), NextPrime, "next_prime_val");
  Builder->CreateRet(NextPrimeVal);

  Builder->SetInsertPoint(NextPrimeFound);
  Builder->CreateRet(IVal);

  verify();
}

void emitCalcPrimesFunction() {
  FunctionType *CalcPrimesType = FunctionType::get(
      Builder->getVoidTy(),
      {Builder->getInt64Ty(),
        Builder->getInt8Ty()->getPointerTo()},
      false
  );

  Function *CalcPrimesFunc = Function::Create(
      CalcPrimesType, Function::ExternalLinkage, "calc_primes", *TheModule);

  BasicBlock *Entry = BasicBlock::Create(*Context, "entry", CalcPrimesFunc);
  BasicBlock *CalcPrimesLoopCond = BasicBlock::Create(*Context, "calc_primes_loop_cond", CalcPrimesFunc);
  BasicBlock *CalcPrimesLoopBody = BasicBlock::Create(*Context, "calc_primes_loop_body", CalcPrimesFunc);
  BasicBlock *CalcPrimesDone = BasicBlock::Create(*Context, "calc_primes_done", CalcPrimesFunc);

  Builder->SetInsertPoint(Entry);
  Function::arg_iterator Args = CalcPrimesFunc->arg_begin();
  Value *N = Args++;
  N->setName("n");
  Value *Primes = Args++;
  Primes->setName("primes");
  Value *I = Builder->CreateAlloca(Builder->getInt64Ty(), nullptr, "i");
  Builder->CreateStore(Builder->getInt64(2), I);
  Builder->CreateBr(CalcPrimesLoopCond);

  Builder->SetInsertPoint(CalcPrimesLoopCond);
  Value *IVal = Builder->CreateLoad(Builder->getInt64Ty(), I, "i_val");
  Value *Cmp1 = Builder->CreateICmpSLT(IVal, N, "cmp1");
  Builder->CreateCondBr(Cmp1, CalcPrimesLoopBody, CalcPrimesDone);

  Builder->SetInsertPoint(CalcPrimesLoopBody);
  Function *PrintfFunc = TheModule->getFunction("printf");
  GlobalVariable *StrVal = TheModule->getNamedGlobal(".str_val");
  Value *StrValPtr = Builder->CreatePointerCast(StrVal, Builder->getInt8Ty()->getPointerTo());
  Builder->CreateCall(PrintfFunc, {StrValPtr, IVal});
  Function *SieveFunc = TheModule->getFunction("sieve");
  Builder->CreateCall(SieveFunc, {IVal, Primes, N});
  Function *NextPrimeFunc = TheModule->getFunction("next_prime");
  Value *NextPrime = Builder->CreateCall(NextPrimeFunc, {IVal, Primes, N}, "next_prime");
  Builder->CreateStore(NextPrime, I);
  Value *Cmp2 = Builder->CreateICmpEQ(NextPrime, IVal, "cmp2");
  Builder->CreateCondBr(Cmp2, CalcPrimesDone, CalcPrimesLoopCond);

  Builder->SetInsertPoint(CalcPrimesDone);
  Builder->CreateRetVoid();

  verify();
}

void emitMainFunction() {
  FunctionType *MainType = FunctionType::get(
      Builder->getInt64Ty(),
      {Builder->getInt64Ty(),
        Builder->getInt8Ty()->getPointerTo()},
      false
  );

  Function *MainFunc = Function::Create(
      MainType, Function::ExternalLinkage, "main", *TheModule);

  BasicBlock *Entry = BasicBlock::Create(*Context, "entry", MainFunc);
  BasicBlock *ParseInput = BasicBlock::Create(*Context, "parse_input", MainFunc);
  BasicBlock *MainBody = BasicBlock::Create(*Context, "main_body", MainFunc);
  BasicBlock *Cleanup = BasicBlock::Create(*Context, "cleanup", MainFunc);
  BasicBlock *Error = BasicBlock::Create(*Context, "error", MainFunc);

  Builder->SetInsertPoint(Entry);
  Function::arg_iterator Args = MainFunc->arg_begin();
  Value *Argc = Args++;
  Argc->setName("argc");
  Value *Argv = Args++;
  Argv->setName("argv");
  Value *ArgcCheck = Builder->CreateICmpEQ(Argc, Builder->getInt64(2), "argc_check");
  Builder->CreateCondBr(ArgcCheck, ParseInput, Error);

  Builder->SetInsertPoint(ParseInput);
  Value *NPtr = Builder->CreateGEP(Builder->getInt8Ty()->getPointerTo(), Argv, Builder->getInt64(1), "n_ptr");
  Value *NArg = Builder->CreateLoad(Builder->getInt8Ty()->getPointerTo(), NPtr, "n_arg");
  Function *AtoiFunc = TheModule->getFunction("atoi");
  Value *N = Builder->CreateCall(AtoiFunc, {NArg}, "n");
  Value *NValid = Builder->CreateICmpSGT(N, Builder->getInt64(2), "n_valid");
  Builder->CreateCondBr(NValid, MainBody, Error);

  Builder->SetInsertPoint(MainBody);
  Function *InitArrayFunc = TheModule->getFunction("init_array");
  Value *Primes = Builder->CreateCall(InitArrayFunc, {N}, "primes");
  Function *CalcPrimesFunc = TheModule->getFunction("calc_primes");
  Builder->CreateCall(CalcPrimesFunc, {N, Primes});
  Builder->CreateBr(Cleanup);

  Builder->SetInsertPoint(Cleanup);
  Function *FreeFunc = TheModule->getFunction("free");
  Builder->CreateCall(FreeFunc, {Primes});
  Builder->CreateRet(Builder->getInt64(0));

  Builder->SetInsertPoint(Error);
  Function *PrintfFunc = TheModule->getFunction("printf");
  GlobalVariable *StrWrongInput = TheModule->getNamedGlobal(".str_wrong_input");
  Value *StrWrongInputPtr = Builder->CreatePointerCast(StrWrongInput, Builder->getInt8Ty()->getPointerTo());
  Builder->CreateCall(PrintfFunc, {StrWrongInputPtr});
  Function *ExitFunc = TheModule->getFunction("exit");
  Builder->CreateCall(ExitFunc, {Builder->getInt64(-1)});
  Builder->CreateUnreachable();

  verify();
}

int main() {
  Context = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("MSU CMC - TOPT 2024, 527, Buklov Boris", *Context);
  Builder = std::make_unique<IRBuilder<>>(*Context);

  emitExternalFunctions();  
  emitConstants();
  emitSetBitFunction();
  emitCheckBitFunction();
  emitInitArrayFunction();
  emitSieveFunction();
  emitNextPrimeFunction();
  emitCalcPrimesFunction();
  emitMainFunction();

  TheModule->print(outs(), nullptr);

  return 0;
}