//
//  functiondecl.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include <llvm/IR/Verifier.h>

#include "irgen.hh"


namespace tango {
namespace irgen {

    void IRGenerator::visit(FunctionDecl& node) {
        // Get the LLVM type of the function.
        llvm::FunctionType* fun_type = static_cast<llvm::FunctionType*>(
            node.get_type()->get_llvm_type(module.getContext()));

        // Create the function object.
        auto fun = llvm::Function::Create(
            fun_type, llvm::Function::ExternalLinkage, node.name, &module);
        std::size_t idx = 0;
        for (auto& arg: fun->args()) {
            arg.setName(node.parameters[idx]->name);
            idx += 1;
        }

        // Create a new basic block to start insertion into.
        auto basic_block = llvm::BasicBlock::Create(module.getContext(), "entry", fun);
        builder.SetInsertPoint(basic_block);

        // Create an alloca for the return value.
        this->current_return_alloca = create_alloca(fun, fun_type->getReturnType(), "rv");

        // Store the function parameters in the local symbol table.
        locals.clear();
        for (auto& arg: fun->args()) {
            // Create an alloca for the argument, and store its value.
            auto alloca = create_alloca(fun, arg.getType(), arg.getName());
            builder.CreateStore(&arg, alloca);
            locals[arg.getName()] = alloca;
        }

        // Generate the body of the function.
        node.body->accept(*this);
        builder.CreateRet(builder.CreateLoad(this->current_return_alloca));

        this->current_return_alloca = nullptr;
        this->current_return_type.reset();
        builder.ClearInsertionPoint();
        
        // Validate the generated code, checking for consistency.
        llvm::verifyFunction(*fun);
    }

} // namespace irgen
} // namespace tango
