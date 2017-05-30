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

    void emit_function_body(
        FunctionDecl&       node,
        llvm::Function*     fun,
        llvm::FunctionType* fun_type,
        IRGenerator&        gen)
    {
        // Create a new basic block to start insertion into.
        auto bb = llvm::BasicBlock::Create(gen.module.getContext(), "entry", fun);
        gen.builder.SetInsertPoint(bb);

        // Store the alloca and (Tango) return type of the return value.
        gen.return_alloca.push(create_alloca(fun, fun_type->getReturnType(), "rv"));
        gen.return_type.push(std::static_pointer_cast<FunctionType>(node.get_type())->codomain);

        // Store the function parameters in its local symbol table.
        IRGenerator::LocalSymbolTable fun_locals;
        for (auto& arg: fun->args()) {
            // Create an alloca for the argument, and store its value.
            auto alloca = create_alloca(fun, arg.getType(), arg.getName());
            gen.builder.CreateStore(&arg, alloca);
            fun_locals[arg.getName()] = alloca;
        }
        gen.locals.push(std::move(fun_locals));

        // Generate the body of the function.
        node.body->accept(gen);
        gen.builder.CreateRet(gen.builder.CreateLoad(gen.return_alloca.top()));

        gen.return_alloca.pop();
        gen.return_type.pop();
        gen.locals.pop();
        gen.builder.ClearInsertionPoint();

        // Validate the generated code, checking for consistency.
        llvm::verifyFunction(*fun);
    }


    void emit_global_function(FunctionDecl& node, IRGenerator& gen) {
        // Global function don't need to be lifted, as they can only
        // capture other global symbols.
        llvm::FunctionType* fun_type = static_cast<llvm::FunctionType*>(
            node.get_type()->get_llvm_type(gen.module.getContext()));

        // Create the LLVM function prototype.
        auto fun = llvm::Function::Create(
            fun_type, llvm::Function::ExternalLinkage, node.name, &gen.module);
        fun->addFnAttr(llvm::Attribute::NoUnwind);

        // Set the name of the function arguments.
        std::size_t idx = 0;
        for (auto& arg: fun->args()) {
            arg.setName(node.parameters[idx++]->name);
        }

        // Generate the function body.
        emit_function_body(node, fun, fun_type, gen);
    }


    void emit_nested_function(FunctionDecl& node, IRGenerator& gen) {
        // Generate the LLVM type of the function.
        llvm::FunctionType* fun_type;
        auto& ctx = gen.module.getContext();

        // If the function doesn't have any free variable, we can create
        // a "normal" function type.
        if (node.md_captures.empty()) {
            fun_type = static_cast<llvm::FunctionType*>(node.get_type()->get_llvm_type(ctx));
        } else {
            // Create a StructType to store the function's environment.
            auto env_type = llvm::StructType::create(ctx, node.name + "_env_t");
            std::vector<llvm::Type*> members;
            for (auto capture: node.md_captures) {
                // Closures capture local references by copy, so we
                // store the referred type in the environment rather
                // than the pointer to.
                auto tango_type = capture.second->is_reference()
                    ? std::static_pointer_cast<RefType>(capture.second)->referred_type
                    : capture.second;
                members.push_back(tango_type->get_llvm_type(ctx));
            }
            env_type->setBody(members);

            // Create a "lifted" function type.
            fun_type = std::static_pointer_cast<FunctionType>(node.get_type())
                ->get_llvm_lifted_type(ctx, env_type);
        }

        // Create the function object.
        auto fun = llvm::Function::Create(
            fun_type, llvm::Function::ExternalLinkage, node.name, &gen.module);
        fun->addFnAttr(llvm::Attribute::NoUnwind);

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

        // Generate the function body.
        emit_function_body(node, fun, fun_type, gen);
    }


    void IRGenerator::visit(FunctionDecl& node) {
        // If we're not generating the body of a function, we're looking at a
        // global function.
        auto insert_block = builder.GetInsertBlock();
        if (insert_block == nullptr) {
            emit_global_function(node, *this);
        } else {
            emit_nested_function(node, *this);
        }
    }

} // namespace irgen
} // namespace tango
