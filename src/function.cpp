// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/function.h"
#include "script/private/function_p.h"

#include "script/cast.h"
#include "script/class.h"
#include "script/functionbuilder.h"
#include "script/literals.h"
#include "script/name.h"
#include "script/namespace.h"
#include "script/operator.h"
#include "script/script.h"

#include "script/program/expression.h"

#include "script/private/cast_p.h"
#include "script/private/class_p.h"
#include "script/private/literals_p.h"
#include "script/private/namespace_p.h"
#include "script/private/operator_p.h"
#include "script/private/script_p.h"

namespace script
{

/*!
 * \class Function
 * \brief The Function class represents a function.
 *
 */

DefaultArgumentList::DefaultArgumentList()
  : data(nullptr)
{

}

DefaultArgumentList::~DefaultArgumentList()
{
  data = nullptr;
}

bool DefaultArgumentList::isEmpty() const
{
  return data == nullptr || (data->size() == 0);
}

size_t DefaultArgumentList::size() const
{
  if (isEmpty())
    return 0;
  return get().size();
}

void DefaultArgumentList::push_back(const DefaultArgument & value)
{
  if (isEmpty())
    data.reset(new std::vector<DefaultArgument>{ value });
  else
    get().push_back(value);
}

void DefaultArgumentList::set(std::vector<DefaultArgument> && defargs)
{
  if (defargs.empty())
    data.reset();
  else
    data.reset(new std::vector<DefaultArgument>{ std::move(defargs) });
}

std::vector<DefaultArgument> & DefaultArgumentList::get()
{
  if (data == nullptr)
    throw std::runtime_error{ "Function has no default parameters" };
  return *data.get();
}

const std::vector<DefaultArgument> & DefaultArgumentList::get() const
{
  if (data == nullptr)
    throw std::runtime_error{ "Function has no default parameters" };
  return *data.get();
}


FunctionImpl::FunctionImpl(const Prototype &p, Engine *e, FunctionImpl::flag_type f)
  : prototype(p)
  , engine(e)
  , flags(f)
{

}

FunctionImpl::~FunctionImpl()
{

}

const std::string & FunctionImpl::name() const
{
  throw std::runtime_error{ "This kind of function does not implement name()" };
}

Name FunctionImpl::get_name() const
{
  throw std::runtime_error{ "This kind of function does not implement get_name()" };
}

void FunctionImpl::force_virtual()
{
  this->flags |= (Function::Virtual << 2);
}

void FunctionImpl::set_impl(NativeFunctionSignature callback)
{
  this->implementation.callback = callback;
  this->flags &= ~Function::InterpretedFunction;
  this->flags |= Function::NativeFunction;
}

void FunctionImpl::set_impl(const std::shared_ptr<program::Statement> program)
{
  this->implementation.program = program;
  this->flags &= ~Function::NativeFunction;
  this->flags |= Function::InterpretedFunction;
}

RegularFunctionImpl::RegularFunctionImpl(const std::string & name, const Prototype &p, Engine *e, FunctionImpl::flag_type f)
  : FunctionImpl(p, e, f)
  , mName(name)
{

}

Name RegularFunctionImpl::get_name() const
{
  return mName;
}


ScriptFunctionImpl::ScriptFunctionImpl(Engine *e)
  : FunctionImpl(Prototype{ Type::Void }, e)
{

}


ConstructorImpl::ConstructorImpl(const Prototype &p, Engine *e, FunctionImpl::flag_type f)
  : FunctionImpl(p, e, f)
{

}

Class ConstructorImpl::getClass() const
{
  return Symbol{ enclosing_symbol.lock() }.toClass();
}

const std::string & ConstructorImpl::name() const
{
  return getClass().name();
}

Name ConstructorImpl::get_name() const 
{
  return name();
}

bool ConstructorImpl::is_default_ctor() const
{
  return this->prototype.count() == 1;
}

bool ConstructorImpl::is_copy_ctor() const
{
  if (this->prototype.count() != 2)
    return false;
  return this->prototype.at(1) == Type::cref(getClass().id());
}

bool ConstructorImpl::is_move_ctor() const
{
  if (this->prototype.count() != 2)
    return false;
  return this->prototype.at(1) == Type::rref(getClass().id());
}

DestructorImpl::DestructorImpl(const Prototype & p, Engine *e, FunctionImpl::flag_type f)
  : FunctionImpl(p, e, f)
{

}

FunctionTemplateInstance::FunctionTemplateInstance(const FunctionTemplate & ft, const std::vector<TemplateArgument> & targs, const std::string & name, const Prototype &p, Engine *e, FunctionImpl::flag_type f)
  : RegularFunctionImpl(name, p, e, f)
  , mTemplate(ft)
  , mArgs(targs)
{

}

/// TODO: maybe move this to functionbuilder.cpp
std::shared_ptr<FunctionTemplateInstance> FunctionTemplateInstance::create(const FunctionTemplate & ft, const std::vector<TemplateArgument> & targs, const FunctionBuilder & builder)
{
  auto impl = std::make_shared<FunctionTemplateInstance>(ft, targs, builder.name, builder.proto, ft.engine(), builder.flags);
  impl->implementation.callback = builder.callback;
  impl->data = builder.data;
  impl->enclosing_symbol = ft.enclosingSymbol().impl();
  return impl;
}

Function::Function(const std::shared_ptr<FunctionImpl> & impl)
  : d(impl)
{

}

bool Function::isNull() const
{
  return d == nullptr;
}

const std::string & Function::name() const
{
  return d->name();
}

Name Function::getName() const
{
  return d->get_name();
}

const Prototype & Function::prototype() const
{
  return d->prototype;
}

const Type & Function::parameter(int index) const
{
  return prototype().at(index);
}

const Type & Function::returnType() const
{
  return prototype().returnType();
}

bool Function::accepts(int argc) const
{
  const int parameter_count = prototype().parameterCount();
  const int default_count = d->default_arguments.size();

  return parameter_count - default_count <= argc && argc <= parameter_count;
}

/*!
* \fun bool hasDefaultArguments() const
* \brief Returns whether the function has default arguments.
*/
bool Function::hasDefaultArguments() const
{
  return !d->default_arguments.isEmpty();
}

size_t Function::defaultArgumentCount() const
{
  return d->default_arguments.size();
}

void Function::addDefaultArgument(const std::shared_ptr<program::Expression> & value)
{
  /// TODO: add type-checking
  d->default_arguments.push_back(value);
}

void Function::addDefaultArgument(const script::Value & val, ParameterPolicy policy)
{
  /// TODO: add type-checking

  if (policy == Value::Take)
  {
    engine()->manage(val);
    d->default_arguments.push_back(program::VariableAccess::New(val));
  }
  else if (policy == Value::Copy || policy == Value::Move) // move not well supported yet
  {
    Value v = engine()->copy(val);
    engine()->manage(v);
    d->default_arguments.push_back(program::VariableAccess::New(val));
  }
}

/*!
 * \fun const std::vector<std::shared_ptr<program::Expression>> & defaultArguments() const
 * \brief Returns the function's default arguments.
 *
 * Note that you cannot concatenate this list to an existing list of arguments to make 
 * a valid call as the default arguments are stored in reverse order; i.e. \c{defaultArguments()[0]} 
 * is the default value for the last parameter, \c{defaultArguments()[1]} is the default value 
 * for the penultimate parameter and so on.
 *
 * Currently this function throws an std::runtime_error if there are no default arguments.
 *
 * \sa hasDefaultArguments()
 */
const std::vector<std::shared_ptr<program::Expression>> & Function::defaultArguments() const
{
  return d->default_arguments.get();
}


Script Function::script() const
{
  auto enclosing_symbol = d->enclosing_symbol.lock();
  if (dynamic_cast<NamespaceImpl*>(enclosing_symbol.get()) != nullptr)
    return Namespace{ std::dynamic_pointer_cast<NamespaceImpl>(enclosing_symbol) }.script();
  else if (dynamic_cast<ClassImpl*>(enclosing_symbol.get()) != nullptr)
    return Class{ std::dynamic_pointer_cast<ClassImpl>(enclosing_symbol) }.script();
  return Script{};
}

bool Function::isConstructor() const
{
  // THe following is incorrect I believe
  // return d->prototype.count() >= 1
  //   & d->prototype.at(0).testFlag(Type::ThisFlag)
  //   & d->prototype.returnType().isConstRef()
  //   & d->prototype.returnType().baseType() == d->prototype.at(0).baseType();
  
  // correct implementation
  return dynamic_cast<ConstructorImpl *>(d.get()) != nullptr;
}

bool Function::isDestructor() const
{
  // This is also incorrect
  /*return d->prototype.count() == 1
    && d->prototype.at(0).testFlag(Type::ThisFlag)
    && d->prototype.returnType() == Type::Void;*/

  // correct implementation
  return dynamic_cast<DestructorImpl *>(d.get()) != nullptr;
}

bool Function::isDefaultConstructor() const
{
  auto ctor = dynamic_cast<const ConstructorImpl *>(d.get());
  return ctor != nullptr && ctor->is_default_ctor();
}

bool Function::isCopyConstructor() const
{
  auto ctor = dynamic_cast<const ConstructorImpl *>(d.get());
  return ctor != nullptr && ctor->is_copy_ctor();
}

bool Function::isMoveConstructor() const
{
  auto ctor = dynamic_cast<const ConstructorImpl *>(d.get());
  return ctor != nullptr && ctor->is_move_ctor();
}

bool Function::isNative() const
{
  return (d->flags & 0x3) == NativeFunction;
}

bool Function::isExplicit() const
{
  return (d->flags >> 2) & Explicit;
}

bool Function::isConst() const
{
  return isNonStaticMemberFunction() && d->prototype.at(0).isConstRef();
}

bool Function::isVirtual() const
{
  return (d->flags >> 2) & Virtual;
}

bool Function::isPureVirtual() const
{
  return (d->flags >> 2) & Pure;
}

bool Function::isDefaulted() const
{
  return (d->flags >> 2) & Default;
}

bool Function::isDeleted() const
{
  return (d->flags >> 2) & Delete;
}

bool Function::isMemberFunction() const
{
  return dynamic_cast<const ClassImpl *>(d->enclosing_symbol.lock().get()) != nullptr;
}

bool Function::isStatic() const
{
  return d->flags & (Static << 2);
}

bool Function::isSpecial() const
{
  return isConstructor() || isDestructor();
}

bool Function::hasImplicitObject() const
{
  return isNonStaticMemberFunction();
}

Class Function::memberOf() const
{
  return Class{ std::dynamic_pointer_cast<ClassImpl>(d->enclosing_symbol.lock()) };
}

AccessSpecifier Function::accessibility() const
{
  return static_cast<AccessSpecifier>(3 & (d->flags >> 9));
}

Namespace Function::enclosingNamespace() const
{
  Class c = memberOf();
  if (c.isNull())
    return Namespace{ std::dynamic_pointer_cast<NamespaceImpl>(d->enclosing_symbol.lock()) };
  return c.enclosingNamespace();
}

bool Function::isOperator() const
{
  return dynamic_cast<OperatorImpl*>(d.get()) != nullptr;
}

Operator Function::toOperator() const
{
  return Operator{ std::dynamic_pointer_cast<OperatorImpl>(d) };
}

bool Function::isLiteralOperator() const
{
  return dynamic_cast<LiteralOperatorImpl*>(d.get()) != nullptr;

}

LiteralOperator Function::toLiteralOperator() const
{
  return LiteralOperator{ std::dynamic_pointer_cast<LiteralOperatorImpl>(d) };
}


bool Function::isCast() const
{
  return dynamic_cast<CastImpl*>(d.get()) != nullptr;
}

Cast Function::toCast() const
{
  return Cast{ std::dynamic_pointer_cast<CastImpl>(d) };
}

bool Function::isTemplateInstance() const
{
  return dynamic_cast<const FunctionTemplateInstance *>(d.get()) != nullptr;
}

FunctionTemplate Function::instanceOf() const
{
  return dynamic_cast<const FunctionTemplateInstance *>(d.get())->mTemplate;
}

const std::vector<TemplateArgument> & Function::arguments() const
{
  return dynamic_cast<const FunctionTemplateInstance *>(d.get())->mArgs;
}

NativeFunctionSignature Function::native_callback() const
{
  return d->implementation.callback;
}

std::shared_ptr<program::Statement> Function::program() const
{
  return d->implementation.program;
}

const std::shared_ptr<UserData> & Function::data() const
{
  return d->data;
}

Engine * Function::engine() const
{
  return d->engine;
}

Function & Function::operator=(const Function & other)
{
  d = other.d;
  return *(this);
}

bool Function::operator==(const Function & other) const
{
  return d == other.d;
}

bool Function::operator!=(const Function & other) const
{
  return d != other.d;
}

} // namespace script
