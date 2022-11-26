#include "runtime.h"

#include <cassert>
#include <optional>

using namespace std;

namespace runtime {

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder& object) {
    if (!object)
        return false;

    if (auto ptn = object.TryAs<Bool>(); ptn != nullptr)
        return ptn->GetValue();
    if (auto ptn = object.TryAs<Number>(); ptn != nullptr)
        return ptn->GetValue();
    if (auto ptn = object.TryAs<String>(); ptn != nullptr)
        return !ptn->GetValue().empty();

    return false;
}

void ClassInstance::Print(std::ostream& os, Context& context) {
    if (HasMethod("__str__"s, 0))
        Call ("__str__"s, {},context)->Print(os, context);
    else
        os << this;
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    auto ptr_metod = class_.GetMethod(method);
    if (ptr_metod != nullptr)
        if (ptr_metod->formal_params.size() == argument_count)
            return true;
    return false;
}

Closure& ClassInstance::Fields() {
    return closure_;
}

const Closure& ClassInstance::Fields() const {
    return closure_;
}

ClassInstance::ClassInstance(const Class& cls) : class_(cls) {
}

ObjectHolder ClassInstance::Call(const std::string& method,
                                 const std::vector<ObjectHolder>& actual_args,
                                 Context& context) {
    if (!HasMethod(method, actual_args.size()))
        throw std::runtime_error("Not method"s);

    auto method_ptr = class_.GetMethod(method);
    Closure args_closure;
    args_closure["self"s] = ObjectHolder::Share(*this);
    for (auto name_ptr = method_ptr->formal_params.begin(); name_ptr != method_ptr->formal_params.end(); ++name_ptr)
        args_closure[*name_ptr] = (actual_args[std::distance(method_ptr->formal_params.begin(), name_ptr)]);

    return method_ptr->body->Execute(args_closure,context);
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent) : name_(name), parent_(parent) {
    for (auto & item : methods)
        methods_[item.name] = std::move(item);
}

const Method* Class::GetMethod(const std::string& name) const {
    if (methods_.count(name))
        return &methods_.at(name);
    if (parent_ != nullptr)
        return parent_->GetMethod(name);
    return nullptr;
}

[[nodiscard]] const std::string& Class::GetName() const {
    return name_;
}

void Class::Print(ostream& os, Context&) {
    os << "Class "sv << GetName();
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if (auto ptn_l = lhs.TryAs<Bool>(); ptn_l != nullptr )
        if (auto ptn_r = rhs.TryAs<Bool>(); ptn_r != nullptr )
            return ptn_l->GetValue() == ptn_r->GetValue();

    if (auto ptn_l = lhs.TryAs<Number>(); ptn_l != nullptr )
        if (auto ptn_r = rhs.TryAs<Number>(); ptn_r != nullptr )
            return ptn_l->GetValue() == ptn_r->GetValue();

    if (auto ptn_l = lhs.TryAs<String>(); ptn_l != nullptr )
        if (auto ptn_r = rhs.TryAs<String>(); ptn_r != nullptr )
            return ptn_l->GetValue() == ptn_r->GetValue();

    if (auto ptn_l = lhs.TryAs<ClassInstance>(); ptn_l != nullptr )
        if (ptn_l->HasMethod("__eq__"s, 1))
            return ptn_l->Call("__eq__"s, {rhs}, context).TryAs<Bool>()->GetValue();

    if ( !(bool)lhs && !(bool)rhs )
        return true;

    throw std::runtime_error("Cannot compare objects for equality"s);
}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if (auto ptn_l = lhs.TryAs<Bool>(); ptn_l != nullptr )
        if (auto ptn_r = rhs.TryAs<Bool>(); ptn_r != nullptr )
            return ptn_l->GetValue() < ptn_r->GetValue();

    if (auto ptn_l = lhs.TryAs<Number>(); ptn_l != nullptr )
        if (auto ptn_r = rhs.TryAs<Number>(); ptn_r != nullptr )
            return ptn_l->GetValue() < ptn_r->GetValue();

    if (auto ptn_l = lhs.TryAs<String>(); ptn_l != nullptr )
        if (auto ptn_r = rhs.TryAs<String>(); ptn_r != nullptr )
            return ptn_l->GetValue() < ptn_r->GetValue();

    if (auto ptn_l = lhs.TryAs<ClassInstance>(); ptn_l != nullptr )
        if (ptn_l->HasMethod("__lt__"s, 1))
            return ptn_l->Call("__lt__"s, {rhs}, context).TryAs<Bool>()->GetValue();

    throw std::runtime_error("Cannot compare objects for less"s);
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Equal(lhs,rhs,context);
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs,context) && NotEqual(lhs, rhs,context);
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Greater(lhs,rhs,context);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs,rhs,context);
}

}  // namespace runtime
