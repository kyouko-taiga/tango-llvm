//
//  block.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include "irgen.hh"


namespace tango {
namespace irgen {

    void IRGenerator::visit(Block& node) {
        for (auto statement: node.statements) {
            statement->accept(*this);
        }
    }

} // namespace irgen
} // namespace tango
