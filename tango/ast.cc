//
//  ast.cpp
//  tango
//
//  Created by 4rch0dia on 27.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include "ast.hh"

namespace tango {

    Block::~Block() {
        for(std::size_t i = 0; i < this->statements.size(); ++i) {
            delete this->statements[i];
            this->statements[i] = nullptr;
        }
    }

    void Block::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

    // -----------------------------------------------------------------------

    void PropertyDecl::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

    // -----------------------------------------------------------------------

    void ParamDecl::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

    // -----------------------------------------------------------------------

    FunctionDecl::~FunctionDecl() {
        for(std::size_t i = 0; i < this->parameters.size(); ++i) {
            delete this->parameters[i];
            this->parameters[i] = nullptr;
        }
        delete this->body;
    }

    void FunctionDecl::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

    // -----------------------------------------------------------------------

    Assignment::~Assignment() {
        delete this->lvalue;
        delete this->rvalue;
    }

    void Assignment::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

    // -----------------------------------------------------------------------

    If::~If() {
        delete this->condition;
        delete this->then_block;
        delete this->else_block;
    }

    void If::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

    // -----------------------------------------------------------------------

    Return::~Return() {
        delete this->value;
    }

    void Return::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

    // -----------------------------------------------------------------------

    BinaryExpr::~BinaryExpr() {
        delete this->left;
        delete this->right;
    }

    void BinaryExpr::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

    // -----------------------------------------------------------------------

    Call::~Call() {
        delete this->callee;
        for(std::size_t i = 0; i < this->arguments.size(); ++i) {
            delete this->arguments[i];
            this->arguments[i] = nullptr;
        }
    }

    void Call::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

    // -----------------------------------------------------------------------

    CallArg::~CallArg() {
        delete this->value;
    }

    void CallArg::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

    // -----------------------------------------------------------------------

    void Identifier::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

    // -----------------------------------------------------------------------

    void IntegerLiteral::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

    // -----------------------------------------------------------------------

    void BooleanLiteral::accept(ASTNodeVisitor& visitor) {
        visitor.visit(*this);
    }

} // namespace tango
