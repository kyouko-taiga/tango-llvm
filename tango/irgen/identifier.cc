//
//  identifier.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include "irgen.hh"


namespace tango {
namespace irgen {

    void IRGenerator::visit(Identifier& node) {
        // Look for the identifier in the local/global symbol tables.
        auto local_it = locals.find(node.name);
        if (local_it != locals.end()) {
            this->stack.push(builder.CreateLoad(local_it->second, node.name.c_str()));
        } else {
            auto global_it = globals.find(node.name);
            if (global_it != globals.end()) {
                this->stack.push(builder.CreateLoad(global_it->second, node.name.c_str()));
            } else {
                throw std::invalid_argument("undefined symbol");
            }
        }
    }
    
} // namespace irgen
} // namespace tango
