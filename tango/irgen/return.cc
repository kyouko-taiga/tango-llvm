//
//  return.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include "irgen.hh"


namespace tango {
namespace irgen {

    void IRGenerator::visit(Return& node) {
        // Make sure we're generating a function's body.
        if (builder.GetInsertBlock() == nullptr) {
            throw std::invalid_argument("return statement outside of a function body");
        }

        // Generate the IR code of the return value.
        node.value->accept(*this);
        auto rv = this->stack.top();
        this->stack.pop();

        // Dereference rv if it's a reference.
        if (node.value->get_type()->is_reference()) {
            rv = builder.CreateLoad(rv);
        }

        // Store it on the return alloca.
        builder.CreateStore(rv, this->current_return_alloca);
    }

} // namespace irgen
} // namespace tango
