// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/class.h"

#include "class_p.h"
#include "script/functionbuilder.h"
#include "function_p.h"
#include "script/engine.h"
#include "engine_p.h"
#include "script/object.h"
#include "script/userdata.h"

namespace script
{

Value ClassImpl::addStaticDataMember(const std::string & name, const Type & t)
{
  Value ret = this->engine->implementation()->buildValue(t);
  this->staticMembers[name] = Class::StaticDataMember{ name, ret };
  return ret;
}


Class::Class(const std::shared_ptr<ClassImpl> & impl)
  : d(impl)
{

}

bool Class::isNull() const
{
  return d == nullptr;
}

int Class::id() const
{
  return d->id;
}

const std::string & Class::name() const
{
  return d->name;
}

Class Class::parent() const
{
  return Class{ d->parent.lock() };
}

bool Class::inherits(const Class & type) const
{
  Class p = parent();
  return *this == type || (!p.isNull() && p.inherits(type));
}

int Class::inheritanceLevel(const Class & type) const
{
  if (*this == type)
    return 0;
  Class p = parent();
  if (p.isNull())
    return -1;
  int lvl = p.inheritanceLevel(type);
  return lvl == -1 ? -1 : 1 + lvl;
}

bool Class::isFinal() const
{
  return d->isFinal;
}

const std::vector<Class::DataMember> & Class::dataMembers() const
{
  return d->dataMembers;
}

int Class::cumulatedDataMemberCount() const
{
  Class p = parent();
  return d->dataMembers.size() + (p.isNull() ? 0 : p.cumulatedDataMemberCount());
}

int Class::attributesOffset() const
{
  Class p = parent();
  if (p.isNull())
    return 0;
  return p.cumulatedDataMemberCount();
}

int Class::attributeIndex(const std::string & attrName) const
{
  int offset = attributesOffset();
  for (size_t i(0); i < d->dataMembers.size(); ++i)
  {
    if (d->dataMembers[i].name == attrName)
      return offset + i;
  }

  Class p = parent();
  if (p.isNull())
    return -1;

  return p.attributeIndex(attrName);
}

Script Class::script() const
{
  return Script{ d->script.lock() };
}

const std::shared_ptr<UserData> & Class::data() const
{
  return d->data;
}

Value Class::instantiate(const std::vector<Value> & args)
{
  return d->engine->construct(id(), args);
}

Class Class::newClass(const ClassBuilder & opts)
{
  Engine *e = d->engine;
  Class c = e->newClass(opts);
  if (!c.isNull())
    d->classes.push_back(c);
  return c;
}

const std::vector<Class> & Class::classes() const
{
  return d->classes;
}

Enum Class::newEnum(const std::string & src)
{
  Engine *e = d->engine;
  Enum enm = e->newEnum(src);
  if (!enm.isNull())
    d->enums.push_back(enm);
  return enm;
}

const std::vector<Enum> & Class::enums() const
{
  return d->enums;
}

const std::vector<Template> & Class::templates() const
{
  return d->templates;
}

const std::vector<Operator> & Class::operators() const
{
  return d->operators;
}

const std::vector<Cast> & Class::casts() const
{
  return d->casts;
}

const std::vector<Function> & Class::memberFunctions() const
{
  return d->functions;
}

bool Class::isAbstract() const
{
  return d->isAbstract;
}

const std::vector<Function> & Class::vtable() const
{
  return d->virtualMembers;
}

void ClassImpl::registerConstructor(const Function & f)
{
  if (f.isDefaultConstructor())
    this->defaultConstructor = f;
  else if (f.isCopyConstructor())
    this->copyConstructor = f;
  else if (f.isMoveConstructor())
    this->moveConstructor = f;

  this->constructors.push_back(f);
}

void ClassImpl::set_parent(const Class & p)
{
  if (p.isNull())
    return;
  this->parent = p.weakref();
  this->isAbstract = p.isAbstract();
  this->virtualMembers = p.vtable();
}

bool ClassImpl::check_overrides(const Function & derived, const Function & base)
{
  if (base.prototype().argc() != base.prototype().argc())
    return false;

  if (derived.returnType() != base.returnType())
    return false;

  for (int i(1); i < derived.prototype().argc(); ++i)
  {
    if (base.prototype().argv(i) != base.prototype().argv(i))
      return false;
  }

  return derived.name() == base.name();
}

void ClassImpl::check_still_abstract()
{
  this->isAbstract = false;
  for (const auto & f : this->virtualMembers)
  {
    if (f.isPureVirtual())
    {
      this->isAbstract = true;
      return;
    }
  }
}

void ClassImpl::update_vtable(Function f)
{
  if (f.isConstructor() || f.isDestructor() || f.isOperator() || f.isCast())
    return;

  if (this->parent.lock() == nullptr)
  {
    if (f.isVirtual())
      this->virtualMembers.push_back(f);
    return;
  }
  else
  {
    Class b = this->parent.lock();
    const auto & vt = b.vtable();

    // first, we retrieve virtual members from base class.
    // We do it this way because the script compiler processes 
    // class decl before member decl, thus the derived classes 
    // are created before the base has its virtual members.
    // This is a bit ugly and dangerous but should work.
    while (this->virtualMembers.size() < vt.size())
      this->virtualMembers.push_back(vt.at(this->virtualMembers.size()));

    // then we update the table
    for (size_t i(0); i < vt.size(); ++i)
    {
      if (check_overrides(f, vt.at(i)))
      {
        f.implementation()->force_virtual();
        this->virtualMembers[i] = f;
        if (vt.at(i).isPureVirtual())
          check_still_abstract();
        return;
      }
    }

    if (f.isVirtual())
      this->virtualMembers.push_back(f);
  }
}

void ClassImpl::register_function(const Function & f)
{
  this->functions.push_back(f);
  update_vtable(f);
  if (f.isPureVirtual())
    this->isAbstract = true;
}

Function Class::newConstructor(const Prototype & proto, NativeFunctionSignature func, uint8 flags)
{
  auto builder = FunctionBuilder::Constructor(*this, proto, func);
  return newConstructor(builder);
}

Function Class::newConstructor(const FunctionBuilder & builder)
{
  Function ctor = engine()->newFunction(builder);
  d->registerConstructor(ctor);
  return ctor;
}

const std::vector<Function> & Class::constructors() const
{
  return d->constructors;
}

Function Class::defaultConstructor() const
{
  return d->defaultConstructor;
}

bool Class::isDefaultConstructible() const
{
  return !d->defaultConstructor.isNull() && !d->defaultConstructor.isDeleted();
}

Function Class::copyConstructor() const
{
  return d->copyConstructor;
}

bool Class::isCopyConstructible() const
{
  return !d->copyConstructor.isNull() && !d->copyConstructor.isDeleted();
}

Function Class::moveConstructor() const
{
  return d->moveConstructor;
}

bool Class::isMoveConstructible() const
{
  return !d->moveConstructor.isNull() && !d->moveConstructor.isDeleted();
}

Function Class::newDestructor(NativeFunctionSignature func)
{
  auto builder = FunctionBuilder::Destructor(*this, func);
  d->destructor = engine()->newFunction(builder);
  return d->destructor;
}

Function Class::destructor() const
{
  return d->destructor;
}

Function Class::newMethod(const std::string & name, const Prototype & proto, NativeFunctionSignature func, uint8 flags)
{
  FunctionBuilder builder = FunctionBuilder::Function(name, proto, func);
  builder.flags = flags;

  /// TODO : do something about this
  /*
  if (builder.proto.argc() == 0 && ((builder.flags & Function::Static) == 0))
  {
    throw std::runtime_error{ "Invalid member function" };
  }
  */

  auto member = engine()->newFunction(builder);
  d->register_function(member);
  return member;
}

Function Class::newMethod(const FunctionBuilder & builder)
{
  auto member = engine()->newFunction(builder);
  d->register_function(member);
  return member;
}

Operator Class::newOperator(const FunctionBuilder & builder)
{
  assert(builder.kind == Function::OperatorFunction);
  auto member = engine()->newFunction(builder).toOperator();
  d->operators.push_back(member);
  return member;
}

Cast Class::newCast(const FunctionBuilder & builder)
{
  Cast cast = engine()->newFunction(builder).toCast();
  d->casts.push_back(cast);
  return cast;
}

void Class::addStaticDataMember(const std::string & name, const Value & value)
{
  StaticDataMember sdm{ name, value };
  d->staticMembers[name] = sdm;
}

const std::map<std::string, Class::StaticDataMember> & Class::staticDataMembers() const
{
  return d->staticMembers;
}

Engine * Class::engine() const
{
  return d->engine;
}

ClassImpl * Class::implementation() const
{
  return d.get();
}

std::weak_ptr<ClassImpl> Class::weakref() const
{
  return std::weak_ptr<ClassImpl>{d};
}

bool Class::operator==(const Class & other) const
{
  return d == other.d;
}

bool Class::operator!=(const Class & other) const
{
  return d != other.d;
}

bool Class::operator<(const Class & other) const
{
  return d < other.d;
}

ClassBuilder::ClassBuilder(const std::string & n, const Class & p)
  : name(n)
  , parent(p)
  , isFinal(false)
{

}

ClassBuilder ClassBuilder::New(const std::string & n)
{
  ClassBuilder ret{ n };
  return ret;
}

ClassBuilder & ClassBuilder::setParent(const Class & p)
{
  this->parent = p;
  return *this;
}

ClassBuilder & ClassBuilder::setFinal(bool f)
{
  this->isFinal = f;
  return *this;
}

ClassBuilder & ClassBuilder::addMember(const Class::DataMember & dm)
{
  this->dataMembers.push_back(dm);
  return *this;
}

ClassBuilder & ClassBuilder::setData(const std::shared_ptr<UserData> & data)
{
  this->userdata = data;
  return *this;
}

} // namespace script
