#include "expression.h"
#include "worksheet.h"
#include "workspace.h"
#include <algorithm>
#include <functional>
#include <numeric>
#include <boost/algorithm/string/trim.hpp>

template<class exp>
std::ostream& operator<<(std::ostream& os, const std::shared_ptr<exp> self) {
    static_assert(std::is_base_of<expression, exp>::value, "exp must be inherited from expression");
    return os << self->debug_message();
}

bool is_letter_or_underscore(char x) {
    return (x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z') || (x == '_');
}
int get_precedence(std::string op) {
    if (op == "*" || op == "/") return 3;
    if (op == "+" || op == "-") return 2;
    if (op == "&") return 1;
    if (op == "=" || op == "<>" || op == "<" || op == ">" || op == "<=" || op == ">=") return 0;
    return -1;
}
bool is_unary(std::string op) {
    return op == "+" || op == "-";
}
expression::parse_expr expression::parse(const std::string& str) {
    // expression := -------- constant --------|
    //                 |----- integer ---|
    //                 |----- text ------|
    //                 |----- bracket ---|
    //                 |----- operator --|
    //                 |- function call -|
    //                 ---- reference ----
    // integer := --- (no exception by stoll) --|
    // text := ---- " -------------------------- " ---|
    //                 |                      |
    //                 -- character except " --
    // bracket := --- ( --- expression --- ) -----|
    // operator := --- expression --- op ----- expression ----|
    // function call := ----- identifier --- ( ------------------------------------- ) -----|
    //                                          |                                 |
    //                                          -- expression ------<-------------
    //                                                         |                  |
    //                                                         -- , -- expression |
    // identifier := --- A-Z ---------------------|
    //                |- a-z -|   |-- A-Z --|
    //                --- _ ---   |-- a-z --|
    //                            |--- _ ---|
    //                            |-- 0-9 --|
    // reference := ------ A-Z, a-z ------------ 1-9 ------------------|
    //                 |              |                 |            |
    //                 ------<---------                 -- 0-9 ---<---

    std::string trimmed = boost::trim_copy(str);
    if (trimmed.empty()) throw parse_exception(str, "empty expresion");

    std::string upper_trimmed = trimmed;
    for (char& c : upper_trimmed) c = toupper(c);
    if (upper_trimmed == "TRUE") return std::make_shared<boolean>(true);
    if (upper_trimmed == "FALSE") return std::make_shared<boolean>(false);

    // string
    if (trimmed[0] == '"' && trimmed[trimmed.length()-1] == '"' && std::count(trimmed.begin(), trimmed.end(), '"') == 2) {
        return std::make_shared<expression::text>(trimmed.substr(1, trimmed.length()-2));
    }

    // integer
    try {
        std::string::size_type size;
        long long res = std::stoll(trimmed, &size);
        if (size != trimmed.size()) throw std::invalid_argument("expect size == trimmed.size()");
        return std::make_shared<expression::integer>(res);
    } catch (std::exception e) {}

    // reference
    try {
        worksheet::cell_reference ref = worksheet::cell_reference::from_code(trimmed);
        return std::make_shared<expression::reference>(ref);
    } catch (std::exception e) {}

    // bracket
    if (trimmed[0] == '(' && trimmed[trimmed.length()-1] == ')') {
        try {
            return parse(trimmed.substr(1, trimmed.length()-2));
        } catch (std::exception e) {}
    }

    // function call
    if (trimmed[trimmed.length()-1] == ')' && is_letter_or_underscore(trimmed[0])) {
        // identifier
        int i;
        std::string func_name;
        func_name += trimmed[0];
        for (i = 1; is_letter_or_underscore(trimmed[i]) || (trimmed[i] >= '0' && trimmed[i] <= '9'); ++i) {
            func_name += trimmed[i];
        }
        if (trimmed[i] == '(') {
            // comma separated expressions
            int bracket_level = 0;
            bool is_text = false;
            std::vector<std::string> str_args;
            std::string cur_str_arg;
            for (i++; i < trimmed.length()-1; ++i) { // absuing for loop(!)
                if (trimmed[i] == '"') {
                    is_text = !is_text;
                }

                if (is_text) {
                    cur_str_arg += trimmed[i];
                    continue;
                }

                if (bracket_level == 0 && trimmed[i] == ',') {
                    str_args.push_back(cur_str_arg);
                    cur_str_arg = "";
                } else {
                    cur_str_arg += trimmed[i];
                }

                if (trimmed[i] == '(') bracket_level++;
                else if (trimmed[i] == ')') {
                    bracket_level--;
                    if (bracket_level < 0) throw parse_exception(str, "bracket unmatched");
                }

            }
            if (bracket_level == 0) {
                if (cur_str_arg.size() != 0 && boost::trim_copy(cur_str_arg) != "") {
                    str_args.push_back(cur_str_arg);
                }
                std::vector<parse_expr> arg(str_args.size());
                bool success = true;
                for (int i = 0; i < str_args.size(); ++i) {
                    try {
                        arg[i] = parse(str_args[i]);
                    } catch (parse_exception e) {
                        success = false;
                        break;
                    }
                }

                if (success) {
                    return std::make_shared<expression::function>(func_name, arg);
                }
            }
        }
    }

    // Operator
    int bracket_level = 0;
    bool is_text = false;
    // Index of the operator which has smallest precedence, i is begin, j is end
    int min_i = -1;
    int min_j = -1;
    int min_precedence = -1;
    for (int i = 0; i < trimmed.size(); ++i) {
        if (trimmed[i] == '"') {
            is_text = !is_text;
        }

        if (is_text) continue;

        if (bracket_level == 0) {
            // try current and next character (e.g. operators <=)
            if (i < trimmed.size()-1) {
                int precedence2 = get_precedence(trimmed.substr(i, 2));
                if (precedence2 != -1 && (min_precedence == -1 || precedence2 <= min_precedence)) {
                    min_i = i;
                    min_j = i+2;
                    min_precedence = precedence2;
                    i++; // skip the next character
                    continue; // no need to check bracket right?
                }
            }
            int precedence1 = get_precedence(trimmed.substr(i, 1));
            if (precedence1 != -1 && (min_precedence == -1 || precedence1 <= min_precedence)) {
                min_i = i;
                min_j = i+1;
                min_precedence = precedence1;
            }
        }

        if (trimmed[i] == '(') bracket_level++;
        else if (trimmed[i] == ')') {
            bracket_level--;
            if (bracket_level < 0) throw parse_exception(str, "bracket unmatched");
        }
    }

    if (min_precedence != -1) {
        try {
            std::vector<parse_expr> args;
            if (!(is_unary(trimmed.substr(min_i, min_j - min_i)) && boost::trim_copy(trimmed.substr(0, min_i)) == "")) {
                args.push_back(parse(trimmed.substr(0, min_i)));
            }
            args.push_back(parse(trimmed.substr(min_j)));
            return std::make_shared<expression::function>(trimmed.substr(min_i, min_j - min_i), args);
        } catch (parse_exception e) {
            throw e;
        }
    }

    throw parse_exception(str, "does not match any expression types");
}

expression::eval_expr expression::primitive::evaluate() const { return shared_from_this(); }

template<typename T>
inline bool cast_and_compare(expression::eval_expr& left, expression::eval_expr& right) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++20-extensions"
    return dynamic_pointer_cast<const T>(left)->raw == dynamic_pointer_cast<const T>(right)->raw;
#pragma GCC diagnostic pop
}

bool operator == (expression::eval_expr& left, expression::eval_expr& right) {
    if (left->get_type() != right->get_type()) return false;
    switch (left->get_type()) {
        case 1: return cast_and_compare<expression::integer>(left, right);
        case 2: return cast_and_compare<expression::text>(left, right);
        case 3: return cast_and_compare<expression::boolean>(left, right);
        case 4: return cast_and_compare<expression::error>(left, right);
        default: return false;
    }
}
bool operator != (expression::eval_expr& left, expression::eval_expr& right) {
    return !(left == right);
}

std::string expression::integer::debug_message() const {
    return "integer(" + std::to_string(raw) + ")";
}
std::string expression::integer::cell_value(int width) const {
    std::string full = std::to_string(raw);
    if (full.length() <= width) {
        full.insert(0, width-full.size(), ' ');
        return full;
    }
    int unsigned_width = width - ((raw >= 0) ? 0 : 1);
    std::string sign = (raw >= 0) ? "" : "-";
    // Cases for scientific notation
    // -----------------------------
    // Less width:    ###
    // Minimal width: 1E+10
    // With decimal:  1.2E+10
    // More space:    1.234E+10
    int exp = full.size() - 1;
    std::string exp_str = std::to_string(exp);
    if (unsigned_width < 3+exp_str.length()) {
        // 3 refers to most significant digit, 'E' and '+'.
        return std::string(width, '#');
    } else if (unsigned_width < 5+exp_str.length()) {
        // 5 refers to most significant digit, '.', second most significant digit, 'E' and '+'.
        std::string res = sign +
            std::string(1, full[0]) +
            "E+" +
            exp_str;
        if (res.size() < width) res.insert(0, width-res.size(), ' ');
        return res;
    } else {
        std::string res = sign +
            std::string(1, full[0]) +
            "." +
            full.substr(1, unsigned_width-4-exp_str.length()) +
            "E+" +
            exp_str;
        if (res.size() < width) res.insert(0, width-res.size(), ' ');
        return res;
    }
}

std::string expression::text::debug_message() const {
    return "text(" + raw + ")";
}
std::string expression::text::cell_value(int width) const {
    std::string res = raw.substr(0, std::min(width, (int)raw.length()));
    if (res.size() < width) {
        res.insert(res.length(), width-res.length(), ' ');
    }
    return res;
}

std::string expression::boolean::debug_message() const {
    return std::string("boolean(") + (raw ? "TRUE" : "FALSE") + ")";
}
std::string expression::error::to_string() const {
    switch (raw) {
        case values::arg: return "#ARG!";
        case values::value: return "#VALUE!";
        case values::div0: return "#DIV/0!";
        case values::name: return "#NAME!";
        case values::recur: return "#RECUR!";
    }
}
std::string expression::error::debug_message() const {
    return "error(" + to_string() + ")";
}
std::string expression::error::cell_value(int width) const {
    std::string content = to_string();
    if (content.length() > width) return std::string(width, '#');
    return std::string((width-content.length())/2, ' ') + content + std::string(width - (width - content.length())/2 - content.length(), ' ');
}
std::string expression::boolean::cell_value(int width) const {
    std::string content = raw ? "TRUE" : "FALSE";
    if (content.length() > width) return std::string(width, '#');
    return std::string((width-content.length())/2, ' ') + content + std::string(width - (width - content.length())/2 - content.length(), ' ');
}

expression::eval_expr expression::reference::evaluate() const {
    worksheet::cell& cell = workspace::ws.cells[ref];
    return cell.calculate();
}
std::string expression::reference::debug_message() const {
    return "reference(" + std::to_string(ref.row.number) + ", " + std::to_string(ref.col.number) + ")";
}

expression::function::raw expression::function::lookup(std::string name) {
    for (char& c : name) c = std::toupper(c);
    if (name == "+") return op_add;
    else if (name == "-") return op_minus;
    else if (name == "*") return op_multiply;
    else if (name == "/") return op_divide;
    else if (name == "&") return op_concat;
    else if (name == "=") return op_eq;
    else if (name == "<>") return op_neq;
    else if (name == "<") return op_less;
    else if (name == "<=") return op_leq;
    else if (name == ">") return op_greater;
    else if (name == ">=") return op_geq;
    else if (name == "SUM") return sum;
    else if (name == "IF") return if_func;
    else throw std::make_shared<error>(error::values::name);
}
expression::eval_expr expression::function::evaluate() const {
    return lookup(name)(arg);
}
std::string expression::function::debug_message() const {
    std::ostringstream stream;
    stream << "function(" << name << ", [";
    for (const std::shared_ptr<expression>& exp : arg) {
        stream << exp->debug_message() << ", ";
    }
    stream << "])";

    return stream.str();
}

template<typename T>
inline std::shared_ptr<const T> cast_or_throw(const expression::eval_expr& x) {
    if (!x->is_type<T>()) throw std::make_shared<expression::error>(expression::error::values::value);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++20-extensions"
    return dynamic_pointer_cast<const T>(x);
#pragma GCC diagnostic pop
}

inline void arg_size_check(const std::vector<std::shared_ptr<expression>>& arg, size_t size) {
    if (arg.size() != size) throw std::make_shared<expression::error>(expression::error::values::arg);
}
inline std::vector<expression::eval_expr> arg_evaluate(const std::vector<std::shared_ptr<expression>>& arg) {
    std::vector<expression::eval_expr> evaluated(arg.size());
    for (int i=0; i<arg.size(); ++i) {
        evaluated[i] = arg[i]->evaluate();
    }
    return evaluated;
}

#define EXPRESSION_FUNCTION_IMPLEMENTATION(name) expression::eval_expr expression::function::name(const std::vector<std::shared_ptr<expression>>& arg)

EXPRESSION_FUNCTION_IMPLEMENTATION(op_add) {
    auto evaluated = arg_evaluate(arg);
    if (arg.size() == 1) {
        return std::make_shared<integer>(cast_or_throw<integer>(evaluated[0])->raw);
    } else if (arg.size() == 2) {
        return std::make_shared<integer>(cast_or_throw<integer>(evaluated[0])->raw + cast_or_throw<integer>(evaluated[1])->raw);
    } else {
        throw std::make_shared<expression::error>(expression::error::values::arg);
    }
}
EXPRESSION_FUNCTION_IMPLEMENTATION(op_minus) {
    auto evaluated = arg_evaluate(arg);
    if (arg.size() == 1) {
        return std::make_shared<integer>(-cast_or_throw<integer>(evaluated[0])->raw);
    } else if (arg.size() == 2) {
        return std::make_shared<integer>(cast_or_throw<integer>(evaluated[0])->raw - cast_or_throw<integer>(evaluated[1])->raw);
    } else {
        throw std::make_shared<expression::error>(expression::error::values::arg);
    }
}
EXPRESSION_FUNCTION_IMPLEMENTATION(op_multiply) {
    arg_size_check(arg, 2);
    auto evaluated = arg_evaluate(arg);
    return std::make_shared<integer>(cast_or_throw<integer>(evaluated[0])->raw * cast_or_throw<integer>(evaluated[1])->raw);
}
EXPRESSION_FUNCTION_IMPLEMENTATION(op_divide) {
    arg_size_check(arg, 2);
    auto evaluated = arg_evaluate(arg);
    return std::make_shared<integer>(cast_or_throw<integer>(evaluated[0])->raw / cast_or_throw<integer>(evaluated[1])->raw);
}
EXPRESSION_FUNCTION_IMPLEMENTATION(op_concat) {
    arg_size_check(arg, 2);
    auto evaluated = arg_evaluate(arg);
    return std::make_shared<text>(cast_or_throw<text>(evaluated[0])->raw + cast_or_throw<text>(evaluated[1])->raw);
}
#define equality_operator_case(op,typeid,typename) \
    case typeid: \
        return std::make_shared<boolean>(evaluated[1]->get_type() == typeid && cast_or_throw<typename>(evaluated[0])->raw op cast_or_throw<typename>(evaluated[1])->raw);
#define equality_operator(op) \
    auto evaluated = arg_evaluate(arg); \
    switch (evaluated[0]->get_type()) { \
        equality_operator_case(op,1,integer) \
        equality_operator_case(op,2,text) \
        equality_operator_case(op,3,boolean) \
        default: throw std::make_shared<expression::error>(expression::error::values::value);\
    }
EXPRESSION_FUNCTION_IMPLEMENTATION(op_eq) {
    equality_operator(==)
}
EXPRESSION_FUNCTION_IMPLEMENTATION(op_neq) {
    equality_operator(!=)
}
EXPRESSION_FUNCTION_IMPLEMENTATION(op_less) {
    equality_operator(<)
}
EXPRESSION_FUNCTION_IMPLEMENTATION(op_leq) {
    equality_operator(<=)
}
EXPRESSION_FUNCTION_IMPLEMENTATION(op_greater) {
    equality_operator(>)
}
EXPRESSION_FUNCTION_IMPLEMENTATION(op_geq) {
    equality_operator(>=)
}
EXPRESSION_FUNCTION_IMPLEMENTATION(sum) {
    int64_t ans = 0;
    for (const std::shared_ptr<expression>& x : arg) {
        ans += cast_or_throw<integer>(x->evaluate())->raw;
    }
    return std::make_shared<integer>(ans);
}
EXPRESSION_FUNCTION_IMPLEMENTATION(if_func) {
    arg_size_check(arg, 3);
    if (cast_or_throw<boolean>(arg[0]->evaluate())->raw) {
        return arg[1]->evaluate();
    } else {
        return arg[2]->evaluate();
    }
}
