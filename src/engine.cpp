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


#if defined(LIBSCRIPT_CONFIG_ENGINE_INJECTED_SOURCE)
#include LIBSCRIPT_CONFIG_ENGINE_INJECTED_SOURCE
#endif // defined(LIBSCRIPT_CONFIG_ENGINE_INJECTED_SOURCE)

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

static Value apply_standard_conversion(const Value & arg, const Type &dest, const StandardConversion & conv, Engine *engine)
{
  if (!conv.isCopyInitialization())
    return arg;

  assert(conv.isCopyInitialization());

  if (conv.isDerivedToBaseConversion() || dest.isObjectType())
    return engine->copy(arg);

  return fundamental_conversion(arg, dest.baseType().data(), engine);
}

static Value apply_conversion(const Value & arg, const Type &dest, const ConversionSequence & conv, Engine *engine)
{
  if (conv.isListInitialization())
    throw std::runtime_error{ "Not implemented" };
  else if (!conv.isUserDefinedConversion())
    return apply_standard_conversion(arg, dest, conv.conv1, engine);

  Value ret;

  if (conv.function.isCast())
  {
    auto cast = conv.function.toCast();
    ret = apply_standard_conversion(arg, cast.sourceType(), conv.conv1, engine);
    ret = engine->invoke(cast, { ret });
  }
  else
  {
    assert(conv.function.isConstructor());

    auto ctor = conv.function;
    ret = apply_standard_conversion(arg, ctor.prototype().at(0), conv.conv1, engine);
    ret = engine->invoke(ctor, { ret }); /// TODO : define behavior of invoking a constructor
  }

  return apply_standard_conversion(ret, dest, conv.conv3, engine);
}

static Value apply_standard_conversion2(const Value & arg, const StandardConversion2 & conv, Engine *engine)
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

static Value apply_conversion2(const Value & arg, const Conversion & conv, Engine *engine)
{
  if (!conv.isUserDefinedConversion())
    return apply_standard_conversion2(arg, conv.firstStandardConversion(), engine);

  Value ret = apply_standard_conversion2(arg, conv.firstStandardConversion(), engine);

  /// TODO : we need to define the behavior of invoking a constructor
  // since conv.userDefinedConversion() can be a ctor
  // Should it return the new object has it does currently
  // or should it take an uninitialized object + the args and return void
  // I think the second option is better and should be implemented
  ret = engine->invoke(conv.userDefinedConversion(), { ret }); 

  return apply_standard_conversion2(ret, conv.secondStandardConversion(), engine);
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
  this->search_dir = support::filesystem::current_path();
}

Object EngineImpl::createObject(Type t)
{
  return createObject(engine->getClass(t));
}

Object EngineImpl::createObject(Class cla)
{
  auto impl = std::make_shared<ObjectImpl>(cla);
  return Object{ impl };
}

Value EngineImpl::default_construct(const Type & t, const Function & ctor)
{
  if (!ctor.isNull())
    return engine->invoke(ctor, { });
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

void EngineImpl::placement(const Function & ctor, Value object, const std::vector<Value> & args)
{
  this->interpreter->placement(ctor, object, args.begin(), args.end());
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
  destroy(Namespace{ impl });
  
  impl->globalNames.clear();
  impl->global_types.clear();

  const int index = s.id();
  this->scripts[index] = Script{};
  while (!this->scripts.empty() && this->scripts.back().isNull())
    this->scripts.pop_back();
}

void EngineImpl::destroy(ClosureType ct)
{
  auto impl = ct.impl();

  impl->operators.clear();
  impl->templates.clear(); /// TODO: clear the template instances

  unregister_closure(ct);

  impl->name.insert(0, "deleted_");
  impl->enclosing_symbol = std::weak_ptr<SymbolImpl>();
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


Engine::Engine()
{
  d = std::unique_ptr<EngineImpl>(new EngineImpl{ this });
}

Engine::~Engine()
{
  d->rootNamespace = Namespace{};

  for (auto m : d->modules)
    m.destroy();
  d->modules.clear();

  garbageCollect();
}


ClassTemplate register_initialize_list_template(Engine*); // defined in initializerlist.cpp

void Engine::setup()
{
  d->context = Context{ std::make_shared<ContextImpl>(this, 0, "main_context") };

  d->rootNamespace = Namespace{ std::make_shared<NamespaceImpl>("", this) };

  register_builtin_operators(d->rootNamespace);

  d->reservations.class_type = Class{ std::make_shared<ClassImpl>(0, "__reserved_class__", nullptr) };
  d->classes.push_back(d->reservations.class_type);
  d->reservations.enum_type = Enum{ std::make_shared<EnumImpl>(0, "__reserved_enum__", nullptr) };
  d->enums.push_back(d->reservations.enum_type);

  Class string = Symbol{ d->rootNamespace }.Class(get_string_typename()).setId(Type::String).get();
  register_string_type(string);

  d->templates.array = ArrayImpl::register_array_template(this);
  d->templates.initializer_list = register_initialize_list_template(this);

  auto ec = std::make_shared<interpreter::ExecutionContext>(this, 1024, 256);
  d->interpreter = std::unique_ptr<interpreter::Interpreter>(new interpreter::Interpreter{ ec, this });
}

Value Engine::newBool(bool bval)
{
  Value v{ new ValueImpl{Type::Boolean, this} };
  v.impl()->set_bool(bval);
  return v;
}

Value Engine::newChar(char cval)
{
  Value v{ new ValueImpl{ Type::Char, this } };
  v.impl()->set_char(cval);
  return v;
}

Value Engine::newInt(int ival)
{
  Value v{ new ValueImpl{ Type::Int, this } };
  v.impl()->set_int(ival);
  return v;
}

Value Engine::newFloat(float fval)
{
  Value v{ new ValueImpl{ Type::Float, this } };
  v.impl()->set_float(fval);
  return v;
}

Value Engine::newDouble(double dval)
{
  Value v{ new ValueImpl{ Type::Double, this } };
  v.impl()->set_double(dval);
  return v;
}

Value Engine::newString(const String & sval)
{
  Value v{ new ValueImpl{ Type::String, this } };
  v.impl()->set_string(sval);
  return v;
}

Array Engine::newArray(ArrayType array_type)
{
  Class array_class = getClass(array_type.type);
  auto data = std::dynamic_pointer_cast<SharedArrayData>(array_class.data());
  auto impl = std::make_shared<ArrayImpl>(data->data, this);
  return Array{ impl };
}

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

  throw std::runtime_error{ "default_construct_fundamental : Implementation error" };
}

Value Engine::construct(Type t, const std::vector<Value> & args)
{
  if (t.isObjectType())
  {
    Class cla = getClass(t);
    const auto & ctors = cla.constructors();
    Function selected = OverloadResolution::select(ctors, args);
    if (selected.isNull())
      throw std::runtime_error{ "No valid constructor could be found" };
    else if (selected.isDeleted())
      throw std::runtime_error{ "The selected constructor is deleted" };

    return call(selected, args);
  }
  else if (t.isFundamentalType())
  {
    if (args.size() > 1)
      throw std::runtime_error{ "Too many arguments provided" };

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
    if (args.size() != 1)
      throw std::runtime_error{ "Incorrect argument count" };

    Value arg = args.front();
    if (arg.type().baseType() != t.baseType())
      throw std::runtime_error{ "Could not construct enumeration from a different enumeration-type" };
    return copy(arg);
  }

  throw std::runtime_error{ "Could not construct value of given type with prodived arguments" };
}

Value Engine::uninitialized(const Type & t)
{
  return buildValue(t.withFlag(Type::UninitializedFlag));
}

void Engine::initialize(Value & memory)
{
  const Type & t = memory.type();
  if (t.isFundamentalType())
  {
    switch (t.baseType().data())
    {
    case Type::Boolean:
      memory.impl()->set_bool(false);
      break;
    case Type::Char:
      memory.impl()->set_char('\0');
      break;
    case Type::Int:
      memory.impl()->set_int(0);
      break;
    case Type::Float:
      memory.impl()->set_float(0.f);
      break;
    case Type::Double:
      memory.impl()->set_double(0.);
      break;
    default:
      throw std::runtime_error{ "Engine::initialize() : fundamental type not implemented" };
    }
  }
  else if(t.isObjectType())
  {
    Class cla = getClass(t);
    Function ctor = cla.defaultConstructor();
    if (ctor.isNull())
      throw std::runtime_error{ "Class has no default constructor" };
    else if (ctor.isDeleted())
      throw std::runtime_error{ "Class has a deleted default constructor" };

    d->interpreter->placement(ctor, memory, &memory, &memory);
  }
  else
    throw std::runtime_error{ "Engine::initialize() : type not supported" };

  memory.impl()->type = memory.type().withoutFlag(Type::UninitializedFlag);
}

void Engine::uninitialized_copy(const Value & value, Value & memory)
{
  if (value.type() != memory.type())
    throw std::runtime_error{ "Engine::uninitialized_copy() : types don't match" };

  const Type t = memory.type();
  if (t.isObjectType())
  {
    Class cla = getClass(t);
    Function copy_ctor = cla.copyConstructor();
    if(copy_ctor.isNull())
      throw std::runtime_error{ "Class has no copy constructor" };
    else if(copy_ctor.isDeleted())
      throw std::runtime_error{ "Class has a deleted copy constructor" };

    d->interpreter->placement(copy_ctor, memory, &value, (&value)+1);
  }
  else if (t.isFundamentalType())
  {
    switch (t.baseType().data())
    {
    case Type::Boolean:
      memory.impl()->set_bool(value.toBool());
      break;
    case Type::Char:
      memory.impl()->set_char(value.toChar());
      break;
    case Type::Int:
      memory.impl()->set_int(value.toInt());
      break;
    case Type::Float:
      memory.impl()->set_float(value.toFloat());
      break;
    case Type::Double:
      memory.impl()->set_double(value.toDouble());
      break;
    default:
      throw std::runtime_error{ "Engine::uninitialized_copy() : fundamental type not implemented" };
    }
  }
  else if (t.isEnumType())
  {
    memory.impl()->set_enumerator(value.toEnumerator());
  }
  else
    throw std::runtime_error{ "Engine::uninitialized_copy() : case not implemented" };

  memory.impl()->type = memory.type().withoutFlag(Type::UninitializedFlag);
}

void Engine::emplace(Value & memory, Function ctor, const std::vector<Value> & args)
{
  d->interpreter->placement(ctor, memory, args.begin(), args.end());
  memory.impl()->type = memory.type().withoutFlag(Type::UninitializedFlag);
}

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

void Engine::manage(Value val)
{
  if (val.isManaged())
    return;

  d->garbageCollector.push_back(val);
  val.impl()->type = val.impl()->type.withFlag(Type::ManagedFlag);
}

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

  throw std::runtime_error{ "copy_fundamental: Implementation error" };
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
      throw std::runtime_error{ "Value's type is not copy constructible." };

    return invoke(copyCtor, { val });
  }
  else if (val.type().isFunctionType())
    return Value::fromFunction(val.toFunction(), val.type().baseType());
  else if (val.type().isClosureType())
    return Value::fromLambda(copy_lambda(val.toLambda(), this));

  /// TODO : implement copy of closures and functions

  throw std::runtime_error{ "Cannot copy given value" };
}

ConversionSequence Engine::conversion(const Type & src, const Type & dest)
{
  return ConversionSequence::compute(src, dest, this);
}

ConversionSequence Engine::conversion(const std::shared_ptr<program::Expression> & expr, const Type & dest)
{
  return ConversionSequence::compute(expr, dest, this);
}

Conversion Engine::conversion2(const Type & src, const Type & dest)
{
  return Conversion::compute(src, dest, this);
}

void Engine::applyConversions(std::vector<script::Value> & values, const std::vector<Type> & types, const std::vector<ConversionSequence> & conversions)
{
  for (size_t i(0); i < values.size(); ++i)
    values[i] = apply_conversion(values.at(i), types.at(i), conversions.at(i), this);
}

void Engine::applyConversions(std::vector<script::Value> & values, const std::vector<Conversion> & conversions)
{
  for (size_t i(0); i < values.size(); ++i)
    values[i] = apply_conversion2(values.at(i), conversions.at(i), this);
}

bool Engine::canCast(const Type & srcType, const Type & destType)
{
  return conversion2(srcType, destType).rank() != ConversionRank::NotConvertible;
}

Value Engine::cast(const Value & val, const Type & destType)
{
  Conversion conv = conversion2(val.type(), destType);
  if (conv.isInvalid())
    throw std::runtime_error{ "Could not convert value to desired type" }; /// TODO : emit better diagnostic

  return apply_conversion2(val, conv, this);
}

Namespace Engine::rootNamespace() const
{
  return d->rootNamespace;
}

FunctionType Engine::getFunctionType(Type id) const
{
  const int index = id.data() & 0xFFFF;
  return d->prototypes[index];
}

FunctionType Engine::getFunctionType(const Prototype & proto)
{
  for (const auto & ft : d->prototypes)
  {
    if (ft.prototype() == proto)
      return ft;
  }

  return newFunctionType(proto);
}

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

Class Engine::getClass(Type id) const
{
  if (!id.isObjectType())
    return Class{};

  int index = id.data() & 0xFFFF;
  return d->classes[index];
}

Enum Engine::getEnum(Type id) const
{
  if (!id.isEnumType())
    return Enum{};

  int index = id.data() & 0xFFFF;
  return d->enums[index];
}

ClosureType Engine::getLambda(Type id) const
{
  if (!id.isClosureType())
    return ClosureType{};

  int index = id.data() & 0xFFFF;
  return d->lambdas[index];
}

template<typename T>
void reserve_range(std::vector<T> & list, const T & value, size_t begin, size_t end)
{
  if (list.size() < end)
    list.resize(end);

  for (size_t i(begin); i < end; ++i)
  {
    if (!list.at(i).isNull() && list.at(i) != value)
      throw std::runtime_error{ "Failed to reserve type range" };
    list[i] = value;
  }
}

void Engine::reserveTypeRange(int begin, int end)
{
  const Type begin_type{ begin };
  const Type end_type{ end };

  if (!begin_type.isValid() || !end_type.isValid() || begin_type.category() != end_type.category())
    throw std::runtime_error{ "Invalid type range" };

  if (begin_type.isFunctionType() || begin_type.isClosureType())
    throw std::runtime_error{ "Closure types and function types cannot be reserved yet" };

  const int index_mask = Type::EnumFlag - 1;

  begin = begin & index_mask;
  end = end & index_mask;

  if (begin_type.isEnumType())
    reserve_range(d->enums, d->reservations.enum_type, begin, end);
  else if (begin_type.isObjectType())
    reserve_range(d->classes, d->reservations.class_type, begin, end);
}

FunctionType Engine::newFunctionType(const Prototype & proto)
{
  const int id = d->prototypes.size();
  Type type{ id | Type::PrototypeFlag };

  Prototype assign_proto;
  assign_proto.setReturnType(Type::ref(type));
  assign_proto.addParameter(Type::ref(type));
  assign_proto.addParameter(Type::cref(type));
  auto assign_op = std::make_shared<OperatorImpl>(AssignmentOperator, assign_proto, this);
  assign_op->set_impl(callbacks::function_variable_assignment);

  FunctionType ret{ type, proto, Operator{ assign_op } };
  d->prototypes.push_back(ret);
  return ret;
}

Script Engine::newScript(const SourceFile & source)
{
  Script ret{ std::make_shared<ScriptImpl>(d->scripts.size(), this, source) };
  d->scripts.push_back(ret);
  return ret;
}

bool Engine::compile(Script s)
{
  compiler::Compiler c{ this };
  return c.compile(s);
}

void Engine::destroy(Script s)
{
  d->destroy(s);
}

Module Engine::newModule(const std::string & name)
{
  Module m{ std::make_shared<ModuleImpl>(this, name) };
  d->modules.push_back(m);
  return m;
}

Module Engine::newModule(const std::string & name, ModuleLoadFunction load, ModuleCleanupFunction cleanup)
{
  Module m{ std::make_shared<ModuleImpl>(this, name, load, cleanup) };
  d->modules.push_back(m);
  return m;
}

const std::vector<Module> & Engine::modules() const
{
  return d->modules;
}

Module Engine::getModule(const std::string & name)
{
  for (const auto & child : d->modules)
  {
    if (child.name() == name)
      return child;
  }

  return Module{};
}

const std::string & Engine::scriptExtension() const
{
  return d->script_extension;
}

void Engine::setScriptExtension(const std::string & ex)
{
  d->script_extension = ex;
}

const support::filesystem::path & Engine::searchDirectory() const
{
  return d->search_dir;
}

void Engine::setSearchDirectory(const support::filesystem::path & dir)
{
  d->search_dir = dir;
}

Namespace Engine::enclosingNamespace(Type t) const
{
  if (t.isFundamentalType() || t.isClosureType() || t.isFunctionType())
    return this->rootNamespace();
  else if (t.isObjectType())
    return getClass(t).enclosingNamespace();
  else if (t.isEnumType())
    return getEnum(t).enclosingNamespace();

  throw std::runtime_error{ "Engine::enclosingNamespace() : type not supported" };
}

Type Engine::typeId(const std::string & typeName, const Scope & scope) const
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

  NameLookup lookup = NameLookup::resolve(typeName, scope);
  Type t = lookup.typeResult();
  if (t.isNull())
    throw std::runtime_error{ "Engine::typeId() : invalid type name" };
  return t;
}

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

  throw std::runtime_error{ "Engine::typeName() : Unknown type" };
}

Context Engine::newContext()
{
  Context c{ std::make_shared<ContextImpl>(this, d->allContexts.size() + 1, "") };
  d->allContexts.push_back(c);
  return c;
}

Context Engine::currentContext() const
{
  return d->context;
}

void Engine::setContext(Context con)
{
  d->context = con;
}

Value Engine::eval(const std::string & command, const Scope & scp)
{
  compiler::Compiler c{ this };
  auto expr = c.compile(command, d->context, scp);
  if (expr == nullptr)
    throw std::runtime_error{ "Could not compile expression" };
  return d->interpreter->eval(expr);
}

Value Engine::call(const Function & f, std::initializer_list<Value> && args)
{
  return d->interpreter->call(f, nullptr, args.begin(), args.end());
}

Value Engine::call(const Function & f, const std::vector<Value> & args)
{
  return d->interpreter->call(f, args);
}

Value Engine::invoke(const Function & f, std::initializer_list<Value> && args)
{
  return d->interpreter->invoke(f, nullptr, args.begin(), args.end());
}

Value Engine::invoke(const Function & f, const std::vector<Value> & args)
{
  return d->interpreter->invoke(f, args.begin(), args.end());
}

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

bool Engine::isInitializerListType(const Type & t) const
{
  if (!t.isObjectType())
    return false;

  return getClass(t).isTemplateInstance() && getClass(t).instanceOf() == getTemplate(InitializerListTemplate);
}

const std::vector<Script> & Engine::scripts() const
{
  return d->scripts;
}

EngineImpl * Engine::implementation() const
{
  return d.get();
}


Value EngineImpl::buildValue(Type t)
{
  Value v{ new ValueImpl{ t, this->engine } };
  return v;
}

Value Engine::buildValue(Type t)
{
  return d->buildValue(t);
}

} // namespace script
