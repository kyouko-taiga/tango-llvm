//
//  irgen.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include <llvm/IR/Verifier.h>

#include "irgen.hh"


namespace tango {
namespace irgen {

    IRGenerator::IRGenerator(
        llvm::Module& mod,
        llvm::IRBuilder<>& irb):
    module(mod), builder(llvm::IRBuilder<>(mod.getContext())) {}


    void IRGenerator::add_main_function() {
        auto i32  = llvm::IntegerType::getInt32Ty(module.getContext());
        auto i8   = llvm::IntegerType::getInt8Ty(module.getContext());
        auto i8pp = llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(i8));

        // define i32 @main(i32, i8**)
        auto fn = static_cast<llvm::Function*>(module.getOrInsertFunction("main", i32, i32, i8pp));
        llvm::BasicBlock::Create(module.getContext(), "entry", fn);
    }


    void IRGenerator::finish_main_function(llvm::Value* exit_status) {
        auto main_fun = module.getFunction("main");

        // ret i32 <exit_status>
        llvm::IRBuilder<> tb(&main_fun->getEntryBlock());
        tb.CreateRet(
            (exit_status != nullptr)
            ? exit_status
            : llvm::ConstantInt::get(module.getContext(), llvm::APInt(32, 0)));

        llvm::verifyFunction(*main_fun);
    }


    llvm::Value* IRGenerator::get_symbol_location(const std::string& name) {
        auto local_it = locals.find(name);
        if (local_it != locals.end()) {
            return local_it->second;
        }

        auto global_it = globals.find(name);
        if (global_it != globals.end()) {
            return global_it->second;
        }

        throw std::invalid_argument("undefined symbol");
    }


    llvm::AllocaInst* create_alloca(
        llvm::Function*    fun,
        llvm::Type*        type,
        const std::string& name)
    {
        llvm::IRBuilder<> tmp_builder(&fun->getEntryBlock(), fun->getEntryBlock().begin());
        return tmp_builder.CreateAlloca(type, 0, name);
    }

} // namespace irgen
} // namespace tango
