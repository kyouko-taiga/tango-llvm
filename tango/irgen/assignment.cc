//
//  assignment.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include "irgen.hh"


namespace tango {
namespace irgen {

    void emit_copy_assignment(Assignment& node, llvm::Value* var_loc, IRGenerator& gen) {
        // Generate the IR code for the rvalue.
        node.rvalue->accept(gen);
        auto val = gen.stack.top();
        gen.stack.pop();

        // Dereference var and/or val if they're references.
        if (node.lvalue->md_type->is_reference()) {
            var_loc = gen.builder.CreateLoad(var_loc);
        }
        if (node.rvalue->md_type->is_reference()) {
            val = gen.builder.CreateLoad(val);
        }

        // Create a store instruction.
        gen.builder.CreateStore(val, var_loc);
    }


    void emit_reference_assignment(Assignment& node, llvm::Value* var_loc, IRGenerator& gen) {
        // Make sure the rvalue is an identifier.
        auto rvalue = dynamic_cast<Identifier*>(node.rvalue);
        if (rvalue == nullptr) {
            throw std::invalid_argument("reference assignement to non-identifier rvalue");
        }

        // Search for the identifier's location to create a store instruction
        // pointing at it.
        auto ref_loc = gen.get_symbol_location(rvalue->name);
        gen.builder.CreateStore(ref_loc, var_loc);
    }


    void IRGenerator::visit(Assignment& node) {
        // If we're not generating the body of a function, we should insert
        // the statement in the main function.
        if (builder.GetInsertBlock() == nullptr) {
            move_to_main_function();
        }

        // Until we implement select and subscript expressions, we can expect
        // the lvalue to always be an identifier.
        auto lvalue = dynamic_cast<Identifier*>(node.lvalue);
        if (lvalue == nullptr) {
            throw std::invalid_argument("invalid lvalue for assignment");
        }

        // Retrieve the variable from either locals, or globals.
        auto var_loc = get_symbol_location(lvalue->name);

        // Emit the assignment instruction.
        switch (node.op) {
            case tango::ao_cpy:
                emit_copy_assignment(node, var_loc, *this);
                break;
            case tango::ao_ref:
                emit_reference_assignment(node, var_loc, *this);
                break;
            default:
                // TODO
                assert(false);
        }
    }
    
} // namespace irgen
} // namespace tango
