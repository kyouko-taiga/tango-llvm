//
//  ast.hh
//  tango
//
//  Created by 4rch0dia on 27.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "types.hh"


namespace tango {

    // Identifier attributes.
    enum IdentifierMutability {
        im_cst, im_mut,
    };

    // Enumerations of operators.
    enum Operator {
        add, sub, mul, div,
    };

    enum AssignmentOperator {
        ao_cpy, ao_ref, ao_mov,
    };

    struct ASTNodeVisitor;

    /// Base class for all AST nodes.
    struct ASTNode {
        virtual ~ASTNode() {};

        // Has to be implemented in every derived class, or dynamic dispatch
        // wouldn't work.
        virtual void accept(ASTNodeVisitor& visitor) = 0;

        // Following are metadata about AST nodes.
        TypePtr md_type;

        // Following are helpers to create ASTs inline.
        ASTNode* set_type(TypePtr type) {
            this->md_type = type;
            return this;
        }

        TypePtr get_type() const {
            if (this->md_type == nullptr) {
                throw std::invalid_argument("untyped node or expression");
            }
            return this->md_type;
        }
    };

    // AST node for blocks of instructions.
    struct Block: public ASTNode {
        Block(const std::vector<ASTNode*> statements):
            statements(statements) {}

        ~Block();

        void accept(ASTNodeVisitor& visitor);

        std::vector<ASTNode*> statements;
    };

    // AST node for property declarations.
    struct PropertyDecl: public ASTNode {
        PropertyDecl(
                const std::string&   name,
                IdentifierMutability im = im_cst):
            name(name), mutability(im) {}

        void accept(ASTNodeVisitor& visitor);

        std::string          name;
        IdentifierMutability mutability;
    };

    /// AST node for function parameters.
    struct FunctionParam: public ASTNode {
        FunctionParam(
                const std::string&   name,
                IdentifierMutability im = im_cst):
            name(name), mutability(im) {}

        void accept(ASTNodeVisitor& visitor);

        std::string          name;
        IdentifierMutability mutability;
    };

    /// AST node for function declarations.
    struct FunctionDecl: public ASTNode {
        FunctionDecl(
                const std::string&                 name,
                const std::vector<FunctionParam*>& parameters,
                Block*                             body):
            name(name), parameters(parameters), body(body) {}

        ~FunctionDecl();

        void accept(ASTNodeVisitor& visitor);

        std::string                 name;
        std::vector<FunctionParam*> parameters;
        Block*                      body;

        /// List of the symbols the function decl captures by closure.
        std::vector<std::pair<std::string, TypePtr>> md_captures;
    };

    // AST node for assignments.
    struct Assignment: public ASTNode {
        Assignment(
                ASTNode*           lvalue,
                AssignmentOperator op,
                ASTNode*           rvalue):
            lvalue(lvalue), op(op), rvalue(rvalue) {}

        ~Assignment();

        void accept(ASTNodeVisitor& visitor);

        ASTNode*           lvalue;
        AssignmentOperator op;
        ASTNode*           rvalue;
    };

    // AST node for conditional statements.
    struct If: public ASTNode {
        If(
                ASTNode* condition,
                Block*   then_block,
                Block*   else_block):
            condition(condition), then_block(then_block), else_block(else_block) {}

        ~If();

        void accept(ASTNodeVisitor& visitor);

        ASTNode* condition;
        Block*   then_block;
        Block*   else_block;
    };

    /// AST node for return statements.
    struct Return: public ASTNode {
        Return(ASTNode* value): value(value) {}

        ~Return();

        void accept(ASTNodeVisitor& visitor);

        ASTNode* value;
    };

    /// AST node for binary expressions.
    struct BinaryExpr: public ASTNode {
        BinaryExpr(
                ASTNode* left,
                ASTNode* right,
                Operator op):
            left(left), right(right), op(op) {}

        ~BinaryExpr();

        void accept(ASTNodeVisitor& visitor);

        ASTNode* left;
        ASTNode* right;
        Operator op;
    };

    // AST node for call arguments.
    struct CallArg: public ASTNode {
        CallArg(
                const std::string& label,
                AssignmentOperator op,
                ASTNode*           value):
            label(label), op(op), value(value) {}

        ~CallArg();

        void accept(ASTNodeVisitor& visitor);

        std::string        label;
        AssignmentOperator op;
        ASTNode*           value;
    };

    /// AST node for call expressions.
    struct Call: public ASTNode {
        Call(
                ASTNode*                     callee,
                const std::vector<CallArg*>& arguments):
            callee(callee), arguments(arguments) {}

        ~Call();

        void accept(ASTNodeVisitor& visitor);

        ASTNode* callee;
        std::vector<CallArg*> arguments;
    };

    /// AST node for identifiers.
    struct Identifier: public ASTNode {
        Identifier(const std::string& name): name(name) {}

        void accept(ASTNodeVisitor& visitor);

        std::string name;
    };

    /// AST node for integer literals.
    struct IntegerLiteral: public ASTNode {
        IntegerLiteral(int value): value(value) {}

        void accept(ASTNodeVisitor& visitor);

        int value;
    };

    struct ASTNodeVisitor {
        virtual void visit(Block&          node) = 0;
        virtual void visit(PropertyDecl&   node) = 0;
        virtual void visit(FunctionParam&  node) = 0;
        virtual void visit(FunctionDecl&   node) = 0;
        virtual void visit(Assignment&     node) = 0;
        virtual void visit(If&             node) = 0;
        virtual void visit(Return&         node) = 0;
        virtual void visit(BinaryExpr&     node) = 0;
        virtual void visit(Call&           node) = 0;
        virtual void visit(CallArg&        node) = 0;
        virtual void visit(Identifier&     node) = 0;
        virtual void visit(IntegerLiteral& node) = 0;
    };

} // namespace tango
