//
//  if.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include "irgen.hh"


namespace tango {
namespace irgen {

    void IRGenerator::visit(If& node) {
        // If we're not generating the body of a function, we should insert
        // the statement in the main function.
        if (builder.GetInsertBlock() == nullptr) {
            move_to_main_function();
        }

        // Generate the IR code of the condition.
        node.condition->accept(*this);
        auto condition = stack.top();
        stack.pop();

        auto fun = builder.GetInsertBlock()->getParent();

        // Create blocks for the then and else clauses.
        auto then_block = llvm::BasicBlock::Create(module.getContext(), "then", fun);
        auto else_block = llvm::BasicBlock::Create(module.getContext(), "else");
        auto cont_block = llvm::BasicBlock::Create(module.getContext(), "cont");

        // Create the branch statement.
        builder.CreateCondBr(condition, then_block, else_block);

        // Generate the IR code for the then clause.
        builder.SetInsertPoint(then_block);
        node.then_block->accept(*this);
        builder.CreateBr(cont_block);

        // Generate the IR code for the else clause.
        fun->getBasicBlockList().push_back(else_block);
        builder.SetInsertPoint(else_block);
        node.else_block->accept(*this);
        builder.CreateBr(cont_block);

        // Generate the IR for the continuation block.
        fun->getBasicBlockList().push_back(cont_block);
    }

} // namespace irgen
} // namespace tango
