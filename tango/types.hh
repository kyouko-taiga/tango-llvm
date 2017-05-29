//
//  type.hh
//  tango
//
//  Created by Dimitri Racordon on 29.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#pragma once

#include <memory>
#include <string>

#include <llvm/IR/Type.h>


namespace Tango {

    struct TypeBase {

        virtual ~TypeBase() {}

        virtual bool is_generic()   const = 0;
        virtual bool is_reference() const = 0;

        virtual llvm::Type* get_llvm_type(llvm::LLVMContext&) const = 0;

    };

    // -----------------------------------------------------------------------

    struct RefType: public TypeBase {

        RefType(const TypeBase& rt): referred_type(rt) {}

        bool is_generic()   const { return this->referred_type.is_generic(); }
        bool is_reference() const { return true; }

        llvm::Type* get_llvm_type(llvm::LLVMContext&) const;

        const TypeBase& referred_type;

    };

    // -----------------------------------------------------------------------

    struct NominalType: public TypeBase {

        NominalType(const std::string& name): name(name) {}

        bool is_generic()   const { return false; }
        bool is_reference() const { return false; }

        const std::string name;

    };

    // -----------------------------------------------------------------------

    struct IntType: public NominalType {

        IntType(): NominalType("Int") {}

        llvm::Type* get_llvm_type(llvm::LLVMContext&) const;

        static const IntType& get() {
            static IntType instance;
            return instance;
        }

    };

    static IntType Int;

} // namespace Tango
