// Copyright (C) 2018-2022 Vincent Chambrin
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
#include "script/private/operator_p.h"
#include "script/private/scope_p.h"
#include "script/private/script_p.h"
#include "script/private/template_p.h"
#include "script/private/typesystem_p.h"
#include "script/private/value_p.h"

#include <sstream>

namespace script
{

EngineImpl::EngineImpl(Engine *e)
  : engine(e)
{

}

Value EngineImpl::default_construct(const Type & t, const Function & ctor)
{
  if (!ctor.isNull())
  {
    return ctor.invoke({ Value() });
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
    return copyctor.invoke({ Value(), val });
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

  std::string message(int code) const override
  {
    switch (static_cast<EngineError::ErrorCode>(code))
    {
    case EngineError::NotImplemented:
      return "not implemented";
    case EngineError::RuntimeError:
      return "runtime error";
    case EngineError::EvaluationError:
      return "evaluation error";
    case EngineError::ConversionError:
      return "conversion error";
    case EngineError::CopyError:
      return "copy error";
    case EngineError::UnknownType:
      return "unknown type";
    case EngineError::NoMatchingConstructor:
      return "no matching constructor";
    case EngineError::ConstructorIsDeleted:
      return "constructor is deleted";
    case EngineError::TooManyArgumentInInitialization:
      return "too many argument in initialization";
    case EngineError::TooFewArgumentInInitialization:
      return "too few argument in initialization";
    default:
      return "unknown engine error";
    }
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
 * This function calls tearDown().
 */
Engine::~Engine()
{
  tearDown();
}


ClassTemplate register_initialize_list_template(Engine*); // defined in initializerlist.cpp

void Engine::setup()
{
  d->typesystem = std::unique_ptr<TypeSystem>(new TypeSystem(TypeSystemImpl::create(this)));

  d->rootNamespace = Namespace{ std::make_shared<NamespaceImpl>("", this) };
  d->context = Context{ std::make_shared<ContextImpl>(this, 0, "default_context") };

  register_type(std::type_index(typeid(void)), Type(Type::Void), "void");
  register_type(std::type_index(typeid(bool)), Type(Type::Boolean), "bool");
  register_type(std::type_index(typeid(char)), Type(Type::Char), "char");
  register_type(std::type_index(typeid(int)), Type(Type::Int), "int");
  register_type(std::type_index(typeid(float)), Type(Type::Float), "float");
  register_type(std::type_index(typeid(double)), Type(Type::Double), "double");
  register_type(std::type_index(typeid(String)), Type(Type::String), "String");

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
 * \fn void tearDown()
 * \brief destroys the engine
 *
 * This function destroys the global namespace and all the modules.
 */
void Engine::tearDown()
{
  while (!d->scripts.empty())
    d->destroy(d->scripts.back());

  for (auto m : d->modules)
    m.destroy();
  d->modules.clear();

  d->rootNamespace = Namespace{};

  d->templates.dict.clear();
  d->templates.array = ClassTemplate();
  d->templates.initializer_list = ClassTemplate();

  if(!d->context.isNull())
    d->context.clear();

  d->interpreter.reset();
  d->compiler.reset();

  d->typesystem = nullptr;
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
 * \fn Type getType(const std::string& name)
 * \brief retrieves a type by its name
 * 
 * Note that the type must have been previsouly registered with
 * \m registerType().
 */
Type Engine::getType(const std::string& name) const
{
  const auto& map = typeSystem()->impl()->typemap_by_name;
  auto it = map.find(name);
  return it != map.end() ? it->second : Type();
}

/*!
 * \fn Value newBool(bool bval)
 * \brief Constructs a new value of type bool
 */
Value Engine::newBool(bool bval)
{
  return Value(new CppValue<bool>(this, script::Type::Boolean, bval));
}

/*!
 * \fn Value newChar(char cval)
 * \brief Constructs a new value of type char
 */
Value Engine::newChar(char cval)
{
  return Value(new CppValue<char>(this, script::Type::Char, cval));
}

/*!
 * \fn Value newInt(int ival)
 * \brief Constructs a new value of type int
 */
Value Engine::newInt(int ival)
{
  return Value(new CppValue<int>(this, script::Type::Int, ival));
}

/*!
 * \fn Value newFloat(float fval)
 * \brief Constructs a new value of type float
 */
Value Engine::newFloat(float fval)
{
  return Value(new CppValue<float>(this, script::Type::Float, fval));
}

/*!
 * \fn Value newDouble(double dval)
 * \brief Constructs a new value of type double
 */
Value Engine::newDouble(double dval)
{
  return Value(new CppValue<double>(this, script::Type::Double, dval));
}

/*!
 * \fn Value newString(const String & sval)
 * \brief Constructs a new value of type String
 */
Value Engine::newString(const String & sval)
{
  return Value(new CppValue<String>(this, script::Type::String, sval));
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
    OverloadResolution::Candidate selected = resolve_overloads(ctors, Type(cla.id()), args);

    if (selected.function.isNull())
      throw ConstructionError{EngineError::NoMatchingConstructor };
    else if (selected.function.isDeleted())
      throw ConstructionError{ EngineError::ConstructorIsDeleted };

    Locals arguments;
    arguments.push(Value::Void);

    for (const auto& a : args)
    {
      arguments.push(a);
    }

    return selected.function.call(arguments);
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
  if (val.isReference())
    return;

  auto *impl = val.impl();

  if (impl->type.isObjectType())
  {
    Function dtor = typeSystem()->getClass(val.type()).destructor();
    dtor.invoke({ val });
  }
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
  {
    return copy_fundamental(val, this);
  }
  else if (val.type().isEnumType())
  {
    Enum enm = typeSystem()->getEnum(val.type());
    return enm.impl()->copy.invoke({ val });
  }
  else if (val.type().isObjectType())
  {
    Class cla = typeSystem()->getClass(val.type());
    Function copyCtor = cla.copyConstructor();
    if (copyCtor.isNull() || copyCtor.isDeleted())
      throw CopyError{};

    return copyCtor.invoke({ Value(), val });
  }
  else if (val.type().isFunctionType())
  {
    return Value::fromFunction(val.toFunction(), val.type().baseType());
  }
  else if (val.type().isClosureType())
  {
    return Value::fromLambda(copy_lambda(val.toLambda(), this));
  }

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
  Script ret{ std::make_shared<ScriptImpl>(static_cast<int>(d->scripts.size()), this, source) };
  d->scripts.push_back(ret);
  return ret;
}

/*!
 * \fn bool compile(Script s, CompileMode mode)
 * \param input script
 * \param compilation mode
 * \brief Compiles a script.
 *
 */
bool Engine::compile(Script s, CompileMode mode)
{
  return compiler()->compile(s, mode);
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
  auto mimpl = std::make_shared<ScriptModule>(static_cast<int>(d->scripts.size()), this, src, name);
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
 * \fn std::string toString(const Type& t) const
 * \param type
 * \brief Computes a string representation of the given type.
 */
std::string Engine::toString(const Type& t) const
{
  std::string ret = typeSystem()->typeName(t);

  if (t.isConst())
    ret = std::string{ "const " } + ret;
  if (t.isReference())
    ret = ret + "&";
  else if (t.isRefRef())
    ret = ret + "&&";

  return ret;
}

/*!
 * \fn std::string toString(const Function& f) const
 * \param function
 * \brief Computes a string representation of the function prototype.
 */
std::string Engine::toString(const Function & f) const
{
  std::stringstream ss;

  if (f.isConstructor())
    ss << f.memberOf().name();
  else if (f.isDestructor())
    ss << "~" << f.memberOf().name();
  else if (f.isCast())
  {
    ss << "operator " << toString(f.returnType());
  }
  else if (f.isOperator())
  {
    ss << toString(f.returnType());
    ss << " operator" << Operator::getSymbol(f.toOperator().operatorId());
  }
  else
  {
    ss << toString(f.returnType()) << " " << f.name();
  }

  ss << "(";
  for (int i(0); i < f.prototype().count(); ++i)
  {
    ss << toString(f.parameter(i));
    if (i < f.prototype().count() - 1)
      ss << ", ";
  }
  ss << ")";

  if (f.isDeleted())
    ss << " = delete";
  else if (f.isDefaulted())
    ss << " = default";

  return ss.str();
}


/*!
 * \fn Context newContext()
 * \brief Creates a new context.
 *
 */
Context Engine::newContext()
{
  Context c{ std::make_shared<ContextImpl>(this, static_cast<int>(d->allContexts.size() + 1), "") };
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
    diagnostic::MessageBuilder msb{ this };
    diagnostic::DiagnosticMessage mssg = msb.error(ex);

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

Type Engine::register_type(std::type_index id, Type::TypeFlag what, const std::string& name)
{
  size_t offset = typeSystem()->reserve(what, 1);
  Type result{ static_cast<int>(offset), what };
  register_type(id, result, name);
  return result;
}

void Engine::register_type(std::type_index id, Type t, const std::string& name)
{
  typeSystem()->impl()->typemap[id] = t;
  typeSystem()->impl()->typemap_by_name[name] = t;
}

Type Engine::find_type_or_throw(std::type_index id) const
{
  auto& typemap = typeSystem()->impl()->typemap;
  auto it = typemap.find(id);

  if (it == typemap.end())
    throw UnknownTypeError();

  return it->second;
}

} // namespace script
