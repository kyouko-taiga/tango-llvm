//
//  functiondecl.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include <llvm/IR/Verifier.h>

#include "irgen.hh"
#include "tango/captureinfo.hh"


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
        auto ib = gen.builder.GetInsertBlock();
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

        if (ib == nullptr) {
            gen.builder.ClearInsertionPoint();
        } else {
            gen.builder.SetInsertPoint(ib);
        }

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
        // Create the "lifted" type of the function.
        std::vector<llvm::Type*> free_types;
        for (auto val: node.capture_list) {
            free_types.push_back(val.decl->get_type()->get_llvm_type(gen.module.getContext()));

            // NOTE: Escaping closures shouldn't capture local references,
            // as such we should get the referred type of reference types
            // when dealing with them.
        }
        llvm::FunctionType* fun_type = std::static_pointer_cast<FunctionType>(node.get_type())
            ->get_llvm_lifted_type(gen.module.getContext(), free_types);

        // Create the LLVM function prototype.
        auto fun = llvm::Function::Create(
            fun_type, llvm::Function::PrivateLinkage, node.name, &gen.module);
        fun->addFnAttr(llvm::Attribute::NoUnwind);

        // Set the name of the function arguments.
        auto arg_it = fun->arg_begin();
        for (auto val: node.capture_list) {
            arg_it->setName(val.decl->name);
            arg_it++;
        }
        std::size_t idx = 0;
        while (arg_it != fun->arg_end()) {
            arg_it->setName(node.parameters[idx++]->name);
            arg_it++;
        }

        // Generate the function body.
        emit_function_body(node, fun, fun_type, gen);

        // Create a local symbol representing the first-class function object
        auto current_fun = gen.builder.GetInsertBlock()->getParent();
        auto alloca = create_alloca(current_fun, gen.tango_types.closure_t, node.name);

        // Store the function pointer.
        // %0 = getelementptr %closure_t, %closure_t* %<fun_name>, i32 0, i32 0
        // store i8* bitcast (<fun_ptr> @<fun_name> to i8*), i8** %0
        auto zero = gen.get_gep_index(0);
        gen.builder.CreateStore(
            gen.builder.CreateBitCast(fun, gen.tango_types.voidp_t),
            gen.builder.CreateGEP(alloca, {zero, zero}));

        // If the function isn't escaping, it doesn't need an environment.
        // %1 = getelementptr %closure_t, %closure_t* %<fun_name>, i32 0, i32 1
        // store i8* null, i8** %1
        gen.builder.CreateStore(
            llvm::ConstantPointerNull::get(gen.tango_types.voidp_t),
            gen.builder.CreateGEP(alloca, {zero, gen.get_gep_index(1)}));

        // Store the closure info.
        gen.closures[node.name] = ClosureInfo(&node, llvm::PointerType::getUnqual(fun_type));

        // TODO: Handle escaping closures.

        gen.locals.top()[node.name] = alloca;
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
