#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

int main() {
    LLVMContext Context;
    auto M = std::make_unique<Module>("calc", Context);
    IRBuilder<> Builder(Context);

    // 函数：int add(int a, int b)
    FunctionType *AddTy =
        FunctionType::get(Type::getInt32Ty(Context),
                          {Type::getInt32Ty(Context), Type::getInt32Ty(Context)},
                          false);
    Function *AddFunc = Function::Create(AddTy, Function::ExternalLinkage, "add", *M);

    BasicBlock *AddBB = BasicBlock::Create(Context, "entry", AddFunc);
    Builder.SetInsertPoint(AddBB);

    auto args = AddFunc->arg_begin();
    Value *a = args++;
    Value *b = args;

    Value *sum = Builder.CreateAdd(a, b, "sum");
    Builder.CreateRet(sum);

    // 创建 main 函数：调用 add(10, 32)
    FunctionType *MainTy = FunctionType::get(Type::getInt32Ty(Context), false);
    Function *MainFunc = Function::Create(MainTy, Function::ExternalLinkage, "main", *M);
    BasicBlock *MainBB = BasicBlock::Create(Context, "entry", MainFunc);
    Builder.SetInsertPoint(MainBB);

    Value *ret = Builder.CreateCall(AddFunc,
                                    {ConstantInt::get(Type::getInt32Ty(Context), 10),
                                     ConstantInt::get(Type::getInt32Ty(Context), 32)});
    Builder.CreateRet(ret);

    // 输出 IR
    M->print(outs(), nullptr);

    return 0;
}
