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

bool ConstructorImpl::is_default_ctor() const
{
  return this->prototype_.count() == 1;
}

bool ConstructorImpl::is_copy_ctor() const
{
  if (this->prototype_.count() != 2)
    return false;
  return this->prototype_.at(1) == Type::cref(getClass().id());
}

bool ConstructorImpl::is_move_ctor() const
{
  if (this->prototype_.count() != 2)
    return false;
  return this->prototype_.at(1) == Type::rref(getClass().id());
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

const std::string & Function::name() const
{
  return d->name();
}

Name Function::getName() const
{
  return d->get_name();
}

const Prototype& Function::prototype() const
{
  return d->prototype();
}

const Type& Function::parameter(size_t index) const
{
  return prototype().at(index);
}

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

bool Function::isConstructor() const
{
  // THe following is incorrect I believe
  // return d->prototype_.count() >= 1
  //   & d->prototype_.at(0).testFlag(Type::ThisFlag)
  //   & d->prototype_.returnType().isConstRef()
  //   & d->prototype_.returnType().baseType() == d->prototype_.at(0).baseType();
  
  // correct implementation
  // @TODO: not good... add virtual functions in FunctionImpl
  return dynamic_cast<ConstructorImpl *>(d.get()) != nullptr;
}

bool Function::isDestructor() const
{
  // This is also incorrect
  /*return d->prototype_.count() == 1
    && d->prototype_.at(0).testFlag(Type::ThisFlag)
    && d->prototype_.returnType() == Type::Void;*/

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
  return d->is_native();
}

bool Function::isExplicit() const
{
  return d->flags.test(FunctionSpecifier::Explicit);
}

bool Function::isConst() const
{
  return isNonStaticMemberFunction() && d->prototype().at(0).isConstRef();
}

bool Function::isVirtual() const
{
  return d->flags.test(FunctionSpecifier::Virtual);
}

bool Function::isPureVirtual() const
{
  return d->flags.test(FunctionSpecifier::Pure);
}

bool Function::isDefaulted() const
{
  return d->flags.test(FunctionSpecifier::Default);
}

bool Function::isDeleted() const
{
  return d->flags.test(FunctionSpecifier::Delete);
}

bool Function::isMemberFunction() const
{
  return dynamic_cast<const ClassImpl *>(d->enclosing_symbol.lock().get()) != nullptr;
}

bool Function::isStatic() const
{
  return d->flags.test(FunctionSpecifier::Static);
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
  return d->flags.getAccess();
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

std::shared_ptr<program::Statement> Function::program() const
{
  return d->body();
}

const std::shared_ptr<UserData> & Function::data() const
{
  return d->data;
}

Engine * Function::engine() const
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
