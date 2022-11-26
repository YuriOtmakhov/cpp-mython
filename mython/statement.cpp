#include "statement.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    return closure[name_] = rvalue_->Execute(closure, context);
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv) : name_(var), rvalue_(std::move(rv)) {
}

VariableValue::VariableValue(const std::string& var_name) : dotted_ids_({var_name}) {
}

VariableValue::VariableValue(std::vector<std::string> dotted_ids) : dotted_ids_(dotted_ids) {
}

ObjectHolder VariableValue::Execute(Closure& closure, Context& /*context*/) {
    auto local_closure = closure;
    for (auto it = dotted_ids_.begin(); it != std::prev(dotted_ids_.end()); ++it)
        if (local_closure.count(*it)) {

            if ( auto ptr = local_closure.at(*it).TryAs<runtime::ClassInstance>(); ptr )
               local_closure = ptr->Fields();
            else
                throw std::runtime_error("not definition var"s);
        } else
            throw std::runtime_error("not definition var"s);

    if (local_closure.count(dotted_ids_.back()))
        return local_closure.at(dotted_ids_.back());
    else
        throw std::runtime_error("not definition var"s);
}

unique_ptr<Print> Print::Variable(const std::string& name) {
    auto new_print_ptn = make_unique<Print>(vector<unique_ptr<Statement>>{});
    new_print_ptn->args_ = name;
    return new_print_ptn;
}

Print::Print(unique_ptr<Statement> argument) /*: args_({std::move(argument)})*/ {
    vector<unique_ptr<Statement>> tmp;
    tmp.emplace_back(std::move(argument));
    args_ = std::move(tmp);
}

Print::Print(vector<unique_ptr<Statement>> args) : args_(std::move(args)) {
}

ObjectHolder Print::Execute(Closure& closure, Context& context) {
    if (std::holds_alternative<std::string>(args_)) {
        if ((bool)closure.at(std::get<std::string>(args_)))
            closure.at(std::get<std::string>(args_))->Print(context.GetOutputStream(),context);
    }else {
        bool is_first = true;
        for(auto& item : std::get<std::vector<std::unique_ptr<Statement>>>(args_)){
            if (is_first)
                is_first = false;
            else
                context.GetOutputStream()<<' ';

            if (auto obj = item->Execute(closure,context); (bool)obj)
                obj->Print(context.GetOutputStream(),context);
            else
                context.GetOutputStream()<<"None"s;
        }
    }
    context.GetOutputStream()<<'\n';
    return ObjectHolder::None();
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args) : object_(std::move(object)), method_(std::move(method)),args_(std::move(args)) {
}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    // Вызывает метод object.method со списком параметров args
    auto class_ptr = object_->Execute(closure,context).TryAs<runtime::ClassInstance>();
    if (class_ptr)
        if (class_ptr->HasMethod(method_, args_.size())) {
            std::vector<ObjectHolder>actual_args;
            actual_args.reserve(args_.size());
            for (auto& item : args_)
                actual_args.emplace_back(item->Execute(closure,context));

            return class_ptr->Call(method_, actual_args, context);
        }

    throw std::runtime_error("method not found"s);
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    auto obj = argument_->Execute(closure, context);
    stringstream str;
    if (obj)
        obj->Print(str, context);
    else
        str << "None"s;

    return ObjectHolder::Own(runtime::String(str.str()));
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    // Заглушка. Реализуйте метод самостоятельно
    // Поддерживается сложение:
    //  число + число
    //  строка + строка
    //  объект1 + объект2, если у объект1 - пользовательский класс с методом _add__(rhs)
    // В противном случае при вычислении выбрасывается runtime_error
    auto lhs = lhs_->Execute(closure,context);
    auto rhs = rhs_->Execute(closure,context);
    if (auto l_ptr = lhs.TryAs<runtime::Number>(); l_ptr)
        if (auto r_ptr = rhs.TryAs<runtime::Number>(); r_ptr)
            return ObjectHolder::Own( runtime::Number( l_ptr->GetValue() +  r_ptr->GetValue() ) );

    if (auto l_ptr = lhs.TryAs<runtime::String>(); l_ptr)
        if (auto r_ptr = rhs.TryAs<runtime::String>(); r_ptr)
            return ObjectHolder::Own( runtime::String( l_ptr->GetValue() +  r_ptr->GetValue() ) );

    if (auto l_ptr = lhs.TryAs<runtime::ClassInstance>(); l_ptr)
        if (/*auto r_ptr = rhs.TryAs<runtime::ClassInstance>(); r_ptr &&*/ l_ptr->HasMethod(ADD_METHOD, 1))
            return l_ptr->Call(ADD_METHOD, {rhs} ,context);

    throw std::runtime_error("incorrect Add operands"s);
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    // Поддерживается вычитание:
    //  число - число
    // Если lhs и rhs - не числа, выбрасывается исключение runtime_error
    auto lhs = lhs_->Execute(closure,context);
    auto rhs = rhs_->Execute(closure,context);
    if (auto l_ptr = lhs.TryAs<runtime::Number>(); l_ptr)
        if (auto r_ptr = rhs.TryAs<runtime::Number>(); r_ptr)
            return ObjectHolder::Own( runtime::Number( l_ptr->GetValue() -  r_ptr->GetValue() ) );

    throw std::runtime_error("incorrect Sub operands"s);
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    // Поддерживается умножение:
    //  число * число
    // Если lhs и rhs - не числа, выбрасывается исключение runtime_error
    auto lhs = lhs_->Execute(closure,context);
    auto rhs = rhs_->Execute(closure,context);
    if (auto l_ptr = lhs.TryAs<runtime::Number>(); l_ptr)
        if (auto r_ptr = rhs.TryAs<runtime::Number>(); r_ptr)
            return ObjectHolder::Own( runtime::Number( l_ptr->GetValue() *  r_ptr->GetValue() ) );

    throw std::runtime_error("incorrect Mult operands"s);
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    // Поддерживается деление:
    //  число / число
    // Если lhs и rhs - не числа, выбрасывается исключение runtime_error
    // Если rhs равен 0, выбрасывается исключение runtime_error
    auto lhs = lhs_->Execute(closure,context);
    auto rhs = rhs_->Execute(closure,context);
    if (auto l_ptr = lhs.TryAs<runtime::Number>(); l_ptr)
        if (auto r_ptr = rhs.TryAs<runtime::Number>(); r_ptr && r_ptr->GetValue())
            return ObjectHolder::Own( runtime::Number( l_ptr->GetValue() / r_ptr->GetValue() ) );

    throw std::runtime_error("incorrect Div operands"s);
}

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for(auto& item : statement_)
        item->Execute(closure,context);
    return ObjectHolder::None();
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    throw statement_->Execute(closure,context);
    return {};
}

ClassDefinition::ClassDefinition(ObjectHolder cls) : class_(cls) {
}

ObjectHolder ClassDefinition::Execute(Closure& closure, Context& /*context*/) {
    // Создаёт внутри closure новый объект, совпадающий с именем класса и значением, переданным в
    // конструктор
    closure[class_.TryAs<runtime::Class>()->GetName()] = class_;

    return ObjectHolder::None();
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv) : object_(object), field_name_(field_name), rvalue_(std::move(rv)) {
}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    //if(closure.count(object_.Execute(closure, context)))
//        return /*closure[*/object_.Execute(closure, context)/*]*/.TryAs<runtime::ClassInstance>()->Fields()[field_name_] = rvalue_->Execute(closure, context);
    if(auto item_ptr = object_.Execute(closure, context).TryAs<runtime::ClassInstance>(); item_ptr /*&& field_name_ != "self"s*/)
        return item_ptr->Fields()[field_name_] = rvalue_->Execute(closure, context);
//    else
//        item->Print(std::cerr, context);
//    return {};
    throw std::runtime_error("Class has not self"s);
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body) : condition_(std::move(condition)), if_body_(std::move(if_body)), else_body_ (std::move(else_body)) {
}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    if (runtime::IsTrue(condition_->Execute(closure,context)))
        return if_body_->Execute(closure,context);
    if (else_body_)
        return else_body_->Execute(closure,context);

    return ObjectHolder::None();
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
    // Значение аргумента rhs вычисляется, только если значение lhs
    // после приведения к Bool равно False

    bool lhs = runtime::IsTrue(lhs_->Execute(closure,context));
    if (lhs)
        return ObjectHolder::Own( runtime::Bool( true ) );

    bool rhs = runtime::IsTrue(rhs_->Execute(closure,context));
    return ObjectHolder::Own( runtime::Bool( rhs ) );
}

ObjectHolder And::Execute(Closure& closure, Context& context) {
    // Заглушка. Реализуйте метод самостоятельно
    // Значение аргумента rhs вычисляется, только если значение lhs
    // после приведения к Bool равно True
    bool lhs = runtime::IsTrue(lhs_->Execute(closure,context));
    if (!lhs)
        return ObjectHolder::Own( runtime::Bool( false ) );

    bool rhs = runtime::IsTrue(rhs_->Execute(closure,context));
    return ObjectHolder::Own( runtime::Bool( rhs ) );
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    bool arg = runtime::IsTrue(argument_->Execute(closure,context));
    return ObjectHolder::Own( runtime::Bool( !arg ) );
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs)), cmp_(cmp) {

}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {

    return ObjectHolder::Own( runtime::Bool(cmp_(lhs_->Execute(closure,context),
                                                rhs_->Execute(closure,context),
                                                context)) );
}

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args) : new_object_class_(class_), args_(std::move(args)) {

}

NewInstance::NewInstance(const runtime::Class& class_) : new_object_class_(class_) {
}

ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
    auto new_object_ = ObjectHolder::Own(runtime::ClassInstance(new_object_class_));
    if(auto ptr_class = new_object_.TryAs<runtime::ClassInstance>(); ptr_class->HasMethod(INIT_METHOD, args_.size())){
        std::vector<ObjectHolder> vector_args;
        vector_args.reserve(args_.size());
        for(auto&& item : args_)
            vector_args.push_back(item->Execute(closure, context));

       ptr_class->Call(INIT_METHOD,vector_args,context);
    }
    return new_object_;//ObjectHolder::Own(std::move(new_object_));
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body) : body_(std::move(body)) {
}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    // Вычисляет инструкцию, переданную в качестве body.
    // Если внутри body была выполнена инструкция return, возвращает результат return
    // В противном случае возвращает None
    try {
        body_->Execute(closure,context);
    }
    catch (ObjectHolder& ans) {
        return ans;
    }

    return ObjectHolder::None();
}

}  // namespace ast
