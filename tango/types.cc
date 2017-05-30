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

    // -----------------------------------------------------------------------

    llvm::Type* IntType::get_llvm_type(llvm::LLVMContext& ctx) const {
        return llvm::Type::getInt64Ty(ctx);
    }

} // namespace tango
