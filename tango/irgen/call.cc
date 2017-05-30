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
        // Get the callee from the symbol table of the module.
        auto& callee_name = static_cast<Identifier*>(node.callee)->name;
        auto callee       = module.getFunction(callee_name);

        if (callee == nullptr) {
            throw std::invalid_argument("unknown function reference");
        }
        if (callee->arg_size() != node.arguments.size()) {
            throw std::invalid_argument("invalid number of arguments");
        }

        std::vector<llvm::Value*> args;
        for (auto call_arg: node.arguments) {
            call_arg->value->accept(*this);
            args.push_back(this->stack.top());
            this->stack.pop();
        }

        this->stack.push(builder.CreateCall(callee, args));
    }
    
} // namespace irgen
} // namespace tango
