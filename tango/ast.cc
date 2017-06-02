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

    ASTNode* parse_node(nlohmann::json& data) {
        std::cout << data << std::endl;
        nlohmann::json::iterator it = data.find("PropertyDecl");
        if (it != data.end()) { return parse_prop_decl(it.value()); }
        it = data.find("FunctionParameter");
        if (it != data.end()) { return parse_param_decl(it.value()); }
        it = data.find("FunctionDecl");
        if (it != data.end()) { return parse_fun_decl(it.value()); }
        it = data.find("Assignment");
        if (it != data.end()) { return parse_assignment(it.value()); }
        it = data.find("If");
        if (it != data.end()) { return parse_if(it.value()); }
        it = data.find("Return");
        if (it != data.end()) { return parse_return(it.value()); }
        it = data.find("Call");
        if (it != data.end()) { return parse_call(it.value()); }
        it = data.find("CallArgument");
        if (it != data.end()) { return parse_call_arg(it.value()); }
        it = data.find("Identifier");
        if (it != data.end()) { return parse_identifier(it.value()); }
        it = data.find("Literal");
        if (it != data.end()) { return parse_literal(it.value()); }

        assert(false);
    }

    Block* parse_block(nlohmann::json& data) {
        std::vector<ASTNode*> statements;
        for (auto stmt: data.at("statements")) {
            statements.push_back(parse_node(stmt));
        }
        return new Block(statements);
    }

    PropertyDecl* parse_prop_decl(nlohmann::json& data) {
        PropertyDecl* ret;
        if (data.at("mutability") == "cst") {
            ret = new PropertyDecl(data.at("name"), im_cst);
        } else if (data.at("mutability") == "mut") {
            ret = new PropertyDecl(data.at("name"), im_mut);
        } else {
            assert(false);
        }

        // TODO: Parse types property.
        ret->set_type(IntType::get());
        return ret;
    }

    ParamDecl* parse_param_decl(nlohmann::json& data) {
        ParamDecl* ret;
        if (data.at("mutability") == "cst") {
            ret = new ParamDecl(data.at("name"), im_cst);
        } else if (data.at("mutability") == "mut") {
            ret = new ParamDecl(data.at("name"), im_mut);
        } else {
            assert(false);
        }

        // TODO: Parse types property.
        ret->set_type(IntType::get());
        return ret;
    }

    FunctionDecl* parse_fun_decl(nlohmann::json& data) {
        auto ret = new FunctionDecl(
            data.at("name"),
            {parse_param_decl(data.at("parameter").at("FunctionParameter"))},
            parse_block(data.at("body").at("Block")));

        // TODO: Parse types property.
        ret->set_type(FunctionType::get(
            {IntType::get()},
            {data.at("parameter").at("FunctionParameter").at("name")},
            IntType::get()));
        return ret;
    }

    Assignment* parse_assignment(nlohmann::json& data) {
        auto lvalue = parse_node(data.at("lvalue"));
        auto rvalue = parse_node(data.at("rvalue"));
        AssignmentOperator op;

        if (data.at("operator") == "=") {
            op = ao_cpy;
        } else if (data.at("operator") == "&-") {
            op = ao_ref;
        } else if (data.at("operator") == "<-") {
            op = ao_mov;
        } else {
            assert(false);
        }

        return new Assignment(lvalue, op, rvalue);
    }

    If* parse_if(nlohmann::json& data) {
        // Parse conditions properly.
        auto condition = new BooleanLiteral(true);
        condition->set_type(BoolType::get());

        return new If(condition, parse_block(data.at("body").at("Block")), new Block({}));
    }

    Return* parse_return(nlohmann::json& data) {
        return new Return(parse_node(data.at("value")));
    }

    Call* parse_call(nlohmann::json& data) {
        auto callee = parse_node(data.at("callee"));
        std::vector<CallArg*> arguments;
        for (auto arg: data.at("arguments")) {
            arguments.push_back(parse_call_arg(arg.at("CallArgument")));
        }

        auto ret = new Call(callee, arguments);
        // TODO: Parse types property.
        ret->set_type(IntType::get());
        return ret;
    }

    CallArg* parse_call_arg(nlohmann::json& data) {
        auto value = parse_node(data.at("value"));
        AssignmentOperator op;

        if (data.at("operator") == "=") {
            op = ao_cpy;
        } else if (data.at("operator") == "&-") {
            op = ao_ref;
        } else if (data.at("operator") == "<-") {
            op = ao_mov;
        } else {
            assert(false);
        }

        return new CallArg(data.at("label"), op, value);
    }

    Identifier* parse_identifier(nlohmann::json& data) {
        std::string name = data.at("name");
        auto ret = new Identifier(name);
        // TODO: Parse types property.
        ret->set_type(IntType::get());
        return ret;
    }

    ASTNode* parse_literal(nlohmann::json& data) {
        // TODO: Parse types property.
        double val = data.at("value");
        auto   ret = new IntegerLiteral(int(val));
        ret->set_type(IntType::get());
        return ret;
    }

    std::unique_ptr<ASTNode> read_ast(std::ifstream& ifs) {
        nlohmann::json ast_data;
        ifs >> ast_data;

        auto module = ast_data.at("ModuleDecl");
        return std::unique_ptr<tango::ASTNode>(parse_block(module.at("body").at("Block")));
    }

} // namespace tango
