#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>


using namespace llvm;

int main() {
    LLVMContext Context;

    // 创建 Module（相当于一个 IR 文件）
    std::unique_ptr<Module> M = std::make_unique<Module>("my_module", Context);

    // 创建 main 函数: int main()
    FunctionType *FT = FunctionType::get(Type::getInt32Ty(Context), false);
    Function *MainFunc = Function::Create(FT, Function::ExternalLinkage, "main", *M);

    // 在 main 中创建一个基本块
    BasicBlock *BB = BasicBlock::Create(Context, "entry", MainFunc);
    IRBuilder<> Builder(BB);



    // return 42;
    Value *RetVal = ConstantInt::get(Type::getInt32Ty(Context), 42);
    Builder.CreateRet(RetVal);

    // 输出 IR 到 stdout
    M->print(outs(), nullptr);

    return 0;
}
