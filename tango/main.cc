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
#include "types.hh"
#include "irgen/irgen.hh"


int main(int argc, char* argv[]) {
    using namespace tango;

    // Create a Tango program.
    TypePtr Int     = tango::IntType::get();
    TypePtr Int_p   = tango::RefType::get(Int);
    TypePtr Int2Int = tango::FunctionType::get({Int}, {"x"}, {Int});

    // mut x
    // x = 0
    auto ast = Block({
        (new PropertyDecl("x"))->set_type(Int),
        new Assignment(
            (new Identifier("x"))->set_type(Int),
            tango::ao_cpy,
            (new IntegerLiteral(10))->set_type(Int)),
    });

//    // def f(cst x: Int) -> Int {
//    //     cst y = x
//    //     return y
//    // }
//    auto ast = Block({
//        (new FunctionDecl("f", {new FunctionParam("x")}, new Block({
//            (new PropertyDecl("y"))->set_type(Int),
//            new Assignment(
//                (new Identifier("y"))->set_type(Int),
//                Tango::ao_cpy,
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
//        (new FunctionDecl("f", {new FunctionParam("x")}, new Block({
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
    pass_manager->run(module);

    module.dump();

    return 0;
}
