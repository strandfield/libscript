// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/class.h"

#include "script/private/class_p.h"
#include "script/private/lambda_p.h"
#include "script/functionbuilder.h"
#include "script/private/function_p.h"
#include "script/engine.h"
#include "script/private/engine_p.h"
#include "script/private/enum_p.h"
#include "script/object.h"
#include "script/private/template_p.h"
#include "script/userdata.h"
#include "script/private/value_p.h"

namespace script
{

Value ClassImpl::add_uninitialized_static_data_member(const std::string & name, const Type & t, AccessSpecifier aspec)
{
  Value ret = this->engine->uninitialized(t);
  this->staticMembers[name] = Class::StaticDataMember{ name, ret, aspec };
  return ret;
}

ClassTemplateInstance::ClassTemplateInstance(ClassTemplate t, const std::vector<TemplateArgument> & args, int i, const std::string & n, Engine *e)
  : ClassImpl(i, n, e)
  , instance_of(t)
  , template_arguments(args)
{

}


std::shared_ptr<ClassTemplateInstance> ClassTemplateInstance::make(const ClassBuilder & builder, const ClassTemplate & ct, const std::vector<TemplateArgument> & args)
{
  auto ret = std::make_shared<ClassTemplateInstance>(ct, args, -1, builder.name, ct.engine());
  ret->set_parent(builder.parent);
  ret->dataMembers = builder.dataMembers;
  ret->isFinal = builder.isFinal;
  ret->data = builder.userdata;
  ret->instance_of = ct;
  ret->template_arguments = args;

  Class class_result{ ret };
  ct.engine()->implementation()->register_class(class_result);

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

bool Class::isClosure() const
{
  return dynamic_cast<ClosureTypeImpl*>(d.get()) != nullptr;
}

ClosureType Class::toClosure() const
{
  return ClosureType{ std::dynamic_pointer_cast<ClosureTypeImpl>(d) };
}

Class::DataMember::DataMember(const Type & t, const std::string & n, AccessSpecifier aspec)
  :name(n)
{
  this->type = Type{ t.data() | (static_cast<int>(aspec) << 26) };
}

AccessSpecifier Class::DataMember::accessibility() const
{
  return static_cast<AccessSpecifier>((this->type.data() >> 26) & 3);
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
  auto enclosing_symbol = d->enclosing_symbol.lock();
  return SymbolImpl::getScript(enclosing_symbol);
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
  {
    d->classes.push_back(c);
    c.impl()->enclosing_symbol = d;
  }
  return c;
}

const std::vector<Class> & Class::classes() const
{
  return d->classes;
}

Enum Class::newEnum(const std::string & name, int id)
{
  Engine *e = d->engine;
  Enum enm = e->newEnum(name, id);
  if (!enm.isNull())
  {
    d->enums.push_back(enm);
    enm.impl()->enclosing_symbol = d;
  }
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
  return newMethod(builder);
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

FunctionBuilder Class::Constructor(NativeFunctionSignature func) const
{
  FunctionBuilder builder{ *this, Function::Constructor };
  builder.callback = func;
  return builder;
}

FunctionBuilder Class::Method(const std::string & name, NativeFunctionSignature func) const
{
  FunctionBuilder builder{ *this, Function::StandardFunction };
  builder.name = name;
  builder.callback = func;
  return builder;
}

FunctionBuilder Class::Operation(OperatorName op, NativeFunctionSignature func) const
{
  FunctionBuilder builder{ *this, op };
  builder.callback = func;
  return builder;
}

FunctionBuilder Class::Conversion(const Type & dest, NativeFunctionSignature func) const
{
  FunctionBuilder builder{ *this, Function::CastFunction };
  builder.callback = func;
  builder.returns(dest);
  return builder;
}


Class::StaticDataMember::StaticDataMember(const std::string &n, const Value & val, AccessSpecifier aspec)
  : name(n)
  , value(val)
{
  val.impl()->type = Type{ val.type().data() | (static_cast<int>(aspec) << 26) };
}

AccessSpecifier Class::StaticDataMember::accessibility() const
{
  return static_cast<AccessSpecifier>((this->value.type().data() >> 26) & 3);
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

ClassBuilder::ClassBuilder(const std::string & n, const Class & p)
  : name(n)
  , parent(p)
  , isFinal(false)
  , id(0)
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

ClassBuilder & ClassBuilder::setId(int n)
{
  this->id = n;
  return *this;
}

} // namespace script
