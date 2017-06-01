//
//  types.cpp
//  tango
//
//  Created by 4rch0dia on 29.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include <llvm/IR/DerivedTypes.h>

#include "types.hh"


namespace tango {

    llvm::Type* RefType::get_llvm_type(llvm::LLVMContext& ctx) const {
        return llvm::PointerType::getUnqual(this->referred_type->get_llvm_type(ctx));
    }

    // -----------------------------------------------------------------------

    llvm::Type* FunctionType::get_llvm_type(llvm::LLVMContext& ctx) const {
        std::vector<llvm::Type*> arg_types;
        for (auto ty: this->domain) {
            arg_types.push_back(ty->get_llvm_type(ctx));
        }
        return llvm::FunctionType::get(this->codomain->get_llvm_type(ctx), arg_types, false);
    }

    llvm::FunctionType* FunctionType::get_llvm_lifted_type(
        llvm::LLVMContext& ctx, llvm::StructType* closure_type) const
    {
        std::vector<llvm::Type*> arg_types;
        arg_types.push_back(llvm::PointerType::getUnqual(closure_type));
        for (auto ty: this->domain) {
            arg_types.push_back(ty->get_llvm_type(ctx));
        }
        return llvm::FunctionType::get(this->codomain->get_llvm_type(ctx), arg_types, false);
    }

    // -----------------------------------------------------------------------

    llvm::Type* IntType::get_llvm_type(llvm::LLVMContext& ctx) const {
        return llvm::Type::getInt64Ty(ctx);
    }

    // -----------------------------------------------------------------------

    TangoLLVMTypes::TangoLLVMTypes(llvm::LLVMContext& ctx) {
        // void* type.
        voidp_t = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(ctx));

        // Integer type.
        // Note we hardcoded it on 64 bits!
        integer_t = llvm::Type::getInt64Ty(ctx);

        std::vector<llvm::Type*> members;

        // Type for function closures.
        closure_t = llvm::StructType::create(ctx, "closure_t");
        members.clear();
        members.push_back(voidp_t);
        members.push_back(voidp_t);
        closure_t->setBody(members);
    }

} // namespace tango
