// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/engine.h"
#include "engine_p.h"

#include "script/array.h"
#include "array_p.h"
#include "builtinoperators.h"
#include "script/cast.h"
#include "cast_p.h"
#include "script/class.h"
#include "class_p.h"
#include "script/context.h"
#include "context_p.h"
#include "script/enum.h"
#include "enum_p.h"
#include "script/enumvalue.h"
#include "script/function.h"
#include "function_p.h"
#include "script/functionbuilder.h"
#include "script/functiontype.h"
#include "script/lambda.h"
#include "lambda_p.h"
#include "script/namelookup.h"
#include "script/namespace.h"
#include "namespace_p.h"
#include "script/object.h"
#include "object_p.h"
#include "script/operator.h"
#include "operator_p.h"
#include "script/overloadresolution.h"
#include "script/scope.h"
#include "scope_p.h"
#include "string_p.h"
#include "template_p.h"
#include "script/value.h"
#include "value_p.h"

#include "script/compiler/compiler.h"

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
    return (T) v.toEnumValue().value();

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
    ret = apply_standard_conversion(arg, ctor.prototype().argv(0), conv.conv1, engine);
    ret = engine->invoke(ctor, { ret }); /// TODO : define behavior of invoking a constructor
  }

  return apply_standard_conversion(ret, dest, conv.conv3, engine);
}

namespace callbacks
{

Value function_variable_assignment(interpreter::FunctionCall *c)
{
  c->arg(0).impl()->setFunction(c->arg(1).toFunction());
  return c->arg(0);
}

} // namespace callbacks


EngineImpl::EngineImpl(Engine *e)
  : engine(e)
  , garbage_collector_running(false)
{

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

void EngineImpl::finalizeObject(Object /* o */, Type /* objectType */)
{

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

Lambda EngineImpl::newLambda()
{
  const int id = static_cast<int>(this->lambdas.size()) | Type::LambdaFlag;
  const int index = id & 0xFFFF;
  Lambda l{ std::make_shared<LambdaImpl>(id) };
  this->lambdas.push_back(l);
  return l;
}

void EngineImpl::destroyClass(Class c)
{
  Scope decl = engine->scope(c);
  if (decl.isNull())
    return;
  decl.impl()->remove_class(c);
  auto & name = c.implementation()->name;
  name.insert(0, "deleted_");
  const int index = c.id() & 0xFFFF;
  this->classes[index] = Class();
  while (!this->classes.empty() && this->classes.back().isNull())
    this->classes.pop_back();
  c.implementation()->id = 0;
}

void EngineImpl::destroyEnum(Enum e)
{
  Scope decl = engine->scope(e);
  if (decl.isNull())
    return;
  decl.impl()->remove_enum(e);
  auto & name = e.implementation()->name;
  name.insert(0, "deleted_");
  const int index = e.id() & 0xFFFF;
  this->enums[index] = Enum();
  while (!this->enums.empty() && this->enums.back().isNull())
    this->enums.pop_back();
  e.implementation()->id = 0;
}


Engine::Engine()
{
  d = std::unique_ptr<EngineImpl>(new EngineImpl{ this });
}

Engine::~Engine()
{
  d->rootNamespace = Namespace{};
  garbageCollect();
}


void Engine::setup()
{
  d->context = Context{ std::make_shared<ContextImpl>(this, 0, "main_context") };

  d->rootNamespace = Namespace{ std::make_shared<NamespaceImpl>("", this) };

  register_builtin_operators(d->rootNamespace);

  Class string = buildClass(ClassBuilder::New(get_string_typename()), Type::String);
  register_string_type(string);
  d->rootNamespace.implementation()->classes.push_back(string);

  d->templates.array = ArrayImpl::register_array_template(this);

  auto ec = std::make_shared<interpreter::ExecutionContext>(this, 1024, 256);
  d->interpreter = std::unique_ptr<interpreter::Interpreter>(new interpreter::Interpreter{ ec, this });
}

Value Engine::newBool(bool bval)
{
  Value v{ new ValueStruct{Type::Boolean, this} };
  v.impl()->setBool(bval);
  return v;
}

Value Engine::newChar(char cval)
{
  Value v{ new ValueStruct{ Type::Char, this } };
  v.impl()->setChar(cval);
  return v;
}

Value Engine::newInt(int ival)
{
  Value v{ new ValueStruct{ Type::Int, this } };
  v.impl()->setInt(ival);
  return v;
}

Value Engine::newFloat(float fval)
{
  Value v{ new ValueStruct{ Type::Float, this } };
  v.impl()->setFloat(fval);
  return v;
}

Value Engine::newDouble(double dval)
{
  Value v{ new ValueStruct{ Type::Double, this } };
  v.impl()->setDouble(dval);
  return v;
}

Value Engine::newString(const String & sval)
{
  Value v{ new ValueStruct{ Type::String, this } };
  v.impl()->setString(sval);
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
  Class array_class = array.getInstance({ TemplateArgument::make(element_type.type.baseType()) });
  auto data = std::dynamic_pointer_cast<SharedArrayData>(array_class.data());
  auto impl = std::make_shared<ArrayImpl>(data->data, this);
  return Array{ impl };
}

const Engine::fail_if_not_instantiated_t Engine::FailIfNotInstantiated;

Array Engine::newArray(ElementType element_type, fail_if_not_instantiated_t)
{
  ClassTemplate array = d->templates.array.asClassTemplate();
  Class arrayClass;
  if (!array.hasInstance({ TemplateArgument::make(element_type.type.baseType()) }, &arrayClass))
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
  val.impl()->setManaged(true);
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
  return Value::fromEnumValue(val.toEnumValue());
}

static LambdaObject copy_lambda(const LambdaObject & l, Engine *e)
{
  auto ret = std::make_shared<LambdaObjectImpl>(l.closureType());
  Lambda closure_type = l.closureType();
  for (size_t i(0); i < l.captures().size(); ++i)
  {
    if (closure_type.captures().at(i).type.isReference())
      ret->captures.push_back(l.captures().at(i));
    else
      ret->captures.push_back(e->copy(l.captures().at(i)));
  }
  return LambdaObject{ ret };
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

void Engine::applyConversions(std::vector<script::Value> & values, const std::vector<Type> & types, const std::vector<ConversionSequence> & conversions)
{
  for (size_t i(0); i < values.size(); ++i)
    values[i] = apply_conversion(values.at(i), types.at(i), conversions.at(i), this);
}

bool Engine::canCast(const Type & srcType, const Type & destType)
{
  return conversion(srcType, destType).rank() != ConversionSequence::Rank::NotConvertible;
}

Value Engine::cast(const Value & val, const Type & destType)
{
  ConversionSequence conv = conversion(val.type(), destType);
  if (conv == ConversionSequence::NotConvertible())
    throw std::runtime_error{ "Could not convert value to desired type" }; /// TODO : emit better diagnostic

  return apply_conversion(val, destType, conv, this);
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

Lambda Engine::getLambda(Type id) const
{
  if (!id.isClosureType())
    return Lambda{};

  int index = id.data() & 0xFFFF;
  return d->lambdas[index];
}

FunctionType Engine::newFunctionType(const Prototype & proto)
{
  const int id = d->prototypes.size();
  Type type{ id | Type::PrototypeFlag };

  Prototype assign_proto;
  assign_proto.setReturnType(Type::ref(type));
  assign_proto.addArgument(Type::ref(type));
  assign_proto.addArgument(Type::cref(type));
  auto assign_op = std::make_shared<OperatorImpl>(Operator::AssignmentOperator, assign_proto, this);
  assign_op->set_impl(callbacks::function_variable_assignment);

  FunctionType ret{ type, proto, Operator{ assign_op } };
  d->prototypes.push_back(ret);
  return ret;
}

Enum Engine::newEnum(const std::string & name)
{
  const int id = d->enums.size();
  auto impl = std::make_shared<EnumImpl>(id | Type::EnumFlag, name, this);
  Enum ret{ impl };
  d->enums.push_back(ret);
  return ret;
}

Class Engine::newClass(const ClassBuilder &opts)
{
  return buildClass(opts);
}

Operator EngineImpl::newBuiltInOperator(Operator::BuiltInOperator op, const Prototype & proto, NativeFunctionSignature impl)
{
  auto ret = std::make_shared<BuiltInOperatorImpl>(op, proto, engine);
  ret->implementation.callback = impl;
  return Operator{ ret };
}

Function EngineImpl::newConstructor(const FunctionBuilder & opts)
{
  if (opts.special.isNull())
    throw std::runtime_error{ "Cannot create a constructor with invalid class" };

  auto impl = std::make_shared<ConstructorImpl>(opts.special, opts.proto, engine, opts.flags);
  impl->implementation.callback = opts.callback;
  impl->data = opts.data;
  return Function{ impl };
}

Function EngineImpl::newDestructor(const FunctionBuilder & opts)
{
  if (opts.special.isNull())
    throw std::runtime_error{ "Cannot create a constructor with invalid class" };

  auto impl = std::make_shared<DestructorImpl>(opts.special, opts.proto, engine, opts.flags);
  impl->implementation.callback = opts.callback;
  impl->data = opts.data;
  return Function{ impl };
}

Function EngineImpl::newFunction(const FunctionBuilder & opts)
{
  auto impl = std::make_shared<RegularFunctionImpl>(opts.name, opts.proto, engine, opts.flags);
  impl->implementation.callback = opts.callback;
  impl->data = opts.data;
  return Function{ impl };
}

Function EngineImpl::newOperator(const FunctionBuilder & opts)
{
  auto impl = std::make_shared<OperatorImpl>(opts.operation, opts.proto, engine, opts.flags);
  impl->implementation.callback = opts.callback;
  impl->data = opts.data;
  return Function{ impl };
}

Function EngineImpl::newCast(const FunctionBuilder & opts)
{
  auto impl = std::make_shared<CastImpl>(opts.proto, engine, opts.flags);
  impl->implementation.callback = opts.callback;
  impl->data = opts.data;
  return Function{ impl };
}

Function Engine::newFunction(const FunctionBuilder & opts)
{
  switch (opts.kind)
  {
  case Function::Constructor:
    return d->newConstructor(opts);
  case Function::Destructor:
    return d->newDestructor(opts);
  case Function::CastFunction:
    return d->newCast(opts);
  case Function::StandardFunction:
    return d->newFunction(opts);
  case Function::OperatorFunction:
    return d->newOperator(opts);
  default:
    break;
  }

  throw std::runtime_error{ "Engine::newFunction() missing function kind" };
}

Script Engine::newScript(const SourceFile & source)
{
  auto ret = std::make_shared<ScriptImpl>(this, source);
  ret->root = Namespace{ std::make_shared<NamespaceImpl>(std::string{}, this) };
  ret->root.implementation()->script = ret;
  d->scripts.push_back(Script{ ret });
  return Script{ ret };
}

bool Engine::compile(Script s)
{
  compiler::Compiler c{ this };
  return c.compile(s);
}

Scope Engine::scope(const Class & cla)
{
  return Scope::find(cla);
}

Scope Engine::scope(const Enum & e)
{
  return Scope::find(e);
}

Scope Engine::scope(Type type)
{
  if (type.isObjectType())
    return scope(getClass(type));
  else if (type.isEnumType())
    return scope(getEnum(type));
  else if (type.isFundamentalType())
    return Scope{ this->rootNamespace() };

  throw std::runtime_error{ "Engine::scope() : scope of closure type and function type not implemented" };
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

Value Engine::eval(const std::string & command, const Script & s)
{
  compiler::Compiler c{ this };
  auto expr = c.compile(command, d->context, s);
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

FunctionTemplate Engine::newFunctionTemplate(const std::string & name, NativeTemplateDeductionFunction deduc, NativeFunctionTemplateInstantiationFunction callback)
{
  return FunctionTemplate{ std::make_shared<FunctionTemplateImpl>(name, deduc, callback, this, nullptr) };
}


ClassTemplate Engine::newClassTemplate(const std::string & name, NativeClassTemplateInstantiationFunction callback)
{
  return ClassTemplate{ std::make_shared<ClassTemplateImpl>(name, callback, this, nullptr) };
}

const Engine::array_template_t Engine::ArrayTemplate = Engine::array_template_t{};

ClassTemplate Engine::getTemplate(array_template_t) const
{
  return d->templates.array;
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
  Value v{ new ValueStruct{ t, this->engine } };
  return v;
}

Value Engine::buildValue(Type t)
{
  return d->buildValue(t);
}

Class Engine::buildClass(const ClassBuilder & opts, int id)
{
  if (id == -1)
    id = static_cast<int>(d->classes.size()) | Type::ObjectFlag;

  int index = id & 0xFFFF;
  if (static_cast<int>(d->classes.size()) <= index)
    d->classes.resize(index + 1);
  else
  {
    if (!d->classes[index].isNull())
      throw std::runtime_error{ "Engine::buildClass() : Class id already used" };
  }

  Class ret{ std::make_shared<ClassImpl>(id, opts.name, this) };
  ret.implementation()->set_parent(opts.parent);
  ret.implementation()->dataMembers = opts.dataMembers;
  ret.implementation()->isFinal = opts.isFinal;
  ret.implementation()->data = opts.userdata;

  d->classes[index] = ret;

  return ret;
}


} // namespace script
