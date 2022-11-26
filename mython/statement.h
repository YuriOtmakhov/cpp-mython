#pragma once

#include "runtime.h"

#include <functional>
#include <variant>

namespace ast {

using Statement = runtime::Executable;

template <typename T>
class ValueStatement : public Statement {
public:
    explicit ValueStatement(T v)
        : value_(std::move(v)) {
    }

    runtime::ObjectHolder Execute(runtime::Closure& /*closure*/,
                                  runtime::Context& /*context*/) override {
        return runtime::ObjectHolder::Share(value_);
    }

private:
    T value_;
};

using NumericConst = ValueStatement<runtime::Number>;
using StringConst = ValueStatement<runtime::String>;
using BoolConst = ValueStatement<runtime::Bool>;

class VariableValue : public Statement {
    std::vector<std::string> dotted_ids_;

public:
    explicit VariableValue(const std::string& var_name);
    explicit VariableValue(std::vector<std::string> dotted_ids);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Assignment : public Statement {
    std::string name_;
    std::unique_ptr<Statement> rvalue_;
public:
    Assignment(std::string var, std::unique_ptr<Statement> rv);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class FieldAssignment : public Statement {
    VariableValue object_;
    std::string field_name_;
    std::unique_ptr<Statement> rvalue_;

public:
    FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class None : public Statement {
public:
    runtime::ObjectHolder Execute([[maybe_unused]] runtime::Closure& closure,
                                  [[maybe_unused]] runtime::Context& context) override {
        return runtime::ObjectHolder::None();
    }
};

class Print : public Statement {

std::variant<std::string , std::vector<std::unique_ptr<Statement>>> args_;

public:

    explicit Print(std::unique_ptr<Statement> argument);
    explicit Print(std::vector<std::unique_ptr<Statement>> args);

    static std::unique_ptr<Print> Variable(const std::string& name);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class MethodCall : public Statement {
std::unique_ptr<Statement> object_;
std::string method_;
std::vector<std::unique_ptr<Statement>> args_;

public:
    MethodCall(std::unique_ptr<Statement> object, std::string method,
               std::vector<std::unique_ptr<Statement>> args);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class NewInstance : public Statement {

const runtime::Class& new_object_class_;
std::vector<std::unique_ptr<Statement>> args_;

public:
    explicit NewInstance(const runtime::Class& class_);
    NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};


class UnaryOperation : public Statement {
protected:
std::unique_ptr<Statement> argument_;
public:
    explicit UnaryOperation(std::unique_ptr<Statement> argument) : argument_(std::move(argument)) {
    }
};

class Stringify : public UnaryOperation {
public:
    using UnaryOperation::UnaryOperation;
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class BinaryOperation : public Statement {
protected:
std::unique_ptr<Statement> lhs_;
std::unique_ptr<Statement> rhs_;
public:
    BinaryOperation(std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs) : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
    }
};

class Add : public BinaryOperation {
public:
    using BinaryOperation::BinaryOperation;

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};


class Sub : public BinaryOperation {
public:
    using BinaryOperation::BinaryOperation;

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Mult : public BinaryOperation {
public:
    using BinaryOperation::BinaryOperation;

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Div : public BinaryOperation {
public:
    using BinaryOperation::BinaryOperation;

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Or : public BinaryOperation {
public:
    using BinaryOperation::BinaryOperation;

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class And : public BinaryOperation {
public:
    using BinaryOperation::BinaryOperation;

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Not : public UnaryOperation {
public:
    using UnaryOperation::UnaryOperation;
    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Compound : public Statement {

std::vector<std::unique_ptr<Statement>> statement_;

public:

    void AddStatement(std::unique_ptr<Statement> stmt) {
        statement_.emplace_back(std::move(stmt));
    }

    template <typename... Args>
    explicit Compound(Args&&... args) {
        (... , AddStatement(std::move(args)));
    }

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class MethodBody : public Statement {

std::unique_ptr<Statement> body_;

public:
    explicit MethodBody(std::unique_ptr<Statement>&& body);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Return : public Statement {

std::unique_ptr<Statement> statement_;

public:
    explicit Return(std::unique_ptr<Statement> statement) : statement_(std::move(statement)) {
    }

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class ClassDefinition : public Statement {

runtime::ObjectHolder class_;

public:

    explicit ClassDefinition(runtime::ObjectHolder cls);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class IfElse : public Statement {

std::unique_ptr<Statement> condition_;
std::unique_ptr<Statement> if_body_;
std::unique_ptr<Statement> else_body_;

public:
    IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
           std::unique_ptr<Statement> else_body);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

class Comparison : public BinaryOperation {
using Comparator = std::function<bool(const runtime::ObjectHolder&,
                                        const runtime::ObjectHolder&, runtime::Context&)>;

Comparator cmp_;

public:

    Comparison(Comparator cmp, std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs);

    runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
};

}  // namespace ast
