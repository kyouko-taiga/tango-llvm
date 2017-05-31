//
//  call.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include "irgen.hh"


namespace tango {
namespace irgen {

    void IRGenerator::visit(Call& node) {
        // If we're not generating the body of a function, we should insert
        // the statement in the main function.
        if (builder.GetInsertBlock() == nullptr) {
            move_to_main_function();
        }

        // Get the callee object.
        auto& callee_name = static_cast<Identifier*>(node.callee)->name;

        // TODO: Handle non-identifier callees.

        // If the callee name isn't in the local symbol table, the referred
        // function is global, and we can get it from the symbol table of the
        // module.
        if (locals.empty() or (locals.top().find(callee_name) == locals.top().end())) {
            auto callee = module.getFunction(callee_name);

            // Set the function's arguments.
            std::vector<llvm::Value*> args;
            for (auto arg: node.arguments) {
                arg->value->accept(*this);
                args.push_back(stack.top());
                stack.pop();
            }

            // Create the function call.
            stack.push(builder.CreateCall(callee, args));
        } else {
            auto closure_info = closures[callee_name];
            auto closure_loc  = get_symbol_location(callee_name);
            auto zero         = get_gep_index(0);

            // Dereference the function pointer.
            auto raw_ptr = builder.CreateLoad(builder.CreateGEP(closure_loc, {zero, zero}));
            auto callee = builder.CreateBitCast(raw_ptr, closure_info.pointer_type);
            callee->getType()->dump();

            // Set the function's captured values.
            std::vector<llvm::Value*> args;
            for (auto cval: closure_info.decl->capture_list) {
                // Get the value of the captured value.
                auto val = get_symbol_location(cval.decl->name);
                args.push_back(builder.CreateLoad(val));
            }

            // Set the function's other arguments.
            for (auto arg: node.arguments) {
                arg->value->accept(*this);
                args.push_back(stack.top());
                stack.pop();
            }

            // Create the function call.
            stack.push(builder.CreateCall(callee, args));
        }

        // TODO: Handle escaping closures.
    }
    
} // namespace irgen
} // namespace tango
