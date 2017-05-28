//
//  main.cpp
//  tango
//
//  Created by 4rch0dia on 27.05.17.
//  Copyright © 2017 University of Geneva. All rights reserved.
//

#include <iostream>
#include <map>
#include <stack>
#include <string>

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>

// Optimizations.
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/Scalar.h>

#include "ast.hh"

#define INT_BITS 64


llvm::Value* log_error(const char *msg) {
    std::cerr << msg << std::endl;
    return nullptr;
}

static llvm::LLVMContext context;
static llvm::IRBuilder<> builder(context);
static std::unique_ptr<llvm::Module> module;
static std::map<std::string, llvm::AllocaInst*> symbol_table;

/// Create an alloca instruction in the enty block of the function.
static llvm::AllocaInst* create_alloca(
    llvm::Function* function, llvm::Type* type, const std::string& name)
{
    llvm::IRBuilder<> tmp_builder(&function->getEntryBlock(), function->getEntryBlock().begin());
    return tmp_builder.CreateAlloca(type, 0, name);
}


using namespace Tango;

struct IRCodeGenerator: public ASTNodeVisitor {

    ~IRCodeGenerator() {
        while (!this->stack.empty()) {
            this->stack.pop();
        }
    }

    void visit(Block& node) {
        for (auto statement: node.statements) {
            statement->accept(*this);
        }
    }

    void visit(PropertyDecl& node) {
    }

    void visit(FunctionParam& node) {}

    void visit(FunctionDecl& node) {
        // Create the function type "(at0, ..., atn) -> rt".
        std::vector<llvm::Type*> arg_types(
            node.parameters.size(), llvm::Type::getInt64Ty(context));

        auto function_type = llvm::FunctionType::get(
            llvm::Type::getInt64Ty(context), arg_types, false);

        // Create the function object.
        auto function = llvm::Function::Create(
            function_type, llvm::Function::ExternalLinkage, node.name, module.get());

        // Set the names for all parameters.
        std::size_t idx = 0;
        for (auto& arg: function->args()) {
            arg.setName(node.parameters[idx]->name);
            idx += 1;
        }

        // Create a new basic block to start insertion into.
        auto basic_block = llvm::BasicBlock::Create(context, "entry", function);
        builder.SetInsertPoint(basic_block);

        // Store the function parameters in the symbol table.
        symbol_table.clear();
        for (auto& arg: function->args()) {
            // Create an alloca for the argument, and store its value.
            auto alloca = create_alloca(function, llvm::Type::getInt64Ty(context), arg.getName());
            builder.CreateStore(&arg, alloca);

            // Update the symbol table.
            symbol_table[arg.getName()] = alloca;
        }

        // Generate the body of the function.
        node.body->statements[0]->accept(*this);
        builder.CreateRet(this->stack.top());
        this->stack.pop();
        builder.ClearInsertionPoint();

        // Validate the generated code, checking for consistency.
        llvm::verifyFunction(*function);

        this->stack.push(function);
    }

    void visit(If& node) {
        // Generate the IR code for the node's condition.
        node.condition->accept(*this);
        auto condition = this->stack.top();
        this->stack.pop();

        // TODO: Make sure the condition is a value of type i1 (bool).

        // Get the current function object.
        auto function   = builder.GetInsertBlock()->getParent();

        // Create a a conditional branching block.
        auto then_block = llvm::BasicBlock::Create(context, "then", function);
        auto else_block = llvm::BasicBlock::Create(context, "else");
        auto branch     = builder.CreateCondBr(condition, then_block, else_block);

        // Generate the IR code for the "then" block.
        builder.SetInsertPoint(then_block);
        node.then_block->accept(*this);

        std::vector<llvm::Value*> values;
        while (!this->stack.empty()) {
            values.insert(values.begin(), this->stack.top());
            this->stack.pop();
        }

        for (auto value: values) {

        }
    }

    void visit(Return& node) {
        // Push the IR code of the return value onto the stack.
        node.value->accept(*this);
    }

    void visit(BinaryExpr& node) {
        // Visit the left and right operand and put their IR code on the stack.
        node.left->accept(*this);
        node.right->accept(*this);

        auto right_value = this->stack.top();
        this->stack.pop();
        auto left_value = this->stack.top();
        this->stack.pop();

        switch (node.op) {
            case Tango::add:
                this->stack.push(builder.CreateAdd(left_value, right_value));
                break;
            case Tango::sub:
                this->stack.push(builder.CreateSub(left_value, right_value));
                break;
            case Tango::mul:
                this->stack.push(builder.CreateMul(left_value, right_value));
                break;
            case Tango::div:
                // Convert the operands to Double.
                auto left_fp = builder.CreateSIToFP(
                    left_value, llvm::Type::getDoubleTy(context));
                auto right_fp = builder.CreateSIToFP(
                    right_value, llvm::Type::getDoubleTy(context));

                // Compute the result in Double.
                auto result_fp = builder.CreateFDiv(left_fp, right_fp);

                // Convert the result in Int.
                this->stack.push(
                    builder.CreateFPToUI(result_fp, llvm::Type::getInt64Ty(context)));
                break;
        }
    }

    void visit(Call& node) {
        // Get the callee from the symbol table of the module.
        auto& callee_name = static_cast<Identifier*>(node.callee)->name;
        auto callee       = module.get()->getFunction(callee_name);

        if (callee == nullptr) {
            log_error("unknown function reference");
            return;
        }

        if (callee->arg_size() != node.arguments.size()) {
            log_error("invalid number of arguments");
            return;
        }

        std::vector<llvm::Value*> args;
        for (auto call_arg: node.arguments) {
            call_arg->value->accept(*this);
            args.push_back(this->stack.top());
            this->stack.pop();
        }

        this->stack.push(builder.CreateCall(callee, args));
    }

    void visit(CallArg& node) {}

    void visit(Identifier& node) {
        // Look for the identifier in the symbol table.
        auto value = symbol_table[node.name];
        if (value == nullptr) {
            log_error("undefined symbol");
            return;
        }

        // Load the value from the stack.
        this->stack.push(builder.CreateLoad(value, node.name.c_str()));
    }

    void visit(IntegerLiteral& node) {
        this->stack.push(llvm::ConstantInt::get(
            context, llvm::APInt(INT_BITS, node.value, true)));
    }

    std::stack<llvm::Value*> stack;
};


int main(int argc, char* argv[]) {
    // Create the module, which holds all the code.
    module = llvm::make_unique<llvm::Module>("tango module", context);

    auto ast = Block({
        new FunctionDecl("f", {new FunctionParam("x", Tango::cst)}, new Block({
            new Return(new Identifier("x"))
        }))
        //new Call(new Identifier("f"), {new CallArg("x", Tango::cpy_assign, new IntegerLiteral(0))})
    });

    // Generate the IR code of the module.
    auto code_generator = IRCodeGenerator();
    ast.accept(code_generator);

    // Create an optimization pass manager.
    // auto pass_manager = llvm::make_unique<llvm::legacy::PassManager>();
    // pass_manager->add(llvm::createPromoteMemoryToRegisterPass());
    // pass_manager->run(*(module.get()));

    module.get()->dump();

    return 0;
}
