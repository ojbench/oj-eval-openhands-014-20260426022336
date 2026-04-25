#include "Evalvisitor.h"
#include <algorithm>
#include <cctype>

// Constructor
EvalVisitor::EvalVisitor() {
    current_scope = std::make_unique<Scope>();
}

// Helper methods to create values
std::unique_ptr<Value> EvalVisitor::make_none() {
    return std::make_unique<NoneValue>();
}

std::unique_ptr<Value> EvalVisitor::make_bool(bool v) {
    return std::make_unique<BoolValue>(v);
}

std::unique_ptr<Value> EvalVisitor::make_int(const std::string& v) {
    return std::make_unique<IntValue>(v);
}

std::unique_ptr<Value> EvalVisitor::make_int(int v) {
    return std::make_unique<IntValue>(v);
}

std::unique_ptr<Value> EvalVisitor::make_float(double v) {
    return std::make_unique<FloatValue>(v);
}

std::unique_ptr<Value> EvalVisitor::make_str(const std::string& v) {
    return std::make_unique<StrValue>(v);
}

// Helper to extract Value* from std::any
Value* get_value_from_any(std::any any_val) {
    if (any_val.type() == typeid(Value*)) {
        return std::any_cast<Value*>(any_val);
    }
    return nullptr;
}

// Big integer arithmetic implementation
std::string IntValue::add(const std::string& a, const std::string& b) {
    std::string result;
    int carry = 0;
    int i = a.length() - 1, j = b.length() - 1;
    
    while (i >= 0 || j >= 0 || carry > 0) {
        int digit_a = (i >= 0) ? (a[i--] - '0') : 0;
        int digit_b = (j >= 0) ? (b[j--] - '0') : 0;
        int sum = digit_a + digit_b + carry;
        carry = sum / 10;
        result.push_back((sum % 10) + '0');
    }
    
    std::reverse(result.begin(), result.end());
    return result;
}

std::string IntValue::subtract(const std::string& a, const std::string& b) {
    if (compare(a, b) < 0) {
        return "-" + subtract(b, a);
    }
    
    std::string result;
    int borrow = 0;
    int i = a.length() - 1, j = b.length() - 1;
    
    while (i >= 0) {
        int digit_a = a[i--] - '0';
        int digit_b = (j >= 0) ? (b[j--] - '0') : 0;
        
        digit_a -= borrow;
        if (digit_a < digit_b) {
            digit_a += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        
        result.push_back((digit_a - digit_b) + '0');
    }
    
    // Remove leading zeros
    while (result.length() > 1 && result.back() == '0') {
        result.pop_back();
    }
    
    std::reverse(result.begin(), result.end());
    return result;
}

std::string IntValue::multiply(const std::string& a, const std::string& b) {
    if (a == "0" || b == "0") return "0";
    
    std::string result(a.length() + b.length(), '0');
    
    for (int i = a.length() - 1; i >= 0; i--) {
        for (int j = b.length() - 1; j >= 0; j--) {
            int product = (a[i] - '0') * (b[j] - '0');
            int sum = product + (result[i + j + 1] - '0');
            
            result[i + j + 1] = (sum % 10) + '0';
            result[i + j] += (sum / 10);
        }
    }
    
    // Remove leading zeros
    size_t pos = result.find_first_not_of('0');
    return (pos == std::string::npos) ? "0" : result.substr(pos);
}

std::string IntValue::divide(const std::string& a, const std::string& b) {
    if (b == "0") throw std::runtime_error("Division by zero");
    
    // Simple implementation - convert to long double for now
    // In a real implementation, you'd want proper big integer division
    long double num_a = std::stold(a);
    long double num_b = std::stold(b);
    long double result = num_a / num_b;
    
    return std::to_string(static_cast<long long>(result));
}

std::string IntValue::mod(const std::string& a, const std::string& b) {
    if (b == "0") throw std::runtime_error("Modulo by zero");
    
    // a % b = a - (a // b) * b
    std::string quotient = floorDiv(a, b);
    std::string product = multiply(quotient, b);
    return subtract(a, product);
}

std::string IntValue::floorDiv(const std::string& a, const std::string& b) {
    if (b == "0") throw std::runtime_error("Division by zero");
    
    // For floor division, we need to handle negative numbers correctly
    bool negative = false;
    std::string abs_a = a, abs_b = b;
    
    if (a[0] == '-') {
        abs_a = a.substr(1);
        negative = !negative;
    }
    if (b[0] == '-') {
        abs_b = b.substr(1);
        negative = !negative;
    }
    
    std::string result = divide(abs_a, abs_b);
    
    // Adjust for floor division
    if ((a[0] == '-') ^ (b[0] == '-')) {
        // Different signs, check if there's a remainder
        std::string product = multiply(result, abs_b);
        if (compare(abs_a, product) != 0) {
            // There's a remainder, subtract 1 for floor division
            result = subtract(result, "1");
        }
    }
    
    if (negative && result != "0") {
        result = "-" + result;
    }
    
    return result;
}

int IntValue::compare(const std::string& a, const std::string& b) {
    // Handle signs
    bool a_negative = (a[0] == '-');
    bool b_negative = (b[0] == '-');
    
    if (a_negative && !b_negative) return -1;
    if (!a_negative && b_negative) return 1;
    
    std::string abs_a = a_negative ? a.substr(1) : a;
    std::string abs_b = b_negative ? b.substr(1) : b;
    
    // Compare lengths
    if (abs_a.length() != abs_b.length()) {
        return (abs_a.length() > abs_b.length()) ? 1 : -1;
    }
    
    // Compare digit by digit
    for (size_t i = 0; i < abs_a.length(); i++) {
        if (abs_a[i] != abs_b[i]) {
            return (abs_a[i] > abs_b[i]) ? 1 : -1;
        }
    }
    
    return 0;
}

// Type conversion helpers
std::unique_ptr<Value> EvalVisitor::to_int(std::unique_ptr<Value> v) {
    switch (v->type) {
        case ValueType::NONE:
            return make_int(0);
        case ValueType::BOOL:
            return make_int(static_cast<BoolValue*>(v.get())->value ? 1 : 0);
        case ValueType::INT:
            return v->clone();
        case ValueType::FLOAT: {
            double f = static_cast<FloatValue*>(v.get())->value;
            return make_int(static_cast<int>(f));
        }
        case ValueType::STR: {
            std::string s = static_cast<StrValue*>(v.get())->value;
            try {
                return make_int(std::stoll(s));
            } catch (...) {
                return make_int(0);
            }
        }
        default:
            return make_int(0);
    }
}

std::unique_ptr<Value> EvalVisitor::to_float(std::unique_ptr<Value> v) {
    switch (v->type) {
        case ValueType::NONE:
            return make_float(0.0);
        case ValueType::BOOL:
            return make_float(static_cast<BoolValue*>(v.get())->value ? 1.0 : 0.0);
        case ValueType::INT: {
            std::string s = static_cast<IntValue*>(v.get())->value;
            return make_float(std::stod(s));
        }
        case ValueType::FLOAT:
            return v->clone();
        case ValueType::STR: {
            std::string s = static_cast<StrValue*>(v.get())->value;
            try {
                return make_float(std::stod(s));
            } catch (...) {
                return make_float(0.0);
            }
        }
        default:
            return make_float(0.0);
    }
}

std::unique_ptr<Value> EvalVisitor::to_str(std::unique_ptr<Value> v) {
    return make_str(v->toString());
}

std::unique_ptr<Value> EvalVisitor::to_bool(std::unique_ptr<Value> v) {
    return make_bool(v->toBool());
}

// Arithmetic helpers
std::unique_ptr<Value> EvalVisitor::add(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    if (a->type == ValueType::STR && b->type == ValueType::STR) {
        return make_str(static_cast<StrValue*>(a.get())->value + static_cast<StrValue*>(b.get())->value);
    }
    if (a->type == ValueType::STR && b->type == ValueType::INT) {
        std::string s = static_cast<StrValue*>(a.get())->value;
        int count = std::stoi(static_cast<IntValue*>(b.get())->value);
        std::string result;
        for (int i = 0; i < count; i++) {
            result += s;
        }
        return make_str(result);
    }
    if (a->type == ValueType::INT && b->type == ValueType::STR) {
        return add(std::move(b), std::move(a));
    }
    
    // Convert to appropriate numeric type
    if (a->type == ValueType::FLOAT || b->type == ValueType::FLOAT) {
        auto fa = to_float(std::move(a));
        auto fb = to_float(std::move(b));
        return make_float(static_cast<FloatValue*>(fa.get())->value + static_cast<FloatValue*>(fb.get())->value);
    }
    
    auto ia = to_int(std::move(a));
    auto ib = to_int(std::move(b));
    return make_int(IntValue::add(static_cast<IntValue*>(ia.get())->value, static_cast<IntValue*>(ib.get())->value));
}

std::unique_ptr<Value> EvalVisitor::subtract(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    if (a->type == ValueType::FLOAT || b->type == ValueType::FLOAT) {
        auto fa = to_float(std::move(a));
        auto fb = to_float(std::move(b));
        return make_float(static_cast<FloatValue*>(fa.get())->value - static_cast<FloatValue*>(fb.get())->value);
    }
    
    auto ia = to_int(std::move(a));
    auto ib = to_int(std::move(b));
    return make_int(IntValue::subtract(static_cast<IntValue*>(ia.get())->value, static_cast<IntValue*>(ib.get())->value));
}

std::unique_ptr<Value> EvalVisitor::multiply(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    if (a->type == ValueType::STR && b->type == ValueType::INT) {
        std::string s = static_cast<StrValue*>(a.get())->value;
        int count = std::stoi(static_cast<IntValue*>(b.get())->value);
        std::string result;
        for (int i = 0; i < count; i++) {
            result += s;
        }
        return make_str(result);
    }
    if (a->type == ValueType::INT && b->type == ValueType::STR) {
        return multiply(std::move(b), std::move(a));
    }
    
    if (a->type == ValueType::FLOAT || b->type == ValueType::FLOAT) {
        auto fa = to_float(std::move(a));
        auto fb = to_float(std::move(b));
        return make_float(static_cast<FloatValue*>(fa.get())->value * static_cast<FloatValue*>(fb.get())->value);
    }
    
    auto ia = to_int(std::move(a));
    auto ib = to_int(std::move(b));
    return make_int(IntValue::multiply(static_cast<IntValue*>(ia.get())->value, static_cast<IntValue*>(ib.get())->value));
}

std::unique_ptr<Value> EvalVisitor::divide(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    auto fa = to_float(std::move(a));
    auto fb = to_float(std::move(b));
    if (static_cast<FloatValue*>(fb.get())->value == 0.0) {
        throw std::runtime_error("Division by zero");
    }
    return make_float(static_cast<FloatValue*>(fa.get())->value / static_cast<FloatValue*>(fb.get())->value);
}

std::unique_ptr<Value> EvalVisitor::floor_divide(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    auto ia = to_int(std::move(a));
    auto ib = to_int(std::move(b));
    return make_int(IntValue::floorDiv(static_cast<IntValue*>(ia.get())->value, static_cast<IntValue*>(ib.get())->value));
}

std::unique_ptr<Value> EvalVisitor::modulo(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    auto ia = to_int(std::move(a));
    auto ib = to_int(std::move(b));
    return make_int(IntValue::mod(static_cast<IntValue*>(ia.get())->value, static_cast<IntValue*>(ib.get())->value));
}

// Comparison helpers
std::unique_ptr<Value> EvalVisitor::equal(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    if (a->type != b->type) {
        // Try to convert to common type
        if (a->type == ValueType::FLOAT || b->type == ValueType::FLOAT) {
            auto fa = to_float(std::move(a));
            auto fb = to_float(std::move(b));
            return make_bool(static_cast<FloatValue*>(fa.get())->value == static_cast<FloatValue*>(fb.get())->value);
        }
        if (a->type == ValueType::INT || b->type == ValueType::INT) {
            auto ia = to_int(std::move(a));
            auto ib = to_int(std::move(b));
            return make_bool(IntValue::compare(static_cast<IntValue*>(ia.get())->value, static_cast<IntValue*>(ib.get())->value) == 0);
        }
        return make_bool(false);
    }
    
    switch (a->type) {
        case ValueType::NONE:
            return make_bool(true);
        case ValueType::BOOL:
            return make_bool(static_cast<BoolValue*>(a.get())->value == static_cast<BoolValue*>(b.get())->value);
        case ValueType::INT:
            return make_bool(IntValue::compare(static_cast<IntValue*>(a.get())->value, static_cast<IntValue*>(b.get())->value) == 0);
        case ValueType::FLOAT:
            return make_bool(static_cast<FloatValue*>(a.get())->value == static_cast<FloatValue*>(b.get())->value);
        case ValueType::STR:
            return make_bool(static_cast<StrValue*>(a.get())->value == static_cast<StrValue*>(b.get())->value);
        default:
            return make_bool(false);
    }
}

std::unique_ptr<Value> EvalVisitor::not_equal(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    auto eq = equal(std::move(a), std::move(b));
    return make_bool(!static_cast<BoolValue*>(eq.get())->value);
}

std::unique_ptr<Value> EvalVisitor::less_than(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    if (a->type == ValueType::STR && b->type == ValueType::STR) {
        return make_bool(static_cast<StrValue*>(a.get())->value < static_cast<StrValue*>(b.get())->value);
    }
    
    if (a->type == ValueType::FLOAT || b->type == ValueType::FLOAT) {
        auto fa = to_float(std::move(a));
        auto fb = to_float(std::move(b));
        return make_bool(static_cast<FloatValue*>(fa.get())->value < static_cast<FloatValue*>(fb.get())->value);
    }
    
    auto ia = to_int(std::move(a));
    auto ib = to_int(std::move(b));
    return make_bool(IntValue::compare(static_cast<IntValue*>(ia.get())->value, static_cast<IntValue*>(ib.get())->value) < 0);
}

std::unique_ptr<Value> EvalVisitor::greater_than(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    auto lt = less_than(std::move(b), std::move(a));
    return make_bool(static_cast<BoolValue*>(lt.get())->value);
}

std::unique_ptr<Value> EvalVisitor::less_equal(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    auto gt = greater_than(std::move(a), std::move(b));
    return make_bool(!static_cast<BoolValue*>(gt.get())->value);
}

std::unique_ptr<Value> EvalVisitor::greater_equal(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    auto lt = less_than(std::move(a), std::move(b));
    return make_bool(!static_cast<BoolValue*>(lt.get())->value);
}

// Logical helpers
std::unique_ptr<Value> EvalVisitor::logical_and(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    if (!a->toBool()) {
        return std::move(a);
    }
    return std::move(b);
}

std::unique_ptr<Value> EvalVisitor::logical_or(std::unique_ptr<Value> a, std::unique_ptr<Value> b) {
    if (a->toBool()) {
        return std::move(a);
    }
    return std::move(b);
}

std::unique_ptr<Value> EvalVisitor::logical_not(std::unique_ptr<Value> a) {
    return make_bool(!a->toBool());
}

// Visitor method implementations
std::any EvalVisitor::visitFile_input(Python3Parser::File_inputContext *ctx) {
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
    }
    return nullptr;
}

std::any EvalVisitor::visitStmt(Python3Parser::StmtContext *ctx) {
    if (ctx->compound_stmt()) {
        return visit(ctx->compound_stmt());
    } else if (ctx->simple_stmt()) {
        return visit(ctx->simple_stmt());
    }
    return nullptr;
}

std::any EvalVisitor::visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) {
    return visit(ctx->small_stmt());
}

std::any EvalVisitor::visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) {
    if (ctx->expr_stmt()) {
        return visit(ctx->expr_stmt());
    }
    return nullptr;
}

std::any EvalVisitor::visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) {
    auto testlist = ctx->testlist();
    if (testlist.size() == 1) {
        return visit(testlist[0]);
    }
    return nullptr;
}

std::any EvalVisitor::visitCompound_stmt(Python3Parser::Compound_stmtContext *ctx) {
    if (ctx->if_stmt()) {
        return visit(ctx->if_stmt());
    } else if (ctx->while_stmt()) {
        return visit(ctx->while_stmt());
    }
    return nullptr;
}

std::any EvalVisitor::visitIf_stmt(Python3Parser::If_stmtContext *ctx) {
    // Simple if implementation - just check first condition
    auto condition_any = visit(ctx->test(0));
    // For now, just execute the first suite
    visit(ctx->suite(0));
    return nullptr;
}

std::any EvalVisitor::visitWhile_stmt(Python3Parser::While_stmtContext *ctx) {
    // Simple while implementation
    visit(ctx->suite());
    return nullptr;
}

std::any EvalVisitor::visitSuite(Python3Parser::SuiteContext *ctx) {
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
    }
    return nullptr;
}

std::any EvalVisitor::visitTest(Python3Parser::TestContext *ctx) {
    if (ctx->or_test()) {
        return visit(ctx->or_test());
    }
    return nullptr;
}

std::any EvalVisitor::visitOr_test(Python3Parser::Or_testContext *ctx) {
    if (ctx->and_test().size() == 1) {
        return visit(ctx->and_test(0));
    }
    // For now, just return the first one
    return visit(ctx->and_test(0));
}

std::any EvalVisitor::visitAnd_test(Python3Parser::And_testContext *ctx) {
    if (ctx->not_test().size() == 1) {
        return visit(ctx->not_test(0));
    }
    // For now, just return the first one
    return visit(ctx->not_test(0));
}

std::any EvalVisitor::visitNot_test(Python3Parser::Not_testContext *ctx) {
    if (ctx->not_test()) {
        return visit(ctx->not_test());
    } else if (ctx->comparison()) {
        return visit(ctx->comparison());
    }
    return nullptr;
}

std::any EvalVisitor::visitComparison(Python3Parser::ComparisonContext *ctx) {
    if (ctx->arith_expr().size() == 1) {
        return visit(ctx->arith_expr(0));
    }
    // For now, just return the first one
    return visit(ctx->arith_expr(0));
}

std::any EvalVisitor::visitArith_expr(Python3Parser::Arith_exprContext *ctx) {
    if (ctx->term().size() == 1) {
        return visit(ctx->term(0));
    }
    
    auto result_any = visit(ctx->term(0));
    auto result = get_value_from_any(result_any);
    if (!result) return nullptr;
    
    for (size_t i = 0; i < ctx->addorsub_op().size(); i++) {
        auto op = ctx->addorsub_op(i);
        auto next_term_any = visit(ctx->term(i + 1));
        auto next_term = get_value_from_any(next_term_any);
        if (!next_term) return nullptr;
        
        if (op->ADD()) {
            // Simple addition for integers
            if (result->type == ValueType::INT && next_term->type == ValueType::INT) {
                std::string sum = IntValue::add(
                    static_cast<IntValue*>(result)->value,
                    static_cast<IntValue*>(next_term)->value
                );
                result = make_int(sum).release();
            } else {
                // For other types, just use string concatenation
                std::string combined = result->toString() + next_term->toString();
                result = make_str(combined).release();
            }
        } else if (op->MINUS()) {
            // Simple subtraction for integers
            if (result->type == ValueType::INT && next_term->type == ValueType::INT) {
                std::string diff = IntValue::subtract(
                    static_cast<IntValue*>(result)->value,
                    static_cast<IntValue*>(next_term)->value
                );
                result = make_int(diff).release();
            }
        }
    }
    
    return std::any(result);
}

std::any EvalVisitor::visitTerm(Python3Parser::TermContext *ctx) {
    if (ctx->factor().size() == 1) {
        return visit(ctx->factor(0));
    }
    
    auto result_any = visit(ctx->factor(0));
    auto result = get_value_from_any(result_any);
    if (!result) return nullptr;
    
    for (size_t i = 0; i < ctx->muldivmod_op().size(); i++) {
        auto op = ctx->muldivmod_op(i);
        auto next_factor_any = visit(ctx->factor(i + 1));
        auto next_factor = get_value_from_any(next_factor_any);
        if (!next_factor) return nullptr;
        
        if (op->STAR()) {
            // Simple multiplication for integers
            if (result->type == ValueType::INT && next_factor->type == ValueType::INT) {
                std::string product = IntValue::multiply(
                    static_cast<IntValue*>(result)->value,
                    static_cast<IntValue*>(next_factor)->value
                );
                result = make_int(product).release();
            }
        }
    }
    
    return std::any(result);
}

std::any EvalVisitor::visitFactor(Python3Parser::FactorContext *ctx) {
    if (ctx->atom_expr()) {
        return visit(ctx->atom_expr());
    }
    return nullptr;
}

std::any EvalVisitor::visitAtom_expr(Python3Parser::Atom_exprContext *ctx) {
    auto atom_any = visit(ctx->atom());
    auto atom = get_value_from_any(atom_any);
    
    // Handle function calls
    if (ctx->trailer() && atom && atom->type == ValueType::STR) {
        std::string name = static_cast<StrValue*>(atom)->value;
        if (name == "print" && ctx->trailer()->OPEN_PAREN()) {
            // Check if there are arguments
            if (ctx->trailer()->arglist()) {
                auto arglist = ctx->trailer()->arglist();
                bool first = true;
                for (auto arg : arglist->argument()) {
                    auto tests = arg->test();
                    if (!tests.empty()) {
                        auto arg_value_any = visit(tests[0]);
                        auto arg_value = get_value_from_any(arg_value_any);
                        if (arg_value) {
                            if (!first) std::cout << " ";
                            std::cout << arg_value->toString();
                            first = false;
                        }
                    }
                }
            }
            std::cout << std::endl;
            return std::any(make_none().release());
        }
    }
    
    return atom_any;
}

std::any EvalVisitor::visitAtom(Python3Parser::AtomContext *ctx) {
    if (ctx->NONE()) {
        return std::any(make_none().release());
    } else if (ctx->TRUE()) {
        return std::any(make_bool(true).release());
    } else if (ctx->FALSE()) {
        return std::any(make_bool(false).release());
    } else if (ctx->NUMBER()) {
        std::string num_str = ctx->NUMBER()->toString();
        if (num_str.find('.') != std::string::npos) {
            return std::any(make_float(std::stod(num_str)).release());
        } else {
            return std::any(make_int(num_str).release());
        }
    } else if (ctx->STRING().size() > 0) {
        std::string str = ctx->STRING()[0]->toString();
        // Remove quotes
        if (str.length() >= 2 && (str.front() == '"' || str.front() == '\'')) {
            str = str.substr(1, str.length() - 2);
        }
        return std::any(make_str(str).release());
    } else if (ctx->NAME()) {
        std::string name = ctx->NAME()->toString();
        auto value = current_scope->get(name);
        if (value) {
            return std::any(value->clone().release());
        }
        return std::any(make_str(name).release());
    } else if (ctx->test()) {
        return visit(ctx->test());
    }
    return std::any(make_none().release());
}

std::any EvalVisitor::visitFormat_string(Python3Parser::Format_stringContext *ctx) {
    return std::any(make_str("").release());
}

std::any EvalVisitor::visitTestlist(Python3Parser::TestlistContext *ctx) {
    if (ctx->test().size() == 1) {
        return visit(ctx->test(0));
    }
    return visit(ctx->test(0));
}

std::any EvalVisitor::visitArglist(Python3Parser::ArglistContext *ctx) {
    return nullptr;
}

std::any EvalVisitor::visitArgument(Python3Parser::ArgumentContext *ctx) {
    return nullptr;
}

std::any EvalVisitor::visitFuncdef(Python3Parser::FuncdefContext *ctx) {
    return nullptr;
}

std::any EvalVisitor::visitParameters(Python3Parser::ParametersContext *ctx) {
    return nullptr;
}

std::any EvalVisitor::visitTypedargslist(Python3Parser::TypedargslistContext *ctx) {
    return nullptr;
}

std::any EvalVisitor::visitTfpdef(Python3Parser::TfpdefContext *ctx) {
    return nullptr;
}

std::any EvalVisitor::visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) {
    return nullptr;
}

std::any EvalVisitor::visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) {
    return nullptr;
}

std::any EvalVisitor::visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) {
    return nullptr;
}

std::any EvalVisitor::visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) {
    return nullptr;
}
