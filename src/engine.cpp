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
#include "script/locals.h"
#include "script/module.h"
#include "script/namelookup.h"
#include "script/namespace.h"
#include "script/object.h"
#include "script/operator.h"
#include "script/overloadresolution.h"
#include "script/scope.h"
#include "script/script.h"
#include "script/string.h"
#include "script/typesystem.h"
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
#include "script/private/template_p.h"
#include "script/private/typesystem_p.h"
#include "script/private/value_p.h"

namespace script
{

EngineImpl::EngineImpl(Engine *e)
  : engine(e)
  , garbage_collector_running(false)
{

}

Value EngineImpl::default_construct(const Type & t, const Function & ctor)
{
  if (!ctor.isNull())
  {
    Value ret = engine->allocate(t.withoutRef());
    ctor.invoke({ ret });
    return ret;
  }
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
  {
    Value ret = engine->allocate(val.type());
    copyctor.invoke({ ret, val });
    return ret;
  }
  else
  {
    return engine->copy(val);
  }
}

void EngineImpl::destroy(const Value & val, const Function & dtor)
{
  auto *impl = val.impl();

  if (impl->type.isObjectType())
  {
    dtor.invoke({ val });
  }

  impl->clear();

  impl->type = 0;
  impl->engine = nullptr;
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
    typesystem->impl()->destroy(e);
  impl->enums.clear();

  for (const auto & nns : impl->namespaces)
    destroy(nns);
  impl->namespaces.clear();

  for (const auto & c : impl->classes)
    typesystem->impl()->destroy(c);
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

namespace errors
{

class EngineCategory : public std::error_category
{
public:

  const char* name() const noexcept override
  {
    return "engine-category";
  }

  std::string message(int) const override
  {
    return "engine-error";
  }
};


const std::error_category& engine_category() noexcept
{
  static const EngineCategory static_instance = {};
  return static_instance;
}

} // namespace errors

EngineError::EngineError(ErrorCode ec)
  : Exceptional(ec)
{

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

  d->typesystem = nullptr;
}


ClassTemplate register_initialize_list_template(Engine*); // defined in initializerlist.cpp

void Engine::setup()
{
  d->typesystem = std::unique_ptr<TypeSystem>(new TypeSystem(TypeSystemImpl::create(this)));

  d->rootNamespace = Namespace{ std::make_shared<NamespaceImpl>("", this) };
  d->context = Context{ std::make_shared<ContextImpl>(this, 0, "default_context") };

  register_builtin_operators(d->rootNamespace);

  Class string = Symbol{ d->rootNamespace }.newClass(StringBackend::class_name()).setId(Type::String).get();
  StringBackend::register_string_type(string);

  d->templates.array = ArrayImpl::register_array_template(this);
  d->templates.initializer_list = register_initialize_list_template(this);

  d->compiler = std::unique_ptr<compiler::Compiler>(new compiler::Compiler{ this });

  auto ec = std::make_shared<interpreter::ExecutionContext>(this, 1024, 256);
  d->interpreter = std::unique_ptr<interpreter::Interpreter>(new interpreter::Interpreter{ ec, this });
}

/*!
 * \fn TypeSystem* typeSystem() const
 * \brief Returns the engine's typesystem.
 */
TypeSystem* Engine::typeSystem() const
{
  return d->typesystem.get();
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
  Class array_class = typeSystem()->getClass(array_type.type);
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

extern Value fundamental_conversion(const Value& src, int destType, Engine* e); // defined in conversions.cpp

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
    Class cla = typeSystem()->getClass(t);
    const auto & ctors = cla.constructors();
    Function selected = OverloadResolution::selectConstructor(ctors, args);
    if (selected.isNull())
      throw ConstructionError{EngineError::NoMatchingConstructor };
    else if (selected.isDeleted())
      throw ConstructionError{ EngineError::ConstructorIsDeleted };

    Value result = allocate(t.withoutRef());

    Locals arguments;
    arguments.push(result);

    for (const auto& a : args)
    {
      arguments.push(a);
    }

    selected.call(arguments);

    return result;
  }
  else if (t.isFundamentalType())
  {
    if (args.size() > 1)
      throw ConstructionError{ EngineError::TooManyArgumentInInitialization };

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
      throw ConstructionError{ EngineError::TooManyArgumentInInitialization };
    else if(args.size() == 0)
      throw ConstructionError{ EngineError::TooFewArgumentInInitialization };

    Value arg = args.front();
    if (arg.type().baseType() != t.baseType())
      throw ConstructionError{ EngineError::NoMatchingConstructor };
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
    Function dtor = typeSystem()->getClass(val.type()).destructor();
    dtor.invoke({ val });
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
 * This function internally asks TypeSystem::isCopyConstructible().
 */
bool Engine::canCopy(const Type & t)
{
  return typeSystem()->isCopyConstructible(t);
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
    Class cla = typeSystem()->getClass(val.type());
    Function copyCtor = cla.copyConstructor();
    if (copyCtor.isNull() || copyCtor.isDeleted())
      throw CopyError{};

    Value object = allocate(cla.id());
    copyCtor.invoke({ object, val });
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
 * \fn bool canConvert(const Type & srcType, const Type & destType)
 * \param source type
 * \param dest type
 * \brief Checks if a conversion is possible.
 *
 */
bool Engine::canConvert(const Type& srcType, const Type& destType) const
{
  return typeSystem()->canConvert(srcType, destType);
}

/*!
 * \fn Value convert(const Value & val, const Type & destType)
 * \param input value
 * \param dest type
 * \brief Converts a value to the given type.
 *
 * Throws \t ConversionError on failure.
 */
Value Engine::convert(const Value& val, const Type& type)
{
  Conversion conv = typeSystem()->conversion(val.type(), type);
  if (conv.isInvalid())
    throw ConversionError{};

  return Conversion::apply(conv, val);
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
 * \fn compiler::Compiler* compiler() const
 * \brief Returns the engine's compiler.
 *
 */
compiler::Compiler* Engine::compiler() const
{
  return d->compiler.get();
}

/*!
 * \fn interpreter::Interpreter* interpreter() const
 * \brief Returns the engine's interpreter.
 *
 */
interpreter::Interpreter* Engine::interpreter() const
{
  return d->interpreter.get();
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
 * \fn Type typeId(const std::string & typeName, const Scope & scope) const
 * \param type name
 * \param scope
 * \brief Searchs for a type by name.
 *
 * Throws \t UnknownTypeError if the type could not be resolved.
 */
Type Engine::typeId(const std::string & typeName, Scope scope) const
{
  return typeSystem()->typeId(typeName, scope);
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
  catch (compiler::CompilationFailure& ex)
  {
    diagnostic::MessageBuilder msb{ diagnostic::Error, this };
    msb << ex;
    diagnostic::DiagnosticMessage mssg = msb.build();

    throw EvaluationError{ mssg.to_string() };
  }

  return d->interpreter->eval(expr);
}

const std::map<std::type_index, Template>& Engine::templateMap() const
{
  return d->templates.dict;
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
