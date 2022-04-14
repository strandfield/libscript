// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/class.h"

#include "script/classbuilder.h"
#include "script/datamember.h"
#include "script/engine.h"
#include "script/enumbuilder.h"
#include "script/name.h"
#include "script/object.h"
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

SymbolKind ClassImpl::get_kind() const
{
  return SymbolKind::Class;
}

Name ClassImpl::get_name() const
{
  return Name(SymbolKind::Class, this->name);
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


/*!
 * \class Class
 */

Class::Class(const std::shared_ptr<ClassImpl>& impl)
  : d(impl)
{

}

/*!
 * \fn bool isNull() const
 * \brief returns whether the class is null
 * 
 * Calling any other function than isNull() on a null class is 
 * undefined behavior.
 */
bool Class::isNull() const
{
  return d == nullptr;
}

/*!
 * \fn int id() const
 * \brief returns the class' id
 */
int Class::id() const
{
  return d->id;
}

/*!
 * \fn const std::string& name() const
 * \brief returns the class name
 */
const std::string& Class::name() const
{
  return d->name;
}

/*!
 * \fn Class parent() const
 * \brief returns the base class of this class
 */
Class Class::parent() const
{
  return Class{ d->parent.lock() };
}

/*!
 * \fn bool inherits(const Class& type) const
 * \brief returns whether this class is derived from a given class
 * 
 * This function returns true if this class is derived directly or 
 * indirectly from \a type.
 */
bool Class::inherits(const Class& type) const
{
  Class p = parent();
  return *this == type || (!p.isNull() && p.inherits(type));
}

/*!
 * \fn int inheritanceLevel(const Class& type) const
 * \brief returns the level of inheritance of this class to a given class
 *
 * If this class is not derived from \a type, this function returns -1; 
 * otherwise, this function returns the depth between this class and \a type 
 * in the inheritance tree.
 * 
 * A return value of 0 indicates that this class equals \a type ; 
 * a return value of 1 indicates a direct inheritance; and so on.
 */
int Class::inheritanceLevel(const Class& type) const
{
  if (*this == type)
    return 0;
  Class p = parent();
  if (p.isNull())
    return -1;
  int lvl = p.inheritanceLevel(type);
  return lvl == -1 ? -1 : 1 + lvl;
}

/*!
 * \fn bool isFinal() const
 * \brief returns whether this class is final
 */
bool Class::isFinal() const
{
  return d->isFinal;
}

/*!
 * \fn Class indirectBase(int n) const
 * \brief returns a base of this class
 * 
 * If \a n is negative or null, this class is returned.
 * If \a n is 1, this returns this class' base class.
 * If \a n is 2, this returns the base class this class' base; and so on.
 * 
 * @TODO: rename ?
 */
Class Class::indirectBase(int n) const
{
  if (n <= 0 || d == nullptr)
    return *this;

  return parent().indirectBase(n - 1);
}

/*!
 * \fn bool isClosure() const
 * \brief returns whether this class is a closure type
 */
bool Class::isClosure() const
{
  // @TODO: avoid the dynamic_cast
  return dynamic_cast<ClosureTypeImpl*>(d.get()) != nullptr;
}

/*!
 * \fn ClosureType toClosure() const
 * \brief returns this class as a closure type
 */
ClosureType Class::toClosure() const
{
  return ClosureType{ std::dynamic_pointer_cast<ClosureTypeImpl>(d) };
}

/*!
 * \fn const std::vector<Class::DataMember>& dataMembers() const
 * \brief returns the data members of this class
 * 
 * This does not include the data members of the base class.
 */
const std::vector<Class::DataMember>& Class::dataMembers() const
{
  return d->dataMembers;
}

/*!
 * \fn int cumulatedDataMemberCount() const
 * \brief returns the cumulated count of data members
 * 
 * This returns the size of dataMembers() plus the number 
 * of data members in all base classes.
 */
int Class::cumulatedDataMemberCount() const
{
  Class p = parent();
  return static_cast<int>(d->dataMembers.size()) + (p.isNull() ? 0 : p.cumulatedDataMemberCount());
}

/*!
 * \fn int attributesOffset() const
 * \brief returns the offset of this class' data members
 */
int Class::attributesOffset() const
{
  Class p = parent();
  if (p.isNull())
    return 0;
  return p.cumulatedDataMemberCount();
}

/*!
 * \fn int attributeIndex(const std::string& attrName) const
 * \brief returns the index of a data member given its name
 * 
 * The data members of the base classes are considered by this function.
 */
int Class::attributeIndex(const std::string& attrName) const
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

/*!
 * \fn Script script() const
 * \brief returns the script in which this class is defined
 * 
 * If the class wasn't defined in a script, this function returns a null Class.
 */
Script Class::script() const
{
  return Symbol{ *this }.script();
}

/*!
 * \fn const std::shared_ptr<UserData>& data() const
 * brief returns the user data associated to this class
 */
const std::shared_ptr<UserData>& Class::data() const
{
  return d->data;
}

/*!
 * \fn Value instantiate(const std::vector<Value>& args)
 * \brief creates a object of this class
 */
Value Class::instantiate(const std::vector<Value>& args)
{
  return d->engine->construct(id(), args);
}

/*!
 * \fn const std::vector<Class>& classes() const
 * \brief returns the classes defined in this class
 * 
 * Note that this is not the list of classes that are derived 
 * from this class.
 */
const std::vector<Class>& Class::classes() const
{
  return d->classes;
}

EnumBuilder Class::newEnum(const std::string& name)
{
  return EnumBuilder{ Symbol{*this}, name };
}

/*!
 * \fn const std::vector<Enum>& enums() const
 * \brief returns the enums defined in this class
 */
const std::vector<Enum>& Class::enums() const
{
  return d->enums;
}

/*!
 * \fn const std::vector<Template>& templates() const
 * \brief returns the templates defined in this class
 */
const std::vector<Template>& Class::templates() const
{
  return d->templates;
}

/*!
 * \fn void addTypedef(Typedef t)
 * \brief adds a typedef to the class
 */
void Class::addTypedef(Typedef t)
{
  d->typedefs.push_back(std::move(t));
}

/*!
 * \fn const std::vector<Typedef>& typedefs() const
 * \brief returns the list of typedef in this class
 */
const std::vector<Typedef>& Class::typedefs() const
{
  return d->typedefs;
}

/*!
 * \fn const std::vector<Operator>& operators() const
 * \brief returns the operators defined in this class
 */
const std::vector<Operator>& Class::operators() const
{
  return d->operators;
}

/*!
 * \fn const std::vector<Cast>& casts() const
 * \brief returns the conversion functions defined in this class
 */
const std::vector<Cast>& Class::casts() const
{
  return d->casts;
}

/*!
 * \fn const std::vector<Function>& memberFunctions() const
 * \brief returns the methods defined in this class
 * 
 * Note that this does not include the operators, conversion functions, 
 * constructors and destructor.
 * 
 * @TODO: rename to methods() ?
 */
const std::vector<Function>& Class::memberFunctions() const
{
  return d->functions;
}

/*!
 * \fn void addMethod(const Function& f)
 * \brief adds a method to this class
 * 
 * This functions does not support adding operators, constructors, 
 * conversion functions; use addFunction() instead.
 */
void Class::addMethod(const Function& f)
{
  d->register_function(f);
}

/*!
 * \fn void addFunction(const Function& f)
 * \brief adds a function to this class
 */
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

/*!
 * \fn bool isAbstract()
 * \brief returns whether this class is an abstract class
 * 
 * An abstract class has at least one virtual pure function.
 */
bool Class::isAbstract() const
{
  return d->isAbstract;
}

/*!
 * \fn const std::vector<Function>& vtable() const
 * \brief returns the vtable
 */
const std::vector<Function>& Class::vtable() const
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

/*!
 * \fn const std::vector<Function>& constructors() const
 * \brief returns the class' constructors
 */
const std::vector<Function>& Class::constructors() const
{
  return d->constructors;
}

/*!
 * \fn Function defaultConstructor() const
 * \brief returns the class default constructor
 */
Function Class::defaultConstructor() const
{
  return d->defaultConstructor;
}

/*!
 * \fn bool isDefaultConstructible() const
 * \brief returns whether the class is default constructible
 */
bool Class::isDefaultConstructible() const
{
  return !d->defaultConstructor.isNull() && !d->defaultConstructor.isDeleted();
}

/*!
 * \fn Function copyConstructor() const
 * \brief returns the class copy constructor
 */
Function Class::copyConstructor() const
{
  return d->copyConstructor;
}

/*!
 * \fn bool isCopyConstructible() const
 * \brief returns whether the class is copy constructible
 */
bool Class::isCopyConstructible() const
{
  return !d->copyConstructor.isNull() && !d->copyConstructor.isDeleted();
}

/*!
 * \fn Function moveConstructor() const
 * \brief returns the class move constructor
 */
Function Class::moveConstructor() const
{
  return d->moveConstructor;
}

/*!
 * \fn bool isMoveConstructible() const
 * \brief returns whether the class is move constructible
 */
bool Class::isMoveConstructible() const
{
  return !d->moveConstructor.isNull() && !d->moveConstructor.isDeleted();
}

/*!
 * \fn Function destructor() const
 * \brief returns the class destructor
 */
Function Class::destructor() const
{
  return d->destructor;
}

ClassBuilder Class::newNestedClass(std::string name) const
{
  return ClassBuilder(Symbol(*this), name);
}

/*!
 * \fn void addStaticDataMember(const std::string& name, const Value& value, AccessSpecifier aspec)
 * \brief add a static data member to the class
 */
void Class::addStaticDataMember(const std::string& name, const Value& value, AccessSpecifier aspec)
{
  StaticDataMember sdm{ name, value, aspec };
  d->staticMembers[name] = sdm;
}

/*!
 * \fn const std::map<std::string, Class::StaticDataMember>& staticDataMembers() const
 * \brief returns the class static data members
 */
const std::map<std::string, Class::StaticDataMember>& Class::staticDataMembers() const
{
  return d->staticMembers;
}

/*!
 * \fn void addFriend(const Function& f)
 * \brief add a friend function to the class
 */
void Class::addFriend(const Function& f)
{
  d->friend_functions.push_back(f);
}

/*!
 * \fn void addFriend(const Class& c)
 * \brief add a friend class to the class
 */
void Class::addFriend(const Class& c)
{
  return d->friend_classes.push_back(c);
}

/*!
 * \fn const std::vector<Function>& friends(const Function&) const
 * \brief returns the class friend functions
 */
const std::vector<Function>& Class::friends(const Function&) const
{
  return d->friend_functions;
}

/*!
 * \fn const std::vector<Function>& friends(const Class&) const
 * \brief returns the class friend classes
 */
const std::vector<Class>& Class::friends(const Class&) const
{
  return d->friend_classes;
}

/*!
 * \fn Class memberOf() const
 * \brief returns the class in which this class is defined
 */
Class Class::memberOf() const
{
  auto enclosing_symbol = d->enclosing_symbol.lock();
  return Class{ std::dynamic_pointer_cast<ClassImpl>(enclosing_symbol) };
}

/*!
 * \fn Namespace enclosingNamespace() const
 * \brief returns the namespace in which this class is defined
 */
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

/*!
 * \fn Engine* engine() const
 * \brief returns the script engine
 */
Engine* Class::engine() const
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

/*!
 * \endclass
 */

} // namespace script
