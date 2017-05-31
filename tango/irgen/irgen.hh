//
//  irgen.hh
//  tango
//
//  Created by Dimitri Racordon on 30.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#pragma once

#include <memory>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <llvm/IR/IRBuilder.h>

#include "tango/ast.hh"


namespace llvm {

    class AllocaInst;
    class Function;
    class GlobalVariable;
    class Module;
    class Type;
    class Value;

} // namespace llvm


namespace tango {

    struct TypeBase;
    typedef std::shared_ptr<TypeBase> TypePtr;

namespace irgen {

    /// Struct that stores a function object and its capture list.
    struct ClosureInfo {
        ClosureInfo(FunctionDecl* decl, llvm::Type* pointer_type)
            : decl(decl), pointer_type(pointer_type) {}
        ClosureInfo(const ClosureInfo& other)
            : decl(other.decl), pointer_type(other.pointer_type) {};
        ClosureInfo()
            : decl(nullptr), pointer_type(nullptr) {}

        FunctionDecl* decl;
        llvm::Type*   pointer_type;
    };

    struct IRGenerator: public ASTNodeVisitor {
        typedef std::set<std::string>                                  LocalCaptures;
        typedef std::unordered_map<std::string, llvm::AllocaInst*>     LocalSymbolTable;
        typedef std::unordered_map<std::string, llvm::GlobalVariable*> GlobalSymbolTable;
        typedef std::unordered_map<std::string, ClosureInfo>           ClosureInfoTable;

        IRGenerator(llvm::Module& mod, llvm::IRBuilder<>& irb);
        // IRGenerator(const IRGenerator&) = delete;

        void visit(Block&);
        void visit(PropertyDecl&);
        void visit(FunctionDecl&);
        void visit(Assignment&);
        void visit(If&);
        void visit(Return&);
        void visit(Call&);
        void visit(Identifier&);
        void visit(IntegerLiteral&);

        void visit(ParamDecl&)  {}
        void visit(BinaryExpr&) {}
        void visit(CallArg&)    {}

        /// Adds a main function to the module under generation.
        void add_main_function();

        /// Adds a return value to the main function.
        void finish_main_function(llvm::Value* exit_status = nullptr);

        /// Moves the insertion point of the builder to the main function.
        void move_to_main_function();

        /// Returns the location of a symbol from the local or global table.
        llvm::Value* get_symbol_location(const std::string& name);

        // Returns an LLVM value suitable for GEP indices.
        llvm::Value* get_gep_index(int idx);

        /// A reference to the LLVM module being generated.
        llvm::Module& module;

        /// The main LLVM IR builder.
        llvm::IRBuilder<> builder;

        /// A stack that lets us accumulate the LLVM values of expressions,
        /// before they are consumed by a statement.
        ///
        /// The stack should be emptied every time after the IR of a
        /// particular statement has been generated.
        std::stack<llvm::Value*> stack;

        /// A stack of maps of local symbols.
        ///
        /// It's a stack so that we can handle nested function definitions.
        std::stack<LocalSymbolTable> locals;

        /// A stack of sets of captured symbols, that are used to determine if
        /// a local symbol corresponds to a pointer to a captured value.
        ///
        /// It's a stack so that we can handle nested function definitions.
        std::stack<LocalCaptures> local_captures;

        /// A map of global symbols.
        GlobalSymbolTable globals;

        /// A map of the ClosureInfo objects.
        ClosureInfoTable closures;

        /// A stack of pointers to the alloca that represent the return space
        /// of the function declaration being visited.
        ///
        /// It's a stack so that we can handle nested function definitions.
        std::stack<llvm::AllocaInst*> return_alloca;
        
        /// A stack of Tango types that represent the return type of the
        /// function declaration being visited.
        ///
        /// It's a stack so that we can handle nested function definitions.
        std::stack<TypePtr> return_type;

        /// A store for the Tango types definitions.
        TangoLLVMTypes tango_types;
    };

    /// Create an alloca instruction in the enty block of the function.
    llvm::AllocaInst* create_alloca(
        llvm::Function*    function,
        llvm::Type*        type,
        const std::string& name);

} // namespace irgen
} // namespace tango
