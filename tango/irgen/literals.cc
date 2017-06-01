//
//  literals.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include "irgen.hh"


namespace tango {
namespace irgen {

    void IRGenerator::visit(IntegerLiteral& node) {
        stack.push(llvm::ConstantInt::get(module.getContext(), llvm::APInt(64, node.value, true)));
    }

    void IRGenerator::visit(BooleanLiteral& node) {
        stack.push(llvm::ConstantInt::get(module.getContext(), llvm::APInt(1, node.value, false)));
    }

} // namespace irgen
} // namespace tango
