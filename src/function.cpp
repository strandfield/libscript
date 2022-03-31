// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/function.h"
#include "script/private/function_p.h"

#include "script/cast.h"
#include "script/class.h"
#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/literals.h"
#include "script/locals.h"
#include "script/name.h"
#include "script/namespace.h"
#include "script/operator.h"
#include "script/script.h"

#include "script/interpreter/interpreter.h"

#include "script/program/expression.h"

#include "script/private/cast_p.h"
#include "script/private/class_p.h"
#include "script/private/literals_p.h"
#include "script/private/namespace_p.h"
#include "script/private/operator_p.h"
#include "script/private/script_p.h"

namespace script
{

FunctionImpl::FunctionImpl(Engine *e, FunctionFlags f)
  : SymbolImpl(e)
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

bool FunctionImpl::is_function() const
{
  return true;
}

bool FunctionImpl::is_ctor() const
{
  return get_kind() == SymbolKind::Constructor;
}

bool FunctionImpl::is_dtor() const
{
  return get_kind() == SymbolKind::Destructor;
}

//bool FunctionImpl::is_native() const
//{
//  return true;
//}

std::shared_ptr<program::Statement> FunctionImpl::body() const
{
  return nullptr;
}

void FunctionImpl::set_return_type(const Type& t)
{
  throw std::runtime_error{ "Bad call to FunctionImpl::set_return_type()" };
}

script::Value FunctionImpl::invoke(script::FunctionCall* c)
{
  throw std::runtime_error{ "Bad call to FunctionImpl::invoke()" };
}

bool FunctionImpl::is_template_instance() const
{
  return false;
}

bool FunctionImpl::is_instantiation_completed() const
{
  return true;
}

void FunctionImpl::complete_instantiation()
{
  throw std::runtime_error{ "Bad call to FunctionImpl::complete_instantiation()" };
}

const std::vector<DefaultArgument> & FunctionImpl::default_arguments() const
{
  static const std::vector<DefaultArgument> defaults = {};
  return defaults;
}

void FunctionImpl::set_default_arguments(std::vector<DefaultArgument> defaults)
{
  if (defaults.empty())
    return;

  throw std::runtime_error{ "Function does not support default arguments" };
}

void FunctionImpl::add_default_argument(const DefaultArgument &)
{
  throw std::runtime_error{ "Function does not support default arguments" };
}

void FunctionImpl::force_virtual()
{
  this->flags.set(FunctionSpecifier::Virtual);
}



RegularFunctionImpl::RegularFunctionImpl(std::string name, const Prototype& p, Engine *e, FunctionFlags f)
  : FunctionImpl(e, f)
  , prototype_(p)
  , mName(std::move(name))
{

}

RegularFunctionImpl::RegularFunctionImpl(std::string name, DynamicPrototype p, Engine *e, FunctionFlags f)
  : FunctionImpl(e, f)
  , prototype_(std::move(p))
  , mName(std::move(name))
{

}

const std::string& RegularFunctionImpl::name() const
{
  return mName;
}

SymbolKind RegularFunctionImpl::get_kind() const
{
  return SymbolKind::Function;
}

Name RegularFunctionImpl::get_name() const
{
  return Name(SymbolKind::Function, name());
}

bool RegularFunctionImpl::is_native() const
{
  return false;
}

std::shared_ptr<program::Statement> RegularFunctionImpl::body() const
{
  return program_;
}

void RegularFunctionImpl::set_body(std::shared_ptr<program::Statement> b)
{
  program_ = b;
}

const Prototype & RegularFunctionImpl::prototype() const
{
  return prototype_;
}

void RegularFunctionImpl::set_return_type(const Type& t)
{
  prototype_.setReturnType(t);
}

const std::vector<DefaultArgument> & RegularFunctionImpl::default_arguments() const
{
  return mDefaultArguments;
}

void RegularFunctionImpl::set_default_arguments(std::vector<DefaultArgument> defaults)
{
  mDefaultArguments = std::move(defaults);
}

void RegularFunctionImpl::add_default_argument(const DefaultArgument& da)
{
  mDefaultArguments.push_back(da);
}



ScriptFunctionImpl::ScriptFunctionImpl(Engine *e)
  : FunctionImpl(e)
{

}

SymbolKind ScriptFunctionImpl::get_kind() const
{
  return SymbolKind::Function;
}

const std::string& ScriptFunctionImpl::name() const
{
  static std::string static_name = "__root";
  return static_name;
}

bool ScriptFunctionImpl::is_native() const
{
  return false;
}

std::shared_ptr<program::Statement> ScriptFunctionImpl::body() const
{
  return program_;
}

void ScriptFunctionImpl::set_body(std::shared_ptr<program::Statement> b)
{
  program_ = b;
}

const Prototype& ScriptFunctionImpl::prototype() const
{
  return this->prototype_;
}



ConstructorImpl::ConstructorImpl(const Prototype& p, Engine *e, FunctionFlags f)
  : FunctionImpl(e, f)
  , prototype_(p)
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

const Prototype& ConstructorImpl::prototype() const
{
  return this->prototype_;
}

SymbolKind ConstructorImpl::get_kind() const
{
  return SymbolKind::Constructor;
}

Name ConstructorImpl::get_name() const 
{
  return Name(SymbolKind::Constructor, prototype().at(0));
}

const std::vector<DefaultArgument>& ConstructorImpl::default_arguments() const
{
  return mDefaultArguments;
}

void ConstructorImpl::set_default_arguments(std::vector<DefaultArgument> defaults)
{
  mDefaultArguments = std::move(defaults);
}

void ConstructorImpl::add_default_argument(const DefaultArgument& da)
{
  mDefaultArguments.push_back(da);
}

bool ConstructorImpl::is_native() const
{
  return false;
}

std::shared_ptr<program::Statement> ConstructorImpl::body() const
{
  return program_;
}

void ConstructorImpl::set_body(std::shared_ptr<program::Statement> b)
{
  program_ = b;
}



DestructorImpl::DestructorImpl(const Prototype& p, Engine *e, FunctionFlags f)
  : FunctionImpl(e, f)
  , proto_{p.returnType(), p.at(0)}
{

}

SymbolKind DestructorImpl::get_kind() const
{
  return SymbolKind::Destructor;
}

const Prototype & DestructorImpl::prototype() const
{
  return this->proto_;
}

bool DestructorImpl::is_native() const
{
  return false;
}

std::shared_ptr<program::Statement> DestructorImpl::body() const
{
  return program_;
}

void DestructorImpl::set_body(std::shared_ptr<program::Statement> b)
{
  program_ = b;
}


FunctionTemplateInstance::FunctionTemplateInstance(const FunctionTemplate & ft, const std::vector<TemplateArgument> & targs, const std::string & name, const Prototype &p, Engine *e, FunctionFlags f)
  : RegularFunctionImpl(name, p, e, f)
  , mTemplate(ft)
  , mArgs(targs)
{

}

/// TODO: maybe move this to functionbuilder.cpp
std::shared_ptr<FunctionTemplateInstance> FunctionTemplateInstance::create(const FunctionTemplate & ft, const std::vector<TemplateArgument> & targs, const FunctionBlueprint& blueprint)
{
  auto impl = std::make_shared<FunctionTemplateInstance>(ft, targs, blueprint.name_.string(), blueprint.prototype(), ft.engine(), blueprint.flags());
  impl->program_ = blueprint.body();
  impl->data = blueprint.data();
  impl->enclosing_symbol = ft.enclosingSymbol().impl();
  return impl;
}

bool FunctionTemplateInstance::is_template_instance() const
{
  return true;
}

bool FunctionTemplateInstance::is_instantiation_completed() const
{
  return program_ != nullptr;
}

void FunctionTemplateInstance::complete_instantiation()
{
  // no-op
}

/*!
 * \class Function
 */

Function::Function(const std::shared_ptr<FunctionImpl> & impl)
  : d(impl)
{

}

bool Function::isNull() const
{
  return d == nullptr;
}

/*!
 * \fn const std::string& name() const
 * \brief returns the function name as a string
 * 
 * Not all functions support this method.
 */
const std::string& Function::name() const
{
  return d->name();
}

/*!
 * \fn Name getName() const
 * \brief returns the function name
 */
Name Function::getName() const
{
  return d->get_name();
}

/*!
 * \fn const Prototype& prototype() const
 * \brief returns the function prototype
 */
const Prototype& Function::prototype() const
{
  return d->prototype();
}

/*!
 * \fn const Type& parameter(size_t index) const
 * \brief returns the function parameter at a given index
 */
const Type& Function::parameter(size_t index) const
{
  return prototype().at(index);
}

/*!
 * \fn const Type& returnType() const
 * \brief returns the function return type
 */
const Type& Function::returnType() const
{
  return prototype().returnType();
}

/*!
 * \fun bool hasDefaultArguments() const
 * \brief Returns whether the function has default arguments.
 *
 * This function is deprecated and might be removed in future versions; 
 * use this alternative \c{defaultArguments().size() != 0} instead.
 */
bool Function::hasDefaultArguments() const
{
  return !d->default_arguments().empty();
}

/*!
* \fun size_t defaultArgumentCount() const
* \brief Returns the number of default arguments of the function.
*
* This function is deprecated and might be removed in future versions;
* use this alternative \c{defaultArguments().size()} instead.
*/
size_t Function::defaultArgumentCount() const
{
  return d->default_arguments().size();
}

void Function::addDefaultArgument(const std::shared_ptr<program::Expression> & value)
{
  /// TODO: add type-checking
  d->add_default_argument(value);
}

/*!
 * \fn void addDefaultArgument(const script::Value & val, ParameterPolicy policy) 
 * \brief Adds a default argument to the function.
 * \param the value of the default argument
 * \param policy describing how the value is to be treated
 *
 * This function is deprecated and might be removed in future versions.
 * It is recommended to provide the default arguments at construction-time using the 
 * \t FunctionBuilder class.
 */
void Function::addDefaultArgument(const script::Value & val, ParameterPolicy policy)
{
  /// TODO: add type-checking

  if (policy == Value::Take)
  {
    d->add_default_argument(program::VariableAccess::New(val));
  }
  else if (policy == Value::Copy || policy == Value::Move) // move not well supported yet
  {
    Value v = engine()->copy(val);
    d->add_default_argument(program::VariableAccess::New(val));
  }
}

/*!
 * \fn const std::vector<std::shared_ptr<program::Expression>> & defaultArguments() const
 * \brief Returns the function's default arguments.
 *
 * Note that you cannot concatenate this list to an existing list of arguments to make 
 * a valid call as the default arguments are stored in reverse order; i.e. \c{defaultArguments()[0]} 
 * is the default value for the last parameter, \c{defaultArguments()[1]} is the default value 
 * for the penultimate parameter and so on.
 */
const std::vector<std::shared_ptr<program::Expression>> & Function::defaultArguments() const
{
  return d->default_arguments();
}

/*!
 * \fn Script script() const
 * \brief returns the script in which this function is defined
 */
Script Function::script() const
{
  auto enclosing_symbol = d->enclosing_symbol.lock();
  if (dynamic_cast<NamespaceImpl*>(enclosing_symbol.get()) != nullptr)
    return Namespace{ std::dynamic_pointer_cast<NamespaceImpl>(enclosing_symbol) }.script();
  else if (dynamic_cast<ClassImpl*>(enclosing_symbol.get()) != nullptr)
    return Class{ std::dynamic_pointer_cast<ClassImpl>(enclosing_symbol) }.script();
  return Script{};
}

/*!
 * \fn Attributes attributes() const
 * \brief returns the attributes associated to this function
 */
Attributes Function::attributes() const
{
  return script().getAttributes(*this);
}

/*!
 * \fn bool isConstructor() const
 * \brief returns whether the function is a constructor
 */
bool Function::isConstructor() const
{
  return d && d->is_ctor();
}

/*!
 * \fn bool isDestructor() const
 * \brief returns whether the function is a destructor
 */
bool Function::isDestructor() const
{
  return d && d->is_dtor();
}

/*!
 * \fn bool isDefaultConstructor() const
 * \brief returns whether the function is a default constructor
 */
bool Function::isDefaultConstructor() const
{
  return isConstructor() && prototype().count() == 1;
}

/*!
 * \fn bool isCopyConstructor() const
 * \brief returns whether the function is a copy constructor
 */
bool Function::isCopyConstructor() const
{
  return isConstructor()
    && prototype().count() == 2
    && prototype().at(1) == Type::cref(Symbol(d->enclosing_symbol.lock()).toClass().id());
}

/*!
 * \fn bool isMoveConstructor() const
 * \brief returns whether the function is a move constructor
 */
bool Function::isMoveConstructor() const
{
  return isConstructor()
    && prototype().count() == 2
    && prototype().at(1) == Type::rref(Symbol(d->enclosing_symbol.lock()).toClass().id());
}

bool Function::isNative() const
{
  return d->is_native();
}

/*!
 * \fn bool isExplicit() const
 * \brief returns whether the function is explicit
 */
bool Function::isExplicit() const
{
  return d->flags.test(FunctionSpecifier::Explicit);
}

/*!
 * \fn bool isConst() const
 * \brief returns whether the function is a const member function
 */
bool Function::isConst() const
{
  return isNonStaticMemberFunction() && d->prototype().at(0).isConstRef();
}

/*!
 * \fn bool isVirtual() const
 * \brief returns whether the function is virtual
 */
bool Function::isVirtual() const
{
  return d->flags.test(FunctionSpecifier::Virtual);
}

/*!
 * \fn bool isPureVirtual() const
 * \brief returns whether the function is a pure virtual function
 */
bool Function::isPureVirtual() const
{
  return d->flags.test(FunctionSpecifier::Pure);
}

/*!
 * \fn bool isDefaulted() const
 * \brief returns whether the function is defaulted
 */
bool Function::isDefaulted() const
{
  return d->flags.test(FunctionSpecifier::Default);
}

/*!
 * \fn bool isDeleted() const
 * \brief returns whether the function is deleted
 */
bool Function::isDeleted() const
{
  return d->flags.test(FunctionSpecifier::Delete);
}

/*!
 * \fn bool isMemberFunction() const
 * \brief returns whether the function is defined in a class
 */
bool Function::isMemberFunction() const
{
  return Symbol(d->enclosing_symbol.lock()).isClass();
}

/*!
 * \fn bool isStatic() const
 * \brief returns whether the function is static
 */
bool Function::isStatic() const
{
  return d->flags.test(FunctionSpecifier::Static);
}

/*!
 * \fn bool isSpecial() const
 * \brief returns whether the function is a constructor or a destructor
 */
bool Function::isSpecial() const
{
  return isConstructor() || isDestructor();
}

/*!
 * \fn bool hasImplicitObject() const
 * \brief returns whether the function has an implicit object parameter
 * 
 * In other words, returns whether the function is a non-static member function.
 */
bool Function::hasImplicitObject() const
{
  return isNonStaticMemberFunction();
}

/*!
 * \fn Class memberOf() const
 * \brief returns the class this function is a member of
 */
Class Function::memberOf() const
{
  return Class(isMemberFunction() ? std::static_pointer_cast<ClassImpl>(d->enclosing_symbol.lock()) : nullptr);
}

AccessSpecifier Function::accessibility() const
{
  return d->flags.getAccess();
}

/*!
 * \fn Namespace enclosingNamespace() const
 * \brief returns the namespace in which the function is defined
 * 
 * If the function is a member function, this returns the namespace in 
 * which the class is defined.
 */
Namespace Function::enclosingNamespace() const
{
  Class c = memberOf();
  if (c.isNull())
    return Namespace{ std::dynamic_pointer_cast<NamespaceImpl>(d->enclosing_symbol.lock()) };
  return c.enclosingNamespace();
}

/*!
 * \fn bool isOperator() const
 * \brief returns whether the function is an operator
 */
bool Function::isOperator() const
{
  return d && d->get_kind() == SymbolKind::Operator;
}

/*!
 * \fn Operator toOperator() const
 * \brief returns this function as an operator
 */
Operator Function::toOperator() const
{
  return Operator(*this);
}

/*!
 * \fn bool isLiteralOperator() const
 * \brief returns whether the function is a literal operator
 */
bool Function::isLiteralOperator() const
{
  return d && d->get_kind() == SymbolKind::LiteralOperator;
}

LiteralOperator Function::toLiteralOperator() const
{
  // @TODO: this is no longer correct as inheriting from LiteralOperatorImpl
  // is not required to be a literal operator.
  return LiteralOperator{ std::dynamic_pointer_cast<LiteralOperatorImpl>(d) };
}

/*!
 * \fn bool isCast() const
 * \brief returns whether the function is a conversion function
 */
bool Function::isCast() const
{
  return d && d->get_kind() == SymbolKind::Cast;
}

Cast Function::toCast() const
{
  // @TODO: this is no longer correct as inheriting from CastImpl
  // is not required to be a cast.
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

std::shared_ptr<program::Statement> Function::program() const
{
  return d->body();
}

const std::shared_ptr<UserData> & Function::data() const
{
  return d->data;
}

/*!
 * \fn Engine* engine() const
 * \brief returns the script engine
 */
Engine* Function::engine() const
{
  return d->engine;
}

/*!
 * \fn Value call(Locals& locals) const
 * \brief Calls the function with the given locals
 *
 * This function converts all the locals to the parameter's type 
 * before calling \m invoke().
 */
Value Function::call(Locals& locals) const
{
  const int offset = (hasImplicitObject() ? 1 : 0);

  for (size_t i = offset; i < locals.size(); ++i)
  {
    locals[i] = engine()->convert(locals[i], parameter(i));
  }

  return invoke(locals.data());
}

/*!
 * \fn Value invoke(std::initializer_list<Value>&& args) const
 * \brief Invokes the function with the given args
 *
 * No conversion nor any type-checking is performed. 
 */
Value Function::invoke(std::initializer_list<Value>&& args) const
{
  return d->engine->interpreter()->invoke(*this, nullptr, args.begin(), args.end());
}

/*!
 * \fn Value invoke(const std::vector<Value>& args) const
 * \brief Overloads invoke()
 */
Value Function::invoke(const std::vector<Value>& args) const
{
  if (args.empty())
  {
    return invoke({});
  }

  return d->engine->interpreter()->invoke(*this, nullptr, &(*args.begin()), &(*args.begin()) + args.size());
}

/*!
 * \fn Value invoke(const Value* begin, const Value* end) const
 * \brief Overloads invoke()
 */
Value Function::invoke(const Value* begin, const Value* end) const
{
  return d->engine->interpreter()->invoke(*this, nullptr, begin, end);
}

bool Function::operator==(const Function & other) const
{
  return d == other.d;
}

bool Function::operator!=(const Function & other) const
{
  return d != other.d;
}

/*!
 * \endclass
 */

} // namespace script
