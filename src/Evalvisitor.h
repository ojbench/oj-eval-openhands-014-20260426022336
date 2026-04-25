#pragma once
#ifndef PYTHON_INTERPRETER_EVALVISITOR_H
#define PYTHON_INTERPRETER_EVALVISITOR_H

#include "Python3ParserBaseVisitor.h"
#include <any>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

// Value type enumeration
enum class ValueType {
    NONE,
    BOOL,
    INT,
    FLOAT,
    STR,
    FUNCTION
};

// Forward declaration
class EvalVisitor;

// Base class for all values
class Value {
public:
    ValueType type;
    
    Value(ValueType t) : type(t) {}
    virtual ~Value() = default;
    
    virtual std::string toString() const = 0;
    virtual bool toBool() const = 0;
    virtual std::unique_ptr<Value> clone() const = 0;
};

// None value
class NoneValue : public Value {
public:
    NoneValue() : Value(ValueType::NONE) {}
    
    std::string toString() const override { return "None"; }
    bool toBool() const override { return false; }
    std::unique_ptr<Value> clone() const override { return std::make_unique<NoneValue>(); }
};

// Boolean value
class BoolValue : public Value {
public:
    bool value;
    
    BoolValue(bool v) : Value(ValueType::BOOL), value(v) {}
    
    std::string toString() const override { return value ? "True" : "False"; }
    bool toBool() const override { return value; }
    std::unique_ptr<Value> clone() const override { return std::make_unique<BoolValue>(value); }
};

// Integer value (using std::string for arbitrary precision)
class IntValue : public Value {
public:
    std::string value;
    
    IntValue(const std::string& v) : Value(ValueType::INT), value(v) {}
    IntValue(int v) : Value(ValueType::INT), value(std::to_string(v)) {}
    
    std::string toString() const override { return value; }
    bool toBool() const override { return value != "0"; }
    std::unique_ptr<Value> clone() const override { return std::make_unique<IntValue>(value); }
    
    // Helper methods for big integer arithmetic
    static std::string add(const std::string& a, const std::string& b);
    static std::string subtract(const std::string& a, const std::string& b);
    static std::string multiply(const std::string& a, const std::string& b);
    static std::string divide(const std::string& a, const std::string& b);
    static std::string mod(const std::string& a, const std::string& b);
    static std::string floorDiv(const std::string& a, const std::string& b);
    static int compare(const std::string& a, const std::string& b);
};

// Float value
class FloatValue : public Value {
public:
    double value;
    
    FloatValue(double v) : Value(ValueType::FLOAT), value(v) {}
    
    std::string toString() const override { 
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << value;
        return oss.str();
    }
    bool toBool() const override { return value != 0.0; }
    std::unique_ptr<Value> clone() const override { return std::make_unique<FloatValue>(value); }
};

// String value
class StrValue : public Value {
public:
    std::string value;
    
    StrValue(const std::string& v) : Value(ValueType::STR), value(v) {}
    
    std::string toString() const override { return value; }
    bool toBool() const override { return !value.empty(); }
    std::unique_ptr<Value> clone() const override { return std::make_unique<StrValue>(value); }
};

// Scope management
class Scope {
public:
    std::map<std::string, std::unique_ptr<Value>> variables;
    Scope* parent;
    
    Scope(Scope* p = nullptr) : parent(p) {}
    
    void set(const std::string& name, std::unique_ptr<Value> value) {
        variables[name] = std::move(value);
    }
    
    Value* get(const std::string& name) {
        auto it = variables.find(name);
        if (it != variables.end()) {
            return it->second.get();
        }
        if (parent) {
            return parent->get(name);
        }
        return nullptr;
    }
    
    bool has(const std::string& name) {
        return get(name) != nullptr;
    }
};

// Forward declaration for FunctionValue
class FunctionValue;

class EvalVisitor : public Python3ParserBaseVisitor {
private:
    std::unique_ptr<Scope> current_scope;
    std::map<std::string, std::unique_ptr<FunctionValue>> functions;
    bool in_loop = false;
    bool should_break = false;
    bool should_continue = false;
    std::unique_ptr<Value> return_value;
    
public:
    EvalVisitor();
    
    // Helper methods
    std::unique_ptr<Value> make_none();
    std::unique_ptr<Value> make_bool(bool v);
    std::unique_ptr<Value> make_int(const std::string& v);
    std::unique_ptr<Value> make_int(int v);
    std::unique_ptr<Value> make_float(double v);
    std::unique_ptr<Value> make_str(const std::string& v);
    
    // Type conversion helpers
    std::unique_ptr<Value> to_int(std::unique_ptr<Value> v);
    std::unique_ptr<Value> to_float(std::unique_ptr<Value> v);
    std::unique_ptr<Value> to_str(std::unique_ptr<Value> v);
    std::unique_ptr<Value> to_bool(std::unique_ptr<Value> v);
    
    // Arithmetic helpers
    std::unique_ptr<Value> add(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    std::unique_ptr<Value> subtract(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    std::unique_ptr<Value> multiply(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    std::unique_ptr<Value> divide(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    std::unique_ptr<Value> floor_divide(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    std::unique_ptr<Value> modulo(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    
    // Comparison helpers
    std::unique_ptr<Value> equal(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    std::unique_ptr<Value> not_equal(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    std::unique_ptr<Value> less_than(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    std::unique_ptr<Value> greater_than(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    std::unique_ptr<Value> less_equal(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    std::unique_ptr<Value> greater_equal(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    
    // Logical helpers
    std::unique_ptr<Value> logical_and(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    std::unique_ptr<Value> logical_or(std::unique_ptr<Value> a, std::unique_ptr<Value> b);
    std::unique_ptr<Value> logical_not(std::unique_ptr<Value> a);
    
    // Override visitor methods
    std::any visitFile_input(Python3Parser::File_inputContext *ctx) override;
    std::any visitStmt(Python3Parser::StmtContext *ctx) override;
    std::any visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) override;
    std::any visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) override;
    std::any visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) override;
    std::any visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) override;
    std::any visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) override;
    std::any visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) override;
    std::any visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) override;
    std::any visitCompound_stmt(Python3Parser::Compound_stmtContext *ctx) override;
    std::any visitIf_stmt(Python3Parser::If_stmtContext *ctx) override;
    std::any visitWhile_stmt(Python3Parser::While_stmtContext *ctx) override;
    std::any visitSuite(Python3Parser::SuiteContext *ctx) override;
    std::any visitTest(Python3Parser::TestContext *ctx) override;
    std::any visitOr_test(Python3Parser::Or_testContext *ctx) override;
    std::any visitAnd_test(Python3Parser::And_testContext *ctx) override;
    std::any visitNot_test(Python3Parser::Not_testContext *ctx) override;
    std::any visitComparison(Python3Parser::ComparisonContext *ctx) override;
    std::any visitArith_expr(Python3Parser::Arith_exprContext *ctx) override;
    std::any visitTerm(Python3Parser::TermContext *ctx) override;
    std::any visitFactor(Python3Parser::FactorContext *ctx) override;
    std::any visitAtom_expr(Python3Parser::Atom_exprContext *ctx) override;
    std::any visitAtom(Python3Parser::AtomContext *ctx) override;
    std::any visitFormat_string(Python3Parser::Format_stringContext *ctx) override;
    std::any visitTestlist(Python3Parser::TestlistContext *ctx) override;
    std::any visitArglist(Python3Parser::ArglistContext *ctx) override;
    std::any visitArgument(Python3Parser::ArgumentContext *ctx) override;
    std::any visitFuncdef(Python3Parser::FuncdefContext *ctx) override;
    std::any visitParameters(Python3Parser::ParametersContext *ctx) override;
    std::any visitTypedargslist(Python3Parser::TypedargslistContext *ctx) override;
    std::any visitTfpdef(Python3Parser::TfpdefContext *ctx) override;
};

// Function value
class FunctionValue : public Value {
public:
    std::string name;
    std::vector<std::string> params;
    std::map<std::string, std::string> default_params;
    Python3Parser::SuiteContext* suite;
    EvalVisitor* visitor;
    
    FunctionValue(const std::string& n, const std::vector<std::string>& p, 
                  const std::map<std::string, std::string>& dp,
                  Python3Parser::SuiteContext* s, EvalVisitor* v)
        : Value(ValueType::FUNCTION), name(n), params(p), default_params(dp), suite(s), visitor(v) {}
    
    std::string toString() const override { return "<function '" + name + "'>"; }
    bool toBool() const override { return true; }
    std::unique_ptr<Value> clone() const override { 
        return std::make_unique<FunctionValue>(name, params, default_params, suite, visitor); 
    }
};

#endif//PYTHON_INTERPRETER_EVALVISITOR_H
