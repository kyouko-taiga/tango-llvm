//
//  ast.cpp
//  tango
//
//  Created by 4rch0dia on 27.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include "ast.hh"
#include "types.hh"
#include "json/json.hpp"


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

    // -----------------------------------------------------------------------

    Block*        parse_block     (nlohmann::json& data);
    PropertyDecl* parse_prop_decl (nlohmann::json& data);
    ParamDecl*    parse_param_decl(nlohmann::json& data);
    FunctionDecl* parse_fun_decl  (nlohmann::json& data);
    Assignment*   parse_assignment(nlohmann::json& data);
    If*           parse_if        (nlohmann::json& data);
    Return*       parse_return    (nlohmann::json& data);
    Call*         parse_call      (nlohmann::json& data);
    CallArg*      parse_call_arg  (nlohmann::json& data);
    Identifier*   parse_identifier(nlohmann::json& data);
    ASTNode*      parse_literal   (nlohmann::json& data);

    Block* parse_block(nlohmann::json& data) {
        std::vector<ASTNode*> statements;

        for (nlohmann::json::iterator it = data.begin(); it != data.end(); ++it) {
            if (it.key() == "PropertyDecl") {
                statements.push_back(parse_prop_decl(it.value()));
            } else if (it.key() == "FunctionParameter") {
                statements.push_back(parse_param_decl(it.value()));
            } else if (it.key() == "FunctionDecl") {
                statements.push_back(parse_fun_decl(it.value()));
            } else if (it.key() == "Assignment") {
                statements.push_back(parse_assignment(it.value()));
            } else if (it.key() == "If") {
                statements.push_back(parse_if(it.value()));
            } else if (it.key() == "Return") {
                statements.push_back(parse_return(it.value()));
            } else if (it.key() == "Call") {
                statements.push_back(parse_call(it.value()));
            } else if (it.key() == "CallArgument") {
                statements.push_back(parse_call_arg(it.value()));
            } else if (it.key() == "Identifier") {
                statements.push_back(parse_identifier(it.value()));
            } else if (it.key() == "Literal") {
                statements.push_back(parse_literal(it.value()));
            } else {
                assert(false);
            }
        }

        return new Block({});
    }

    PropertyDecl* parse_prop_decl(nlohmann::json& data) {
        PropertyDecl* ret;
        if (data["mutability"] == "cst") {
            ret = new PropertyDecl(data["name"], im_cst);
        } else if (data["mutability"] == "mut") {
            ret = new PropertyDecl(data["name"], im_mut);
        } else {
            assert(false);
        }

        // TODO: Parse types property.
        ret->set_type(IntType::get());
        return ret;
    }

    ParamDecl* parse_param_decl(nlohmann::json& data) {
        ParamDecl* ret;
        if (data["mutability"] == "cst") {
            ret = new ParamDecl(data["name"], im_cst);
        } else if (data["mutability"] == "mut") {
            ret = new ParamDecl(data["name"], im_mut);
        } else {
            assert(false);
        }

        // TODO: Parse types property.
        ret->set_type(IntType::get());
        return ret;
    }

    FunctionDecl* parse_fun_decl(nlohmann::json& data) {
        auto ret = new FunctionDecl(
            data["name"],
            {parse_param_decl(data["parameter"]["FunctionParameter"])},
            parse_block(data["body"]["Block"]));

        // TODO: Parse types property.
        ret->set_type(FunctionType::get(
            {IntType::get()},
            {data["parameter"]["FunctionParameter"]["name"]},
            IntType::get()));
        return ret;
    }

    std::unique_ptr<ASTNode> read_ast(std::ifstream& ifs) {
        nlohmann::json ast_data;
        ifs >> ast_data;

        auto module = ast_data["ModuleDecl"];
        return std::unique_ptr<tango::ASTNode>(parse_block(module["body"]["Block"]));
    }

} // namespace tango
