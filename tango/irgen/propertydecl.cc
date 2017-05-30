//
//  propertydecl.cc
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include "irgen.hh"


namespace tango {
namespace irgen {

    void IRGenerator::visit(PropertyDecl& node) {
        // Get the LLVM type of the property.
        auto prop_type = node.get_type()->get_llvm_type(module.getContext());

        // Check whether the variable is a reference, in which case we create
        // a pointer type to the variable's referred type.
        if (node.get_type()->is_reference()) {
            prop_type = static_cast<llvm::PointerType*>(prop_type);
        }

        // Check whether we should declare a local or global variable. If
        // we're not generating the body of a function, we're looking at a
        // global variable.
        auto insert_block = builder.GetInsertBlock();
        if (insert_block == nullptr) {
            // Create a global variable.
            module.getOrInsertGlobal(node.name, prop_type);
            auto global_var = module.getNamedGlobal(node.name);
            global_var->setLinkage(llvm::GlobalVariable::CommonLinkage);

            // Store the variable in the global symbol table.
            globals[node.name] = global_var;
        } else {
            // Get the LLVM function under declaration.
            auto fun = insert_block->getParent();

            // Create an alloca for the variable, and store it as a local
            // symbol table.
            locals.top()[node.name] = create_alloca(fun, prop_type, node.name);
        }

        // TODO: Handle garbage collected variables.
    }

} // namespace irgen
} // namespace tango
