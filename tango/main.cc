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
#include <stdexcept>
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
#define INT_TY   llvm::Type::getInt64Ty(context)


llvm::Value* log_error(const char *msg) {
    std::cerr << msg << std::endl;
    return nullptr;
}

static llvm::LLVMContext context;
static llvm::IRBuilder<> builder(context);
static std::unique_ptr<llvm::Module> module;

static std::map<std::string, llvm::AllocaInst*>     locals;
static std::map<std::string, llvm::GlobalVariable*> globals;

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
        auto insert_block = builder.GetInsertBlock();
        auto function     = insert_block
            ? insert_block->getParent()
            : nullptr;

        switch (node.type_storage) {
            case Tango::ts_alloc:
                // If we're not generating the body of a function, we're
                // looking at a global variable.
                if (insert_block == nullptr) {
                    // Create the global variable, initialized to 0.
                    module->getOrInsertGlobal(node.name, INT_TY);
                    auto global_var = module->getNamedGlobal(node.name);
                    global_var->setLinkage(llvm::GlobalVariable::CommonLinkage);
                    global_var->setInitializer(
                        llvm::ConstantInt::get(context, llvm::APInt(INT_BITS, 0, true)));

                    // Store it in globals.
                    globals[node.name] = global_var;
                } else {
                    // Create an alloca for the variable.
                    auto local_var = create_alloca(function, INT_TY, node.name);
                    builder.CreateStore(
                        llvm::ConstantInt::get(context, llvm::APInt(INT_BITS, 0, true)),
                        local_var);

                    // Store the variable in locals.
                    locals[node.name] = local_var;
                }

                break;

            case Tango::ts_ref:
                {
                    // Create an alloca for the variable.
                    auto ptr_ty = llvm::PointerType::get(INT_TY, 0);
                    auto local_var = create_alloca(function, ptr_ty, node.name);
                    builder.CreateStore(llvm::ConstantPointerNull::get(ptr_ty), local_var);

                    // Store the variable in locals.
                    locals[node.name] = local_var;
                }
        }

        // TODO: Handle ts_heap (shared) variables.
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

        // Store the function parameters in the local symbol table.
        locals.clear();
        for (auto& arg: function->args()) {
            // Create an alloca for the argument, and store its value.
            auto alloca = create_alloca(function, llvm::Type::getInt64Ty(context), arg.getName());
            builder.CreateStore(&arg, alloca);

            // Update the symbol table.
            locals[arg.getName()] = alloca;
        }

        // Generate the body of the function.
        node.body->accept(*this);
        builder.ClearInsertionPoint();

        // Validate the generated code, checking for consistency.
        llvm::verifyFunction(*function);

        this->stack.push(function);
    }

    void visit(Assignment& node) {
        // Until we implement select and subscript expressions, we can expect
        // the lvalue to always be an identifier.
        auto target = dynamic_cast<Identifier*>(node.lvalue);
        if (target == nullptr) {
            throw std::invalid_argument("invalid lvalue for assignment");
        }

        // Retrieve the variable from either locals, or globals.
        llvm::Value* var = nullptr;

        auto local_it = locals.find(target->name);
        if (local_it != locals.end()) {
            var = local_it->second;
        } else {
            auto global_it = globals.find(target->name);
            if (global_it != globals.end()) {
                var = global_it->second;
            }
        }

        if (var == nullptr) {
            throw std::invalid_argument("undefined symbol");
        }

        // Dereference var if it's a pointer.
        if (llvm::isa<llvm::PointerType>(var->getType())) {
            var = builder.CreateLoad(var);
        }

        // Push the IR code of the rvalue onto the stack.
        node.rvalue->accept(*this);

        // Create a store instruction.
        builder.CreateStore(this->stack.top(), var);
        this->stack.pop();
    }

    void visit(If& node) {
//        // Generate the IR code for the node's condition.
//        node.condition->accept(*this);
//        auto condition = this->stack.top();
//        this->stack.pop();
//
//        // TODO: Make sure the condition is a value of type i1 (bool).
//
//        // Get the current function object.
//        auto function   = builder.GetInsertBlock()->getParent();
//
//        // Create a a conditional branching block.
//        auto then_block = llvm::BasicBlock::Create(context, "then", function);
//        auto else_block = llvm::BasicBlock::Create(context, "else");
//        auto branch     = builder.CreateCondBr(condition, then_block, else_block);
//
//        // Generate the IR code for the "then" block.
//        builder.SetInsertPoint(then_block);
//        node.then_block->accept(*this);
//
//        std::vector<llvm::Value*> values;
//        while (!this->stack.empty()) {
//            values.insert(values.begin(), this->stack.top());
//            this->stack.pop();
//        }
    }

    void visit(Return& node) {
        // Make sure we're generating a function's body.
        if (builder.GetInsertBlock() == nullptr) {
            log_error("return statement outside of a function body");
        }

        // Push the IR code of the return value onto the stack and create the
        // return statement.
        node.value->accept(*this);
        builder.CreateRet(this->stack.top());
    }

    void visit(BinaryExpr& node) {
        // Push the IR code of the left and right operand onto the stack,
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
        // Look for the identifier in the local symbol table.
        auto value = locals[node.name];
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


void add_main_function() {
    auto i32  = llvm::IntegerType::getInt32Ty(module->getContext());
    auto i8   = llvm::IntegerType::getInt8Ty(module->getContext());
    auto i8pp = llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(i8));

    // define i32 @main(i32, i8**)
    auto fn = static_cast<llvm::Function*>(module->getOrInsertFunction("main", i32, i32, i8pp));
    auto bb = llvm::BasicBlock::Create(module->getContext(), "entry", fn);

    // ret i32 0
    llvm::IRBuilder<> tb(bb, bb->begin());
    tb.CreateRet(llvm::ConstantInt::get(module->getContext(), llvm::APInt(32, 0)));
}


int main(int argc, char* argv[]) {
    // Create the module, which holds all the code.
    module = llvm::make_unique<llvm::Module>("tango module", context);

    // Add the main function.
    add_main_function();

    auto ast = Block({
        new FunctionDecl("f", {new FunctionParam("x")}, new Block({
            new PropertyDecl("y"),
            // new PropertyDecl("y", Tango::tm_mut, Tango::ts_ref),
            new Assignment(new Identifier("y"), Tango::ao_ref, new Identifier("x")),
            new Return(new Identifier("y")),
        })),
//        new Call(new Identifier("f"), {new CallArg("x", Tango::cpy_assign, new IntegerLiteral(0))})
    });

    // Generate the IR code of the module.
    auto code_generator = IRCodeGenerator();
    ast.accept(code_generator);

    // Create an optimization pass manager.
//     auto pass_manager = llvm::make_unique<llvm::legacy::PassManager>();
//     pass_manager->add(llvm::createPromoteMemoryToRegisterPass());
//     pass_manager->run(*(module.get()));

    module.get()->dump();

    return 0;
}
