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

FunctionImpl::FunctionImpl(const Prototype &p, Engine *e, FunctionImpl::flag_type f)
  : prototype(p)
  , engine(e)
  , flags(f)
{

}

FunctionImpl::~FunctionImpl()
{

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
  return this->prototype.argc() == 0;
}

bool ConstructorImpl::is_copy_ctor() const
{
  if (this->prototype.argc() != 1)
    return false;
  return this->prototype.argv(0) == Type::cref(this->mClass.id());
}

bool ConstructorImpl::is_move_ctor() const
{
  if (this->prototype.argc() != 1)
    return false;
  return this->prototype.argv(0) == Type::rref(this->mClass.id());
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

std::string Function::name() const
{
  return d->name();
}

const Prototype & Function::prototype() const
{
  return d->prototype;
}

std::vector<Prototype> Function::prototypes() const
{
  std::vector<Prototype> ret;
  Prototype proto = d->prototype;
  ret.push_back(proto);
  while (proto.hasDefaultArgument())
  {
    proto.popArgument();
    ret.push_back(proto);
  }
  return ret;
}

bool Function::accepts(int argc) const
{
  const int parameter_count = prototype().argumentCount();
  const int default_count = prototype().defaultArgCount();

  return parameter_count - default_count <= argc && argc <= parameter_count;
}

const Type & Function::parameter(int index) const
{
  return prototype().argv(index);
}

const Type & Function::returnType() const
{
  return prototype().returnType();
}


Script Function::script() const
{
  return Script{ d->script.lock() };
}

bool Function::isConstructor() const
{
  // THe following is incorrect I believe
  // return d->prototype.argc() >= 1
  //   & d->prototype.argv(0).testFlag(Type::ThisFlag)
  //   & d->prototype.returnType().isConstRef()
  //   & d->prototype.returnType().baseType() == d->prototype.argv(0).baseType();
  
  // correct implementation
  return dynamic_cast<ConstructorImpl *>(d.get()) != nullptr;
}

bool Function::isDestructor() const
{
  // This is also incorrect
  /*return d->prototype.argc() == 1
    && d->prototype.argv(0).testFlag(Type::ThisFlag)
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
  return isMemberFunction() && d->prototype.argv(0).isConstRef();
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
  if (d->prototype.argc() == 0)
    return false;
  return d->prototype.argv(0).testFlag(Type::ThisFlag);
}

Class Function::memberOf() const
{
  if (isConstructor())
    return static_cast<const ConstructorImpl*>(d.get())->mClass;
  else if(isDestructor())
    return static_cast<const DestructorImpl*>(d.get())->mClass;

  if (d->prototype.argc() == 0)
    return Class{};
  if (!d->prototype.argv(0).testFlag(Type::ThisFlag))
    return Class{};
  Type cla = d->prototype.argv(0);
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

FunctionImpl * Function::implementation() const
{
  return d.get();
}

std::weak_ptr<FunctionImpl> Function::weakref() const
{
  return std::weak_ptr<FunctionImpl>(d);
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
