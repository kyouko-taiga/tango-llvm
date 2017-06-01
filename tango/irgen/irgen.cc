//
//  irgen.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include <algorithm>
#include <llvm/IR/Verifier.h>

#include "irgen.hh"


namespace tango {
namespace irgen {

    IRGenerator::IRGenerator(
        llvm::Module& mod,
        llvm::IRBuilder<>& irb):
        module(mod), builder(llvm::IRBuilder<>(mod.getContext())), tango_types(mod.getContext()) {}


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


    void IRGenerator::move_to_main_function() {
        auto main_fun = module.getFunction("main");
        if (main_fun == nullptr) {
            throw std::invalid_argument("invalid top-level statement");
        }
        builder.SetInsertPoint(&main_fun->getEntryBlock());

        // Note that we don't need to clear the insertion point once we've
        // generated the assignment instruction, as either we'll put
        // another instruction after, or we'll set the insertion point in
        // another function's body anyway.
    }


    llvm::Value* IRGenerator::get_symbol_location(const std::string& name) {
        if (!locals.empty()) {
            auto it = locals.top().find(name);
            if (it != locals.top().end()) {
                return it->second;
            }
        }

        if (!local_captures.empty()) {
            auto fun_name = builder.GetInsertBlock()->getParent()->getName();
            auto it       = locals.top().find(fun_name);
            if (it != locals.top().end()) {
                auto closure = builder.CreateLoad(it->second);
                if (name == fun_name) {
                    return it->second;
                }

                auto captures   = local_captures.top();
                auto capture_it = std::find(captures.begin(), captures.end(), name);
                if (capture_it != captures.end()) {
                    auto zero    = get_gep_index(0);
                    auto raw_ptr = builder.CreateLoad(
                        builder.CreateGEP(closure, {zero, get_gep_index(1)}));
                    auto env_ptr = builder.CreateBitCast(
                        raw_ptr, llvm::PointerType::getUnqual(closures[fun_name].env_type));
                    auto idx     = get_gep_index(std::distance(captures.begin(), capture_it));
                    return builder.CreateLoad(builder.CreateGEP(env_ptr, {zero, idx}));
                }
            }
        }

        auto it = globals.find(name);
        if (it != globals.end()) {
            return it->second;
        }

        throw std::invalid_argument("undefined symbol");
    }


    llvm::Value* IRGenerator::get_gep_index(std::size_t idx) {
        return llvm::ConstantInt::get(module.getContext(), llvm::APInt(32, idx, false));
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
