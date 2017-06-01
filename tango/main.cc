//
//  main.cpp
//  tango
//
//  Created by 4rch0dia on 27.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include <iostream>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

// Optimizations.
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/Scalar.h>

#include "ast.hh"
#include "captureinfo.hh"
#include "types.hh"
#include "irgen/irgen.hh"


int main(int argc, char* argv[]) {
    using namespace tango;

    // Create a Tango program.
    TypePtr Int     = tango::IntType::get();
    TypePtr Int_p   = tango::RefType::get(Int);
    TypePtr Int2Int = tango::FunctionType::get({Int}, {"x"}, {Int});
    TypePtr Bool    = tango::BoolType::get();

    // cst x: Int
    // if true {
    //     x = 5
    // } else {
    //     x = 10
    // }
    auto ast = Block({
        new PropertyDecl("x"),
        new If(
            new BooleanLiteral(true),
            new Block({
                new Assignment(new Identifier("x"), tango::ao_cpy, new IntegerLiteral(5))
            }),
            new Block({
                new Assignment(new Identifier("x"), tango::ao_cpy, new IntegerLiteral(10))
            })),
    });

    ast.statements[0]->set_type(Int);

    auto if_stmt = static_cast<If*>(ast.statements[1]);
    if_stmt->condition->set_type(Bool);

    auto x_assign = static_cast<Assignment*>(if_stmt->then_block->statements[0]);
    x_assign->lvalue->set_type(Int);
    x_assign->rvalue->set_type(Int);

    x_assign = static_cast<Assignment*>(if_stmt->else_block->statements[0]);
    x_assign->lvalue->set_type(Int);
    x_assign->rvalue->set_type(Int);

//    // def f(cst x: Int) -> Int {
//    //     def g(cst y: Int) -> Int {
//    //         return x
//    //     }
//    //     return g(y = 0)
//    // }
//    // f(x = 42)
//    auto ast = Block({
//        new FunctionDecl("f", {new ParamDecl("x")}, new Block({
//            new FunctionDecl("g", {new ParamDecl("y")}, new Block({
//                new Return(new Identifier("x")),
//            })),
//            new Return(
//                new Call(new Identifier("g"), {new CallArg("y", tango::ao_cpy, new IntegerLiteral(0))})),
//        })),
//        new Call(new Identifier("f"), {new CallArg("x", tango::ao_cpy, new IntegerLiteral(42))})
//    });
//
//    auto f_decl = static_cast<FunctionDecl*>(ast.statements[0]);
//    f_decl->set_type(Int2Int);
//    f_decl->parameters[0]->set_type(Int);
//
//    auto g_decl = static_cast<FunctionDecl*>(f_decl->body->statements[0]);
//    g_decl->set_type(Int2Int);
//    g_decl->parameters[0]->set_type(Int);
//    g_decl->capture_list.push_back(CapturedValue(f_decl->parameters[0]));
//    static_cast<Return*>(g_decl->body->statements[0])->value->set_type(Int);
//
//    auto f_return = static_cast<Return*>(f_decl->body->statements[1]);
//    f_return->value->set_type(Int);
//
//    auto call_to_g = static_cast<Call*>(f_return->value);
//    call_to_g->set_type(Int);
//    call_to_g->callee->set_type(Int2Int);
//    call_to_g->arguments[0]->value->set_type(Int);
//
//    auto call_to_f = static_cast<Call*>(ast.statements[1]);
//    call_to_f->set_type(Int);
//    call_to_f->callee->set_type(Int2Int);
//    call_to_f->arguments[0]->value->set_type(Int);

//    // def f(cst x: Int) -> Int {
//    //     cst y = x
//    //     return y
//    // }
//    auto ast = Block({
//        (new FunctionDecl("f", {new ParamDecl("x")}, new Block({
//            (new PropertyDecl("y"))->set_type(Int),
//            new Assignment(
//                (new Identifier("y"))->set_type(Int),
//                tango::ao_cpy,
//                (new Identifier("x"))->set_type(Int)),
//            new Return((new Identifier("y"))->set_type(Int)),
//        })))->set_type(Int2Int),
//    });

//    // mut z: Int
//    // def f(cst x: Int) -> Int {
//    //     mut y &- z
//    //     y = x
//    //     return y
//    // }
//    auto ast = Block({
//        (new PropertyDecl("z"))->set_type(Int),
//        (new FunctionDecl("f", {new ParamDecl("x")}, new Block({
//            (new PropertyDecl("y"))->set_type(Int_p),
//            new Assignment(
//                (new Identifier("y"))->set_type(Int_p),
//                tango::ao_ref,
//                (new Identifier("z"))->set_type(Int)),
//            new Assignment(
//                (new Identifier("y"))->set_type(Int_p),
//                tango::ao_cpy,
//                (new Identifier("x"))->set_type(Int)),
//            new Return((new Identifier("y"))->set_type(Int_p)),
//        })))->set_type(Int2Int),
//    });

    // Create the module, which holds all the code.
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder(context);
    llvm::Module module("tango module", context);

    // Generate the IR code of the module.
    auto ir_generator = tango::irgen::IRGenerator(module, builder);
    ir_generator.add_main_function();
    ast.accept(ir_generator);
    ir_generator.finish_main_function();

    // Create an optimization pass manager.
    auto pass_manager = llvm::make_unique<llvm::legacy::PassManager>();
    pass_manager->add(llvm::createPromoteMemoryToRegisterPass());
//    pass_manager->add(llvm::createInstructionCombiningPass());
//    pass_manager->add(llvm::createReassociatePass());
    pass_manager->run(module);

    module.dump();

    return 0;
}
