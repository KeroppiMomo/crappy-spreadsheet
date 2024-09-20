#ifndef __INCLUDE_EXPRESSION_
#define __INCLUDE_EXPRESSION_

#include "worksheet_reference.h"
#include <string>
#include <map>
#include <vector>
#include <sstream>

/**
 * Define the evaluation of expressions in formulas.
 */
struct expression {
    struct primitive;
    struct integer;
    struct text;
    struct boolean;
    struct error;
    struct compound;
    struct function;
    struct reference;
    struct parse_exception;

    /**
     * Evaluated expression type.
     */
    typedef std::shared_ptr<const primitive> eval_expr;

    /**
     * Parsed expression type.
     */
    typedef std::shared_ptr<expression> parse_expr;

    /**
     * Evaluate the expression.
     */
    virtual eval_expr evaluate() const = 0;

    /**
     * Parse an expresion text.
     *
     * @throws expresion::parse_exception Thrown if the string cannot be parsed.
     */
    static parse_expr parse(const std::string& str) noexcept(false);

    /**
     * Generate a text representation of the expression tree.
     */
    virtual std::string debug_message() const noexcept = 0;

    template<class exp>
    friend std::ostream& operator<<(std::ostream& os, const std::shared_ptr<exp> self);
};

/**
 * Thrown in `expression::evaluate` if the expression text is invalid.
 */
struct expression::parse_exception: std::exception {
    /**
     * The currently attempted expression to be parsed.
     */
    std::string attempt;
    std::string message;
    parse_exception(std::string attempt, std::string message): attempt(attempt), message(message) {}
    const char* what() const noexcept override {
        return message.c_str();
        // stream << "Attempt to parse '" << attempt << "' but failed with message '" << message << "'.";
        // return stream.str();
    }
};

#define primitive_get_type int8_t get_type() const override { return type; } \
static_assert(true, "require semi-colon after macro, https://stackoverflow.com/a/59153563/10845353")
/**
 * A primitive expression represents data with basic types and is reduced to its simplest form.
 */
struct expression::primitive: expression, std::enable_shared_from_this<expression::primitive> {
    /**
     * Type ID of a primitive type.
     */
    static const int8_t type = 0;
    /**
     * Get the type ID of a primitive expression instance.
     */
    virtual int8_t get_type() const { return type; }

    /**
     * Returns whether the primitive expression is of a particular type.
     */
    template<typename T>
    bool is_type() const {
        return this->get_type() == T::type;
    }

    /**
     * Evaluate the expression.
     *
     * Since a primitive expression is already evaluated, this just returns a
     * pointer to itself.
     */
    eval_expr evaluate() const override;

    /**
     * Generate a text representation of the expression tree for debug purpose.
     */
    virtual std::string debug_message() const noexcept override = 0;
    /**
     * Generate a text representation of the evaluated expression to be
     * displayed in a worksheet cell.
     *
     * @param width Width the the worksheet cell.
     */
    virtual std::string cell_value(int width) const noexcept = 0;

    friend bool operator == (std::shared_ptr<const primitive>& left, std::shared_ptr<const primitive>& right);
    friend bool operator != (std::shared_ptr<const primitive>& left, std::shared_ptr<const primitive>& right);
};
/**
 * An integer expression.
 */
struct expression::integer: expression::primitive {
    static const int8_t type = 1;
    primitive_get_type;
    int64_t raw;
    integer(int64_t raw): raw(raw) {};
    std::string debug_message() const noexcept override;
    std::string cell_value(int width) const noexcept override;
};
/**
 * A text expression.
 */
struct expression::text: expression::primitive {
    static const int8_t type = 2;
    primitive_get_type;
    std::string raw;
    text(std::string raw): raw(raw) {};
    std::string debug_message() const noexcept override;
    std::string cell_value(int width) const noexcept override;
};
/**
 * A boolean expression.
 */
struct expression::boolean: expression::primitive {
    static const int8_t type = 3;
    primitive_get_type;
    bool raw;
    boolean(bool raw): raw(raw) {};
    std::string debug_message() const noexcept override;
    std::string cell_value(int width) const noexcept override;
};
/**
 * An runtime error expression.
 */
struct expression::error: expression::primitive {
    static const int8_t type = 4;
    primitive_get_type;

    /**
     * Possible error values.
     */
    enum struct values {
        /**
         * Represent an error related to function arguments, such as invalid
         * arguments and wrong number of arguments.
         */
        arg,
        /**
         * Represent an error related to numeric values, such as failure to
         * convert to a numeric value.
         */
        value,
        /**
         * Represent a division by zero erro.
         */
        div0,
        /**
         * Represent an error in function name lookup.
         */
        name,
        /**
         * Represent an recurring reference error.
         */
        recur
    };
    values raw;
    error(values raw): raw(raw) {}
    std::string to_string() const;
    std::string debug_message() const noexcept override;
    std::string cell_value(int width) const noexcept override;
};

/**
 * A compound expression depends on the workspace or
 * other expressions and requires evluation.
 */
struct expression::compound: expression {};
struct expression::reference: expression {
    struct not_evaluated_exception;
    std::shared_ptr<const primitive> evaluate() const override;
    worksheet_reference::cell_reference ref;
    reference(worksheet_reference::cell_reference ref): ref(ref) {}
    std::string debug_message() const noexcept override;
};
struct expression::function: expression {
    typedef std::function<std::shared_ptr<const primitive>(const std::vector<std::shared_ptr<expression>>&)> raw;
    static raw lookup(std::string name);

    std::string name;
    std::vector<std::shared_ptr<expression>> arg;
    std::shared_ptr<const primitive> evaluate() const override;

    function(std::string name, std::vector<std::shared_ptr<expression>> arg): name(name), arg(arg) { }

    std::string debug_message() const noexcept override;

    static std::shared_ptr<const primitive> op_add(const std::vector<std::shared_ptr<expression>>& arg);
    static std::shared_ptr<const primitive> op_minus(const std::vector<std::shared_ptr<expression>>& arg);
    static std::shared_ptr<const primitive> op_multiply(const std::vector<std::shared_ptr<expression>>& arg);
    static std::shared_ptr<const primitive> op_divide(const std::vector<std::shared_ptr<expression>>& arg);
    static std::shared_ptr<const primitive> op_concat(const std::vector<std::shared_ptr<expression>>& arg);
    static std::shared_ptr<const primitive> op_eq(const std::vector<std::shared_ptr<expression>>& arg);
    static std::shared_ptr<const primitive> op_neq(const std::vector<std::shared_ptr<expression>>& arg);
    static std::shared_ptr<const primitive> op_less(const std::vector<std::shared_ptr<expression>>& arg);
    static std::shared_ptr<const primitive> op_leq(const std::vector<std::shared_ptr<expression>>& arg);
    static std::shared_ptr<const primitive> op_greater(const std::vector<std::shared_ptr<expression>>& arg);
    static std::shared_ptr<const primitive> op_geq(const std::vector<std::shared_ptr<expression>>& arg);
    static std::shared_ptr<const primitive> sum(const std::vector<std::shared_ptr<expression>>& arg);
    static std::shared_ptr<const primitive> if_func(const std::vector<std::shared_ptr<expression>>& arg);
};

#endif
