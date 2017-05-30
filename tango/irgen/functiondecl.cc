//
//  functiondecl.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include <llvm/IR/Verifier.h>

#include "irgen.hh"
// #include "tango/types.hh"


namespace tango {
namespace irgen {

    void IRGenerator::visit(FunctionDecl& node) {
        // Generate the LLVM type of the function.
        llvm::FunctionType* fun_type;

        auto& ctx = module.getContext();

        // If the function doesn't have any free variable, we can create
        // a "normal" function type.
        if (node.md_captures.empty()) {
            fun_type = static_cast<llvm::FunctionType*>(node.get_type()->get_llvm_type(ctx));
        } else {
            // Create a StructType to store the function's environment.
            auto env_type = llvm::StructType::create(ctx, node.name + "_env_t");
            std::vector<llvm::Type*> members;
            for (auto capture: node.md_captures) {
                auto tango_type = capture.second->is_reference()
                    ? std::dynamic_pointer_cast<RefType>(capture.second)->referred_type
                    : capture.second;
                members.push_back(tango_type->get_llvm_type(ctx));
            }
            env_type->setBody(members);

            fun_type = std::dynamic_pointer_cast<FunctionType>(node.get_type())
                ->get_llvm_lifted_type(module.getContext(), env_type);
        }

        // Create the function object.
        auto fun = llvm::Function::Create(
            fun_type, llvm::Function::ExternalLinkage, node.name, &module);

        // Set the name of the function arguments.
        std::size_t idx = 0;
        auto arg_it = fun->arg_begin();
        if (!node.md_captures.empty()) {
            arg_it->setName("__env");
            arg_it++;
        }
        while (arg_it != fun->arg_end()) {
            arg_it->setName(node.parameters[idx]->name);
            arg_it++;
            idx++;
        }

        // Create a new basic block to start insertion into.
        auto basic_block = llvm::BasicBlock::Create(module.getContext(), "entry", fun);
        builder.SetInsertPoint(basic_block);

        // Store the alloca and return type of the return value.
        return_alloca.push(create_alloca(fun, fun_type->getReturnType(), "rv"));
        return_type.push(std::dynamic_pointer_cast<FunctionType>(node.get_type())->codomain);

        // Store the function parameters in the local symbol table.
        // std::map<std::string, llvm::AllocaInst*> fun_locals;
        locals.clear();
        for (auto& arg: fun->args()) {
            // Create an alloca for the argument, and store its value.
            auto alloca = create_alloca(fun, arg.getType(), arg.getName());
            builder.CreateStore(&arg, alloca);
            locals[arg.getName()] = alloca;
        }

        // Generate the body of the function.
        node.body->accept(*this);
        builder.CreateRet(builder.CreateLoad(return_alloca.top()));

        return_alloca.pop();
        return_type.pop();
        builder.ClearInsertionPoint();
        
        // Validate the generated code, checking for consistency.
        llvm::verifyFunction(*fun);
    }

} // namespace irgen
} // namespace tango
