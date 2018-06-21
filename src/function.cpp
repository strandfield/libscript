// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/function.h"
#include "script/private/function_p.h"

#include "script/cast.h"
#include "script/private/cast_p.h"
#include "script/class.h"
#include "script/literals.h"
#include "script/private/literals_p.h"
#include "script/operator.h"
#include "script/private/operator_p.h"

namespace script
{

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
  return data == nullptr;
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
    data.reset(new std::vector<DefaultArgument>{});

  get().push_back(value);
}

std::vector<DefaultArgument> & DefaultArgumentList::get()
{
  if (isEmpty())
    throw std::runtime_error{ "Function has no default parameters" };
  return *data.get();
}

const std::vector<DefaultArgument> & DefaultArgumentList::get() const
{
  if (isEmpty())
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


StaticMemberFunctionImpl::StaticMemberFunctionImpl(const Class & c, const std::string & name, const Prototype &p, Engine *e, FunctionImpl::flag_type f)
  : RegularFunctionImpl(name, p, e, f)
  , mClass(c)
{

}

StaticMemberFunctionImpl::~StaticMemberFunctionImpl()
{

}


ScriptFunctionImpl::ScriptFunctionImpl(Engine *e)
  : FunctionImpl(Prototype{ Type::Void }, e)
{

}


ConstructorImpl::ConstructorImpl(const Class & name, const Prototype &p, Engine *e, FunctionImpl::flag_type f)
  : FunctionImpl(p, e, f)
  , mClass(name)
{

}

bool ConstructorImpl::is_default_ctor() const
{
  return this->prototype.count() == 0;
}

bool ConstructorImpl::is_copy_ctor() const
{
  if (this->prototype.count() != 1)
    return false;
  return this->prototype.at(0) == Type::cref(this->mClass.id());
}

bool ConstructorImpl::is_move_ctor() const
{
  if (this->prototype.count() != 1)
    return false;
  return this->prototype.at(0) == Type::rref(this->mClass.id());
}

DestructorImpl::DestructorImpl(const Class & name, const Prototype &p, Engine *e, FunctionImpl::flag_type f)
  : FunctionImpl(p, e, f)
  , mClass(name)
{

}

FunctionTemplateInstance::FunctionTemplateInstance(const FunctionTemplate & ft, const std::vector<TemplateArgument> & targs, const std::string & name, const Prototype &p, Engine *e, FunctionImpl::flag_type f)
  : RegularFunctionImpl(name, p, e, f)
  , mTemplate(ft)
  , mArgs(targs)
{

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
  d->default_arguments.push_back(value);
}

const std::vector<std::shared_ptr<program::Expression>> & Function::defaultArguments() const
{
  return d->default_arguments.get();
}


Script Function::script() const
{
  return Script{ d->script.lock() };
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
  if (isStatic())
    return true;

  if (d->prototype.count() == 0)
    return false;
  return d->prototype.at(0).testFlag(Type::ThisFlag);
}

bool Function::isStatic() const
{
  return dynamic_cast<StaticMemberFunctionImpl*>(d.get()) != nullptr;
}

Class Function::memberOf() const
{
  if (isConstructor())
    return static_cast<const ConstructorImpl*>(d.get())->mClass;
  else if (isDestructor())
    return static_cast<const DestructorImpl*>(d.get())->mClass;
  else if (isStatic())
    return static_cast<const StaticMemberFunctionImpl*>(d.get())->mClass;

  if (d->prototype.count() == 0)
    return Class{};
  if (!d->prototype.at(0).testFlag(Type::ThisFlag))
    return Class{};
  Type cla = d->prototype.at(0);
  return d->engine->getClass(cla);
}

AccessSpecifier Function::accessibility() const
{
  return static_cast<AccessSpecifier>(3 & (d->flags >> 9));
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
