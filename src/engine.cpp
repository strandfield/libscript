// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/engine.h"
#include "script/private/engine_p.h"

#include "script/array.h"
#include "script/cast.h"
#include "script/class.h"
#include "script/classbuilder.h"
#include "script/context.h"
#include "script/conversions.h"
#include "script/enum.h"
#include "script/enumerator.h"
#include "script/function.h"
#include "script/functionbuilder.h"
#include "script/functiontype.h"
#include "script/lambda.h"
#include "script/module.h"
#include "script/namelookup.h"
#include "script/namespace.h"
#include "script/object.h"
#include "script/operator.h"
#include "script/overloadresolution.h"
#include "script/scope.h"
#include "script/script.h"
#include "script/value.h"

#include "script/compiler/compiler.h"
#include "script/compiler/compilererrors.h"

#include "script/private/array_p.h"
#include "script/private/builtinoperators.h"
#include "script/private/class_p.h"
#include "script/private/context_p.h"
#include "script/private/enum_p.h"
#include "script/private/function_p.h"
#include "script/private/lambda_p.h"
#include "script/private/module_p.h"
#include "script/private/namespace_p.h"
#include "script/private/object_p.h"
#include "script/private/operator_p.h"
#include "script/private/scope_p.h"
#include "script/private/script_p.h"
#include "script/private/string_p.h"
#include "script/private/template_p.h"
#include "script/private/value_p.h"


#if defined(LIBSCRIPT_HAS_CONFIG)
#include "config/libscript/engine.cpp"
#endif // defined(LIBSCRIPT_HAS_CONFIG)

namespace script
{


template<typename T>
T fundamental_value_cast(const Value & v)
{
  switch (v.type().baseType().data())
  {
  case Type::Boolean:
    return static_cast<T>(v.toBool());
  case Type::Char:
    return static_cast<T>(v.toChar());
  case Type::Int:
    return static_cast<T>(v.toInt());
  case Type::Float:
    return static_cast<T>(v.toFloat());
  case Type::Double:
    return static_cast<T>(v.toDouble());
  default:
    break;
  }

  if (v.type().isEnumType())
    return (T) v.toEnumerator().value();

  throw std::runtime_error{ "fundamental_value_cast : Implementation error" };
}


Value fundamental_conversion(const Value & src, int destType, Engine *e)
{
  switch (destType)
  {
  case Type::Boolean:
    return e->newInt(fundamental_value_cast<bool>(src));
  case Type::Char:
    return e->newChar(fundamental_value_cast<char>(src));
  case Type::Int:
    return e->newInt(fundamental_value_cast<int>(src));
  case Type::Float:
    return e->newFloat(fundamental_value_cast<float>(src));
  case Type::Double:
    return e->newDouble(fundamental_value_cast<double>(src));
  default:
    break;
  }

  throw std::runtime_error{ "fundamental_conversion : Implementation error" };
}

static Value apply_standard_conversion(const Value & arg, const StandardConversion & conv, Engine *engine)
{
  if (conv.isReferenceConversion())
    return arg;

  if (conv.isCopy())
    return engine->copy(arg);

  if (conv.isDerivedToBaseConversion())
  {
    Class target = engine->getClass(arg.type()).indirectBase(conv.derivedToBaseConversionDepth());
    return engine->invoke(target.copyConstructor(), { arg });
  }

  return fundamental_conversion(arg, conv.destType().baseType().data(), engine);
}

static Value apply_conversion(const Value & arg, const Conversion & conv, Engine *engine)
{
  if (!conv.isUserDefinedConversion())
    return apply_standard_conversion(arg, conv.firstStandardConversion(), engine);

  Value ret = apply_standard_conversion(arg, conv.firstStandardConversion(), engine);
  if(!conv.firstStandardConversion().isReferenceConversion())
    engine->manage(ret);

  if (conv.userDefinedConversion().isCast())
  {
    ret = engine->invoke(conv.userDefinedConversion(), { ret });
  }
  else
  {
    Value obj = engine->allocate(conv.destType());
    engine->invoke(conv.userDefinedConversion(), { obj, ret });
    ret = obj;
  }

  return apply_standard_conversion(ret, conv.secondStandardConversion(), engine);
}

namespace callbacks
{

Value function_variable_assignment(interpreter::FunctionCall *c)
{
  c->arg(0).impl()->set_function(c->arg(1).toFunction());
  return c->arg(0);
}

} // namespace callbacks


EngineImpl::EngineImpl(Engine *e)
  : engine(e)
  , garbage_collector_running(false)
{

}

Value EngineImpl::default_construct(const Type & t, const Function & ctor)
{
  if (!ctor.isNull())
    return engine->invoke(ctor, { engine->allocate(t.withoutRef()) });
  else
  {
    switch (t.baseType().data())
    {
    case Type::Boolean:
      return engine->newBool(false);
    case Type::Char:
      return engine->newChar(0);
    case Type::Int:
      return engine->newInt(0);
    case Type::Float:
      return engine->newFloat(0.f);
    case Type::Double:
      return engine->newDouble(0.);
    }
  }

  throw std::runtime_error{ "Not implemented" };
}

Value EngineImpl::copy(const Value & val, const Function & copyctor)
{
  if (!copyctor.isNull())
    return engine->invoke(copyctor, { val });
  else
    return engine->copy(val);
}

void EngineImpl::destroy(const Value & val, const Function & dtor)
{
  auto *impl = val.impl();

  if (impl->type.isObjectType())
    engine->invoke(dtor, { val });

  impl->clear();

  impl->type = 0;
  impl->engine = nullptr;
}

ClosureType EngineImpl::newLambda()
{
  const int id = static_cast<int>(this->lambdas.size()) | Type::LambdaFlag;
  // const int index = id & 0xFFFF;
  ClosureType l{ std::make_shared<ClosureTypeImpl>(id, this->engine) };
  this->lambdas.push_back(l);
  return l;
}

void EngineImpl::register_class(Class & c, int id)
{
  if (id < 1)
  {
    id = static_cast<int>(this->classes.size()) | Type::ObjectFlag;
  }
  else
  {
    if ((id & Type::ObjectFlag) == 0)
      throw std::runtime_error{ "Invalid requested type id for class" };
  }
  
  const int index = id & 0xFFFF;
  if (static_cast<int>(this->classes.size()) <= index)
  {
    this->classes.resize(index + 1);
  }
  else
  {
    if (!this->classes[index].isNull() && this->classes[index] != this->reservations.class_type)
      throw std::runtime_error{ "Engine::newClass() : Class id already used" };
  }

  c.impl()->id = id;
  c.impl()->engine = this->engine;

  this->classes[index] = c;
}

void EngineImpl::register_enum(Enum & e, int id)
{
  if (id < 1)
  {
    id = static_cast<int>(this->enums.size()) | Type::EnumFlag;
  }
  else
  {
    if ((id & Type::EnumFlag) == 0)
      throw std::runtime_error{ "Invalid requested type id for enum" };
  }

  const int index = id & 0xFFFF;
  if (static_cast<int>(this->enums.size()) <= index)
  {
    this->enums.resize(index + 1);
  }
  else
  {
    if (!this->enums[index].isNull() && this->enums[index] != this->reservations.enum_type)
      throw std::runtime_error{ "Enum id already used" };
  }

  e.impl()->id = id;
  e.impl()->engine = this->engine;

  this->enums[index] = e;
}

void EngineImpl::destroy(Enum e)
{
  e.impl()->name.insert(0, "deleted_");
  unregister_enum(e);
}

void EngineImpl::destroy(Class c)
{
  for (const auto & v : c.staticDataMembers())
  {
    this->engine->destroy(v.second.value);
  }

  auto impl = c.impl();
  impl->staticMembers.clear();

  for (const auto & e : impl->enums)
    destroy(e);

  for (const auto & c : impl->classes)
    destroy(c);
  impl->classes.clear();

  impl->functions.clear();
  impl->operators.clear();
  impl->casts.clear();
  impl->templates.clear(); /// TODO: clear the template instances
  impl->typedefs.clear();

  unregister_class(c);

  impl->name.insert(0, "deleted_");
  impl->enclosing_symbol = std::weak_ptr<SymbolImpl>();
}

void EngineImpl::destroy(Namespace ns)
{
  for (const auto & v : ns.vars())
  {
    this->engine->destroy(v.second);
  }

  auto impl = ns.impl();
  impl->variables.clear();

  for (const auto & e : impl->enums)
    destroy(e);
  impl->enums.clear();

  for (const auto & nns : impl->namespaces)
    destroy(nns);
  impl->namespaces.clear();

  for (const auto & c : impl->classes)
    destroy(c);
  impl->classes.clear();

  impl->functions.clear();
  impl->operators.clear();
  impl->literal_operators.clear();
  impl->templates.clear(); /// TODO: clear the template instances
  impl->typedefs.clear();

  impl->enclosing_symbol = std::weak_ptr<SymbolImpl>();
}

void EngineImpl::destroy(Script s)
{
  auto impl = s.impl();

  while (!impl->globals.empty())
  {
    engine->destroy(impl->globals.back());
    impl->globals.pop_back();
  }

  destroy(Namespace{ impl });

  impl->globalNames.clear();
  impl->global_types.clear();

  const int index = s.id();
  this->scripts[index] = Script{};
  while (!this->scripts.empty() && this->scripts.back().isNull())
    this->scripts.pop_back();
}

template<typename T>
void squeeze(std::vector<T> & list)
{
  while (!list.empty() && list.back().isNull())
    list.pop_back();
}

void EngineImpl::unregister_class(Class &c)
{
  const int index = c.id() & 0xFFFF;
  this->classes[index] = Class();
  squeeze(this->classes);
}

void EngineImpl::unregister_enum(Enum &e)
{
  const int index = e.id() & 0xFFFF;
  this->enums[index] = Enum();
  squeeze(this->enums);
}

void EngineImpl::unregister_closure(ClosureType &c)
{
  const int index = c.id() & 0xFFFF;
  this->lambdas[index] = ClosureType();
  squeeze(this->lambdas);
}


/*!
 * \class Engine
 * \brief Script engine class
 */

Engine::Engine()
{
  d = std::unique_ptr<EngineImpl>(new EngineImpl{ this });
}

/*!
 * \fn ~Engine()
 * \brief Destroys the script engine
 *
 * This function destroys the global namespace and all the modules.
 * All variables registered for garbage collection are also destroyed 
 * in the reverse order they have been added, regarless of the value 
 * of their reference counter.
 */
Engine::~Engine()
{
  d->rootNamespace = Namespace{};

  for (auto m : d->modules)
    m.destroy();
  d->modules.clear();

  while (!d->scripts.empty())
    d->destroy(d->scripts.back());

  {
    d->garbage_collector_running = true;
    size_t s = d->garbageCollector.size();
    while (s-- > 0)
    {
      destroy(d->garbageCollector[s]);
    }
    d->garbageCollector.clear();
  }
}


ClassTemplate register_initialize_list_template(Engine*); // defined in initializerlist.cpp

void Engine::setup()
{
  d->rootNamespace = Namespace{ std::make_shared<NamespaceImpl>("", this) };
  d->context = Context{ std::make_shared<ContextImpl>(this, 0, "default_context") };

  register_builtin_operators(d->rootNamespace);

  d->reservations.class_type = Class{ std::make_shared<ClassImpl>(0, "__reserved_class__", nullptr) };
  d->classes.push_back(d->reservations.class_type);
  d->reservations.enum_type = Enum{ std::make_shared<EnumImpl>(0, "__reserved_enum__", nullptr) };
  d->enums.push_back(d->reservations.enum_type);

  Class string = Symbol{ d->rootNamespace }.newClass(get_string_typename()).setId(Type::String).get();
  register_string_type(string);

  d->templates.array = ArrayImpl::register_array_template(this);
  d->templates.initializer_list = register_initialize_list_template(this);

  d->compiler = std::unique_ptr<compiler::Compiler>(new compiler::Compiler{ this });

  auto ec = std::make_shared<interpreter::ExecutionContext>(this, 1024, 256);
  d->interpreter = std::unique_ptr<interpreter::Interpreter>(new interpreter::Interpreter{ ec, this });
}

/*!
 * \fn Value newBool(bool bval)
 * \brief Constructs a new value of type bool
 */
Value Engine::newBool(bool bval)
{
  Value v{ new ValueImpl{Type::Boolean, this} };
  v.impl()->set_bool(bval);
  return v;
}

/*!
 * \fn Value newChar(char cval)
 * \brief Constructs a new value of type char
 */
Value Engine::newChar(char cval)
{
  Value v{ new ValueImpl{ Type::Char, this } };
  v.impl()->set_char(cval);
  return v;
}

/*!
 * \fn Value newInt(int ival)
 * \brief Constructs a new value of type int
 */
Value Engine::newInt(int ival)
{
  Value v{ new ValueImpl{ Type::Int, this } };
  v.impl()->set_int(ival);
  return v;
}

/*!
 * \fn Value newFloat(float fval)
 * \brief Constructs a new value of type float
 */
Value Engine::newFloat(float fval)
{
  Value v{ new ValueImpl{ Type::Float, this } };
  v.impl()->set_float(fval);
  return v;
}

/*!
 * \fn Value newDouble(double dval)
 * \brief Constructs a new value of type double
 */
Value Engine::newDouble(double dval)
{
  Value v{ new ValueImpl{ Type::Double, this } };
  v.impl()->set_double(dval);
  return v;
}

/*!
 * \fn Value newString(const String & sval)
 * \brief Constructs a new value of type String
 */
Value Engine::newString(const String & sval)
{
  Value v{ new ValueImpl{ Type::String, this } };
  v.impl()->set_string(sval);
  return v;
}

/*!
 * \fn Array newArray(ArrayType array_type)
 * \param type of the array
 * \brief Constructs a new array with the given array-type
 *
 * This function returns an object of type \t Array, any array
 * can be converted to a \t Value using \m{Value::fromArray}.
 */
Array Engine::newArray(ArrayType array_type)
{
  Class array_class = getClass(array_type.type);
  auto data = std::dynamic_pointer_cast<SharedArrayData>(array_class.data());
  auto impl = std::make_shared<ArrayImpl>(data->data, this);
  return Array{ impl };
}

/*!
 * \fn Array newArray(ElementType element_type, ...)
 * \param element type of the array
 * \brief Constructs a new array with the given element-type
 *
 * If no such array-type with the given element-type exists, it 
 * is instantiated.
 * You can call this function with the additional argument 
 * \c{Engine::FailIfNotInstantiated} to make this function throw in
 * such case.
 */
Array Engine::newArray(ElementType element_type)
{
  ClassTemplate array = d->templates.array.asClassTemplate();
  Class array_class = array.getInstance({ TemplateArgument{element_type.type.baseType()} });
  auto data = std::dynamic_pointer_cast<SharedArrayData>(array_class.data());
  auto impl = std::make_shared<ArrayImpl>(data->data, this);
  return Array{ impl };
}

const Engine::fail_if_not_instantiated_t Engine::FailIfNotInstantiated;

Array Engine::newArray(ElementType element_type, fail_if_not_instantiated_t)
{
  ClassTemplate array = d->templates.array.asClassTemplate();
  Class arrayClass;
  if (!array.hasInstance({ TemplateArgument{element_type.type.baseType()} }, &arrayClass))
    throw std::runtime_error{ "No array of that type" };

  auto data = std::dynamic_pointer_cast<SharedArrayData>(arrayClass.data());
  auto impl = std::make_shared<ArrayImpl>(data->data, this);
  return Array{ impl };
}

static Value default_construct_fundamental(int type, Engine *e)
{
  switch (type)
  {
  case Type::Boolean:
    return e->newBool(false);
  case Type::Char:
    return e->newChar(0);
  case Type::Int:
    return e->newInt(0);
  case Type::Float:
    return e->newFloat(0.f);
  case Type::Double:
    return e->newDouble(0.);
  default:
    break;
  }

  /// Implementation error
  assert(false);
  std::abort();
}

/*!
 * \fn Value construct(Type t, const std::vector<Value> & args)
 * \param type of the value to construct
 * \param arguments to be passed the constructor
 * \brief Constructs a new value of the given type with the provided arguments.
 *
 * If \c t is a fundamental type, at most one argument may be provided in \c args.
 * If \c t is an enum type, a value of the same type must be provided.
 * If \c t is an object type, overload resolution is performed to select a 
 * suitable constructor.
 *
 * On failure, this function throws a \t ConstructionError with a code describing 
 * the error (e.g. NoMatchingConstructor, ConstructorIsDeleted, TooManyArgumentInInitialization, ...).
 */
Value Engine::construct(Type t, const std::vector<Value> & args)
{
  if (t.isObjectType())
  {
    Class cla = getClass(t);
    const auto & ctors = cla.constructors();
    Function selected = OverloadResolution::selectConstructor(ctors, args);
    if (selected.isNull())
      throw NoMatchingConstructor{};
    else if (selected.isDeleted())
      throw ConstructorIsDeleted{};

    Value result = allocate(t.withoutRef());
    d->interpreter->call(selected, &result, args.data(), args.data() + args.size());
    return result;
  }
  else if (t.isFundamentalType())
  {
    if (args.size() > 1)
      throw TooManyArgumentInInitialization{};

    if (args.size() == 0)
      return default_construct_fundamental(t.baseType().data(), this);
    else
    {
      Value arg = args.front();
      if (arg.type().isFundamentalType())
        return fundamental_conversion(arg, t.baseType().data(), this);
    }
  }
  else if (t.isEnumType())
  {
    if (args.size() > 1)
      throw TooManyArgumentInInitialization{};
    else if(args.size() == 0)
      throw TooFewArgumentInInitialization{};

    Value arg = args.front();
    if (arg.type().baseType() != t.baseType())
      throw NoMatchingConstructor{}; /// TODO: throw something else ?
    return copy(arg);
  }

  throw NotImplemented{ "Engine::construct() : case not implemented" };
}

/*!
 * \fn void destroy(Value val)
 * \param value to destroy
 * \brief Destroys a value.
 *
 * By calling this function, you also implicitly transfer ownership of the value 
 * back to the engine; this means that you shouldn't use the value any further.
 */
void Engine::destroy(Value val)
{
  auto *impl = val.impl();

  if (impl->type.isObjectType())
  {
    Function dtor = getClass(val.type()).destructor();
    invoke(dtor, { val });
  }

  impl->clear();

  impl->type = 0;
  impl->engine = nullptr;
}

/*!
 * \fn void manage(Value val)
 * \param value which lifetime is to be managed
 * \brief Adds a value to the garbage collector.
 *
 * By calling this function, you ask the engine to take care of 
 * destroying the value when it is no longer used.
 * Actual destruction takes place any time after the object is no longer 
 * reachable.
 * It is safe to call this function multiple times with the same value.
 *
 * \sa Value::isManaged
 */
void Engine::manage(Value val)
{
  if (val.isManaged())
    return;

  d->garbageCollector.push_back(val);
  val.impl()->type = val.impl()->type.withFlag(Type::ManagedFlag);
}

/*!
 * \fn void garbageCollect()
 * \brief Runs garbage collection.
 *
 * The engine destroys all values that are no longer reachable.
 */
void Engine::garbageCollect()
{
  if (d->garbage_collector_running)
    return;

  d->garbage_collector_running = true;

  std::vector<Value> temp;
  // we cannot use for-range loop here because iterators might be invalidated 
  // during the call to a destructor...
  for (size_t i(0); i < d->garbageCollector.size(); ++i)
  {
    const Value & val = d->garbageCollector.at(i);
    if (val.impl()->ref > 1)
    {
      temp.push_back(val);
      continue;
    }

    destroy(val);
  }

  std::swap(d->garbageCollector, temp);

  d->garbage_collector_running = false;
}

/*!
 * \fn Value allocate(const Type & t)
 * \param type of the value
 * \brief Creates an uninitialized value of the given type
 *
 * The returned value is left uninitialized. It is your responsability to 
 * assign it a value or manually call a constructor on it before using it.
 * Unless stated otherwise, all functions in the library expect initialized values.
 * There is no way to detect if a value is initialized or not, you have to remember 
 * the initialization-state of each value manually.
 */
Value Engine::allocate(const Type & t)
{
  Value v{ new ValueImpl{ t, this } };
  return v;
}

/*!
 * \fn void free(Value & v)
 * \param input value
 * \brief Transfer ownership of the value back to the engine.
 *
 * No destructor is called on the value, this function assumes that the value has 
 * already been destroyed (by manually calling a destructor) or never was initialized 
 * (for example after a call to \m allocate).
 */
void Engine::free(Value & v)
{
  auto *impl = v.impl();

  impl->clear();

  impl->type = 0;
  impl->engine = nullptr;
}

/*!
 * \fn bool canCopy(const Type & t)
 * \param input type
 * \brief Returns whether a type is copyable.
 *
 * Note that the \c const and \c{&} qualifiers of \c t are ignored.
 */
bool Engine::canCopy(const Type & t)
{
  if (t.isFundamentalType())
    return true;
  else if (t.isEnumType())
    return true;
  else if (t.isObjectType())
  {
    Class cla = getClass(t);
    Function copyCtor = cla.copyConstructor();
    if (copyCtor.isNull() || copyCtor.isDeleted())
      return false;
    return true;
  }
  else if (t.isFunctionType())
    return true;
  else if (t.isClosureType())
    return true;

  return false;
}

static Value copy_fundamental(const Value & val, Engine *e)
{
  switch (val.type().baseType().data())
  {
  case Type::Boolean:
    return e->newBool(val.toBool());
  case Type::Char:
    return e->newChar(val.toChar());
  case Type::Int:
    return e->newInt(val.toInt());
  case Type::Float:
    return e->newFloat(val.toFloat());
  case Type::Double:
    return e->newDouble(val.toDouble());
  default:
    break;
  }

  assert(false);
  std::abort();
}

static Value copy_enumvalue(const Value & val, Engine *e)
{
  return Value::fromEnumerator(val.toEnumerator());
}

static Lambda copy_lambda(const Lambda & l, Engine *e)
{
  auto ret = std::make_shared<LambdaImpl>(l.closureType());
  ClosureType closure_type = l.closureType();
  for (size_t i(0); i < l.captures().size(); ++i)
  {
    if (closure_type.captures().at(i).type.isReference())
      ret->captures.push_back(l.captures().at(i));
    else
      ret->captures.push_back(e->copy(l.captures().at(i)));
  }
  return Lambda{ ret };
}

/*!
 * \fn Value copy(const Value & val)
 * \param input value
 * \brief Creates a copy of a value.
 * 
 * Throws \t CopyError on failure. 
 * This may happen if the type has a deleted or no copy constructor.
 */
Value Engine::copy(const Value & val)
{
  if (val.type().isFundamentalType())
    return copy_fundamental(val, this);
  else if (val.type().isEnumType())
    return copy_enumvalue(val, this);
  else if (val.type().isObjectType())
  {
    Class cla = getClass(val.type());
    Function copyCtor = cla.copyConstructor();
    if (copyCtor.isNull() || copyCtor.isDeleted())
      throw CopyError{};

    Value object = allocate(cla.id());
    invoke(copyCtor, { object, val });
    return object;
  }
  else if (val.type().isFunctionType())
    return Value::fromFunction(val.toFunction(), val.type().baseType());
  else if (val.type().isClosureType())
    return Value::fromLambda(copy_lambda(val.toLambda(), this));

  /// TODO : implement copy of closures and functions

  throw CopyError{};
}

/*!
 * \fn Conversion conversion(const Type & src, const Type & dest)
 * \param source type
 * \param dest type
 * \brief Computes a conversion sequence.
 *
 */
Conversion Engine::conversion(const Type & src, const Type & dest)
{
  return Conversion::compute(src, dest, this);
}

/*!
 * \fn void applyConversions(std::vector<script::Value> & values, const std::vector<Conversion> & conversions)
 * \param input values
 * \param conversions to be applied
 * \brief Applies conversions to a range of value.
 *
 */
void Engine::applyConversions(std::vector<script::Value> & values, const std::vector<Conversion> & conversions)
{
  for (size_t i(0); i < values.size(); ++i)
    values[i] = apply_conversion(values.at(i), conversions.at(i), this);
}

/*!
 * \fn bool canCast(const Type & srcType, const Type & destType)
 * \param source type
 * \param dest type
 * \brief Checks if a conversion is possible.
 *
 */
bool Engine::canCast(const Type & srcType, const Type & destType)
{
  return conversion(srcType, destType).rank() != ConversionRank::NotConvertible;
}

/*!
 * \fn Value cast(const Value & val, const Type & destType)
 * \param input value
 * \param dest type
 * \brief Converts a value to the given type.
 *
 * Throws \t ConversionError on failure.
 */
Value Engine::cast(const Value & val, const Type & destType)
{
  Conversion conv = conversion(val.type(), destType);
  if (conv.isInvalid())
    throw ConversionError{};

  return apply_conversion(val, conv, this);
}

/*!
 * \fn Namespace rootNamespace() const
 * \brief Returns the global namespace.
 *
 */
Namespace Engine::rootNamespace() const
{
  return d->rootNamespace;
}

/*!
 * \fn FunctionType getFunctionType(Type id) const
 * \param function type
 * \brief Returns type info for the given type id.
 *
 */
FunctionType Engine::getFunctionType(Type id) const
{
  const int index = id.data() & 0xFFFF;
  return d->prototypes[index];
}

/*!
 * \fn FunctionType getFunctionType(const Prototype & proto)
 * \param prototype
 * \brief Returns type info for the function type associated with the prototype.
 *
 * If no such type exists, it is created.
 */
FunctionType Engine::getFunctionType(const Prototype & proto)
{
  for (const auto & ft : d->prototypes)
  {
    if (ft.prototype() == proto)
      return ft;
  }

  return newFunctionType(proto);
}

/*!
 * \fn bool hasType(const Type & t) const
 * \param input type
 * \brief Returns whether a given type exists.
 *
 */
bool Engine::hasType(const Type & t) const
{
  if (t.isFundamentalType())
    return true;

  const size_t index = t.data() & 0xFFFF;
  if (t.isObjectType())
    return d->classes.size() > index && !d->classes.at(index).isNull();
  else if(t.isEnumType())
    return d->enums.size() > index && !d->enums.at(index).isNull();
  else if(t.isClosureType())
    return d->lambdas.size() > index && d->lambdas.at(index).impl() != nullptr;
  else if (t.isFunctionType())
    return d->prototypes.size() > index && !d->prototypes.at(index).assignment().isNull();

  return false;
}

/*!
 * \fn Class getClass(Type id) const
 * \param input type
 * \brief Returns the Class associated with the given type.
 *
 */
Class Engine::getClass(Type id) const
{
  if (!id.isObjectType())
    return Class{};

  int index = id.data() & 0xFFFF;
  return d->classes[index];
}

/*!
 * \fn Enum getEnum(Type id) const
 * \param input type
 * \brief Returns the Enum associated with the given type.
 *
 */
Enum Engine::getEnum(Type id) const
{
  if (!id.isEnumType())
    return Enum{};

  int index = id.data() & 0xFFFF;
  return d->enums[index];
}

/*!
 * \fn ClosureType getLambda(Type id) const
 * \param input type
 * \brief Returns typeinfo associated with the given closure type.
 *
 */
ClosureType Engine::getLambda(Type id) const
{
  if (!id.isClosureType())
    return ClosureType{};

  int index = id.data() & 0xFFFF;
  return d->lambdas[index];
}

template<typename T>
bool reserve_range(std::vector<T> & list, const T & value, size_t begin, size_t end)
{
  if (list.size() < end)
    list.resize(end);

  for (size_t i(begin); i < end; ++i)
  {
    if (!list.at(i).isNull() && list.at(i) != value)
      return false; // Type already used
    list[i] = value;
  }

  return true;
}

/*!
 * \fn void reserveTypeRange(int begin, int end)
 * \param first type
 * \param last type
 * \brief Reserves a type range.
 *
 * Reserved types cannot be attributed unless they are explicitly requested.
 * Returns true on success; false otherwise.
 */
bool Engine::reserveTypeRange(int begin, int end)
{
  const Type begin_type{ begin };
  const Type end_type{ end };

  if (!begin_type.isValid() || !end_type.isValid() || begin_type.category() != end_type.category())
    return false; // Invalid type range

  if (begin_type.isFunctionType() || begin_type.isClosureType())
    return false; //Closure types and function types cannot be reserved yet

  const int index_mask = Type::EnumFlag - 1;

  begin = begin & index_mask;
  end = end & index_mask;

  if (begin_type.isEnumType())
    return reserve_range(d->enums, d->reservations.enum_type, begin, end);
  else if (begin_type.isObjectType())
    return reserve_range(d->classes, d->reservations.class_type, begin, end);

  return false;
}

/*!
 * \fn FunctionType newFunctionType(const Prototype & proto)
 * \param prototype
 * \brief Creates a new function type.
 *
 * This function does not check if such type exists an creates a new one 
 * anyway.
 * Use \m getFunctionType instead.
 */
FunctionType Engine::newFunctionType(const Prototype & proto)
{
  const int id = d->prototypes.size();
  Type type{ id | Type::PrototypeFlag };

  BinaryOperatorPrototype assign_proto{ Type::ref(type), Type::ref(type), Type::cref(type) };
  auto assign_op = std::make_shared<BinaryOperatorImpl>(AssignmentOperator, assign_proto, this, FunctionFlags{});
  assign_op->set_impl(callbacks::function_variable_assignment);

  FunctionType ret{ type, proto, Operator{ assign_op } };
  d->prototypes.push_back(ret);
  return ret;
}

/*!
 * \fn compiler::Compiler* compiler() const
 * \brief Returns the engine's compiler.
 *
 */
compiler::Compiler* Engine::compiler() const
{
  return d->compiler.get();
}

/*!
 * \fn Script newScript(const SourceFile & source)
 * \param source file
 * \brief Creates a new script with the given source.
 *
 */
Script Engine::newScript(const SourceFile & source)
{
  Script ret{ std::make_shared<ScriptImpl>(d->scripts.size(), this, source) };
  d->scripts.push_back(ret);
  return ret;
}

/*!
 * \fn bool compile(Script s)
 * \param input script
 * \brief Compiles a script.
 *
 */
bool Engine::compile(Script s)
{
  return compiler()->compile(s);
}

/*!
 * \fn void destroy(Script s)
 * \param input script
 * \brief Destroys a script.
 *
 */
void Engine::destroy(Script s)
{
  d->destroy(s);
}

/*!
 * \fn Module newModule(const std::string & name)
 * \param module name
 * \brief Creates a new native module.
 *
 */
Module Engine::newModule(const std::string & name)
{
  Module m{ std::make_shared<NativeModule>(this, name) };
  d->modules.push_back(m);
  return m;
}

/*!
 * \fn Module newModule(const std::string & name, ModuleLoadFunction load, ModuleCleanupFunction cleanup)
 * \param module name
 * \param load function
 * \param cleanup function
 * \brief Creates a new native module.
 *
 */
Module Engine::newModule(const std::string & name, ModuleLoadFunction load, ModuleCleanupFunction cleanup)
{
  Module m{ std::make_shared<NativeModule>(this, name, load, cleanup) };
  d->modules.push_back(m);
  return m;
}

/*!
 * \fn Module newModule(const std::string & name, const SourceFile & src)
 * \param module name
 * \param load function
 * \param cleanup function
 * \brief Creates a new script module.
 *
 */
Module Engine::newModule(const std::string & name, const SourceFile & src)
{
  auto mimpl = std::make_shared<ScriptModule>(d->scripts.size(), this, src, name);
  d->scripts.push_back(Script{ mimpl });
  Module m{ mimpl };
  d->modules.push_back(m);
  return m;
}

/*!
 * \fn const std::vector<Module> & modules() const
 * \brief Returns all existing modules.
 *
 */
const std::vector<Module> & Engine::modules() const
{
  return d->modules;
}

/*!
 * \fn Module getModule(const std::string & name)
 * \param module name
 * \brief Returns the module with the given name.
 *
 */
Module Engine::getModule(const std::string & name)
{
  for (const auto & m : d->modules)
  {
    if (m.name() == name)
      return m;
  }

  return Module{};
}

/*!
 * \fn Namespace enclosingNamespace(Type t) const
 * \param input type
 * \brief Return the enclosing namespace of the given type.
 *
 */
Namespace Engine::enclosingNamespace(Type t) const
{
  if (t.isFundamentalType() || t.isClosureType() || t.isFunctionType())
    return this->rootNamespace();
  else if (t.isObjectType())
    return getClass(t).enclosingNamespace();
  else if (t.isEnumType())
    return getEnum(t).enclosingNamespace();

  // Reasonable default
  return this->rootNamespace();
}

/*!
 * \fn Type typeId(const std::string & typeName, const Scope & scope) const
 * \param type name
 * \param scope
 * \brief Searchs for a type by name.
 *
 * Throws \t UnknownTypeError if the type could not be resolved.
 */
Type Engine::typeId(const std::string & typeName, Scope scope) const
{
  static const std::map<std::string, Type> fundamentalTypes = std::map<std::string, Type>{
    std::make_pair(std::string{"void"}, Type{Type::Void}),
    std::make_pair(std::string{"bool"}, Type{ Type::Boolean}),
    std::make_pair(std::string{"char"}, Type{ Type::Char}),
    std::make_pair(std::string{"int"}, Type{ Type::Int}),
    std::make_pair(std::string{"float"}, Type{ Type::Float}),
    std::make_pair(std::string{"double"}, Type{ Type::Double}),
  };

  auto it = fundamentalTypes.find(typeName);
  if (it != fundamentalTypes.end())
    return it->second;

  if (scope.isNull())
    scope = Scope{ d->rootNamespace };

  NameLookup lookup = NameLookup::resolve(typeName, scope);
  Type t = lookup.typeResult();
  if (t.isNull())
    throw UnknownTypeError{};
  return t;
}

/*!
 * \fn std::string typeName(Type t) const
 * \param input type
 * \brief Returns the name of a type.
 *
 */
std::string Engine::typeName(Type t) const
{
  if (t.isObjectType())
    return getClass(t).name();
  else if (t.isEnumType())
    return getEnum(t).name();
  else if (t.isClosureType())
    throw std::runtime_error{ "Not implemented : name of closure type" };
  else if (t.isFunctionType())
    throw std::runtime_error{ "Not implemented : name of function type" };

  static const std::string types[] = {
    "Null",
    "void",
    "bool",
    "char",
    "int",
    "float",
    "double",
  };

  int index = t.data() & 0xFFFF;
  if (index < 7)
    return types[index];

  throw UnknownTypeError{};
}

/*!
 * \fn Context newContext()
 * \brief Creates a new context.
 *
 */
Context Engine::newContext()
{
  Context c{ std::make_shared<ContextImpl>(this, d->allContexts.size() + 1, "") };
  d->allContexts.push_back(c);
  return c;
}

/*!
 * \fn Context currentContext() const
 * \brief Returns the current context.
 *
 */
Context Engine::currentContext() const
{
  return d->context;
}

/*!
 * \fn void setContext(Context con)
 * \param the context
 * \brief Sets the current context.
 *
 */
void Engine::setContext(Context con)
{
  d->context = con;
}

/*!
 * \fn Value eval(const std::string & command)
 * \param input command
 * \brief Evaluates an expression.
 *
 * The \m currentContext is used to evaluate the expression.
 *
 * Throws \t EvaluationError on failure.
 */
Value Engine::eval(const std::string & command)
{
  compiler::Compiler c{ this };
  std::shared_ptr<program::Expression> expr;
  try
  {
    expr = c.compile(command, d->context);
  }
  catch (compiler::CompilerException & ex)
  {
    diagnostic::MessageBuilder msb{ diagnostic::Error, this };
    msb << ex;
    diagnostic::Message mssg = msb.build();

    throw EvaluationError{ mssg.to_string() };
  }

  return d->interpreter->eval(expr);
}

/*!
 * \fn Value call(const Function & f, std::initializer_list<Value> && args)
 * \param function
 * \param arguments
 * \brief Calls a function with the given arguments.
 *
 */
Value Engine::call(const Function & f, std::initializer_list<Value> && args)
{
  return d->interpreter->call(f, nullptr, args.begin(), args.end());
}

/*!
 * \fn Value call(const Function & f, const std::vector<Value> & args)
 * \param function
 * \param arguments
 * \brief Calls a function with the given arguments.
 *
 */
Value Engine::call(const Function & f, const std::vector<Value> & args)
{
  return d->interpreter->call(f, nullptr, &args.front(), (&args.front()) + args.size());
}

/*!
 * \fn Value invoke(const Function & f, std::initializer_list<Value> && args)
 * \param function
 * \param arguments
 * \brief Calls a function with the given arguments.
 *
 */
Value Engine::invoke(const Function & f, std::initializer_list<Value> && args)
{
  return d->interpreter->invoke(f, nullptr, args.begin(), args.end());
}

/*!
 * \fn Value invoke(const Function & f, const std::vector<Value> & args)
 * \param function
 * \param arguments
 * \brief Calls a function with the given arguments.
 *
 */

Value Engine::invoke(const Function & f, const std::vector<Value> & args)
{
  return d->interpreter->invoke(f, nullptr, &(args.front()), (&args.front()) + args.size());
}


/*!
 * \fn ClassTemplate getTemplate(...) const
 * \param which template
 * \brief Returns a built-in class template.
 *
 * Currently supported: \c{Engine::ArrayTemplate}, \c{Engine::InitializerListTemplate}.
 */

const Engine::array_template_t Engine::ArrayTemplate = Engine::array_template_t{};

ClassTemplate Engine::getTemplate(array_template_t) const
{
  return d->templates.array;
}

const Engine::initializer_list_template_t Engine::InitializerListTemplate = Engine::initializer_list_template_t{};

ClassTemplate Engine::getTemplate(initializer_list_template_t) const
{
  return d->templates.initializer_list;
}

/*!
 * \fn bool isInitializerListType(const Type & t) const
 * \param input type
 * \brief Returns whether a type is an initializer list type.
 *
 */
bool Engine::isInitializerListType(const Type & t) const
{
  if (!t.isObjectType())
    return false;

  return getClass(t).isTemplateInstance() && getClass(t).instanceOf() == getTemplate(InitializerListTemplate);
}

/*!
 * \fn const std::vector<Script> & scripts() const
 * \brief Returns all list of all existing scripts.
 *
 */
const std::vector<Script> & Engine::scripts() const
{
  return d->scripts;
}

EngineImpl * Engine::implementation() const
{
  return d.get();
}

} // namespace script
