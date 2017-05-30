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
        auto loc = get_symbol_location(node.name);
        this->stack.push(builder.CreateLoad(loc, node.name.c_str()));
    }
    
} // namespace irgen
} // namespace tango
