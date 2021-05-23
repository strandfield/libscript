// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/class.h"

#include "script/castbuilder.h"
#include "script/classbuilder.h"
#include "script/constructorbuilder.h"
#include "script/datamember.h"
#include "script/destructorbuilder.h"
#include "script/engine.h"
#include "script/enumbuilder.h"
#include "script/functionbuilder.h"
#include "script/name.h"
#include "script/object.h"
#include "script/operatorbuilder.h"
#include "script/script.h"
#include "script/staticdatamember.h"
#include "script/userdata.h"

#include "script/private/class_p.h"
#include "script/private/engine_p.h"
#include "script/private/enum_p.h"
#include "script/private/function_p.h"
#include "script/private/lambda_p.h"
#include "script/private/namespace_p.h"
#include "script/private/template_p.h"
#include "script/private/value_p.h"

namespace script
{

Name ClassImpl::get_name() const
{
  return Name{ this->name };
}

Value ClassImpl::add_default_constructed_static_data_member(const std::string & name, const Type & t, AccessSpecifier aspec)
{
  Value ret = this->engine->construct(t, {});
  this->staticMembers[name] = Class::StaticDataMember{ name, ret, aspec };
  return ret;
}

ClassTemplateInstance::ClassTemplateInstance(ClassTemplate t, const std::vector<TemplateArgument> & args, int i, const std::string & n, Engine *e)
  : ClassImpl(i, n, e)
  , instance_of(t)
  , template_arguments(args)
{

}

DataMember::DataMember(const Type & t, const std::string & n, AccessSpecifier aspec)
  :name(n)
{
  this->type = Type{ t.data() | (static_cast<int>(aspec) << 26) };
}

AccessSpecifier DataMember::accessibility() const
{
  return static_cast<AccessSpecifier>((this->type.data() >> 26) & 3);
}


StaticDataMember::StaticDataMember(const std::string &n, const Value & val, AccessSpecifier aspec)
  : name(n)
  , value(val)
{
  val.impl()->type = Type{ val.type().data() | (static_cast<int>(aspec) << 26) };
}

AccessSpecifier StaticDataMember::accessibility() const
{
  return static_cast<AccessSpecifier>((this->value.type().data() >> 26) & 3);
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

Class Class::indirectBase(int n) const
{
  if (n <= 0 || d == nullptr)
    return *this;

  return parent().indirectBase(n - 1);
}

bool Class::isClosure() const
{
  return dynamic_cast<ClosureTypeImpl*>(d.get()) != nullptr;
}

ClosureType Class::toClosure() const
{
  return ClosureType{ std::dynamic_pointer_cast<ClosureTypeImpl>(d) };
}

const std::vector<Class::DataMember> & Class::dataMembers() const
{
  return d->dataMembers;
}

int Class::cumulatedDataMemberCount() const
{
  Class p = parent();
  return static_cast<int>(d->dataMembers.size()) + (p.isNull() ? 0 : p.cumulatedDataMemberCount());
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
      return offset + static_cast<int>(i);
  }

  Class p = parent();
  if (p.isNull())
    return -1;

  return p.attributeIndex(attrName);
}

Script Class::script() const
{
  return Symbol{ *this }.script();
}

const std::shared_ptr<UserData> & Class::data() const
{
  return d->data;
}

Value Class::instantiate(const std::vector<Value> & args)
{
  return d->engine->construct(id(), args);
}

const std::vector<Class> & Class::classes() const
{
  return d->classes;
}

EnumBuilder Class::newEnum(const std::string & name)
{
  return EnumBuilder{ Symbol{*this}, name };
}

const std::vector<Enum> & Class::enums() const
{
  return d->enums;
}

const std::vector<Template> & Class::templates() const
{
  return d->templates;
}

const std::vector<Typedef> & Class::typedefs() const
{
  return d->typedefs;
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

void Class::addMethod(const Function& f)
{
  d->register_function(f);
}

void Class::addFunction(const Function& f)
{
  if (f.isOperator())
    d->operators.push_back(f.toOperator());
  else if (f.isCast())
    d->casts.push_back(f.toCast());
  else if (f.isConstructor())
    d->registerConstructor(f);
  else if (f.isDestructor())
    d->destructor = f;
  else
    d->register_function(f);
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
  this->parent = p.impl();
  this->isAbstract = p.isAbstract();
  this->virtualMembers = p.vtable();
}

bool ClassImpl::check_overrides(const Function & derived, const Function & base)
{
  if (base.prototype().count() != base.prototype().count())
    return false;

  if (derived.returnType() != base.returnType())
    return false;

  for (int i(1); i < derived.prototype().count(); ++i)
  {
    if (base.prototype().at(i) != base.prototype().at(i))
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
    Class b{ this->parent.lock() };
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
        f.impl()->force_virtual();
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

Function Class::destructor() const
{
  return d->destructor;
}

ConstructorBuilder Class::newConstructor(NativeFunctionSignature func) const
{
  return ConstructorBuilder{ Symbol{ *this } }.setCallback(func);
}

DestructorBuilder Class::newDestructor(NativeFunctionSignature func) const
{
  return DestructorBuilder{ Symbol{*this} }.setCallback(func);
}

OperatorBuilder Class::newOperator(OperatorName op, NativeFunctionSignature func) const
{
  return OperatorBuilder{ Symbol{*this}, op }.setCallback(func);
}

FunctionCallOperatorBuilder Class::newFunctionCallOperator(NativeFunctionSignature func) const
{
  return FunctionCallOperatorBuilder{ Symbol{ *this } }.setCallback(func);
}


CastBuilder Class::newConversion(const Type & dest, NativeFunctionSignature func) const
{
  return CastBuilder{ Symbol{*this}, dest }.setCallback(func);
}

ClassBuilder Class::newNestedClass(const std::string & name) const
{
  return Symbol{ *this }.newClass(name);
}

void Class::addStaticDataMember(const std::string & name, const Value & value, AccessSpecifier aspec)
{
  StaticDataMember sdm{ name, value, aspec };
  d->staticMembers[name] = sdm;
}

const std::map<std::string, Class::StaticDataMember> & Class::staticDataMembers() const
{
  return d->staticMembers;
}

void Class::addFriend(const Function & f)
{
  d->friend_functions.push_back(f);
}

void Class::addFriend(const Class & c)
{
  return d->friend_classes.push_back(c);
}

const std::vector<Function> & Class::friends(const Function &) const
{
  return d->friend_functions;
}

const std::vector<Class> & Class::friends(const Class &) const
{
  return d->friend_classes;
}

Class Class::memberOf() const
{
  auto enclosing_symbol = d->enclosing_symbol.lock();
  return Class{ std::dynamic_pointer_cast<ClassImpl>(enclosing_symbol) };
}

Namespace Class::enclosingNamespace() const
{
  Class c = memberOf();
  if (c.isNull())
    return Namespace{ std::dynamic_pointer_cast<NamespaceImpl>(d->enclosing_symbol.lock()) };
  return c.enclosingNamespace();
}

bool Class::isTemplateInstance() const
{
  return dynamic_cast<const ClassTemplateInstance *>(d.get()) != nullptr;
}

ClassTemplate Class::instanceOf() const
{
  return dynamic_cast<const ClassTemplateInstance *>(d.get())->instance_of;
}

const std::vector<TemplateArgument> & Class::arguments() const
{
  return dynamic_cast<const ClassTemplateInstance *>(d.get())->template_arguments;
}

Engine * Class::engine() const
{
  return d->engine;
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

} // namespace script
