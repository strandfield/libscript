// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/typesystem.h"
#include "script/private/typesystem_p.h"

#include "script/engine.h"
#include "script/conversions.h"
#include "script/namelookup.h"
#include "script/scope.h"

#include "script/private/class_p.h"
#include "script/private/engine_p.h"
#include "script/private/enum_p.h"
#include "script/private/lambda_p.h"
#include "script/private/operator_p.h"
#include "script/private/value_p.h"

namespace script
{

template<typename T>
void squeeze(std::vector<T>& list)
{
  while (!list.empty() && list.back().isNull())
    list.pop_back();
}

namespace callbacks
{

Value function_variable_assignment(interpreter::FunctionCall* c)
{
  static_cast<FunctionValue*>(c->arg(0).impl())->function = c->arg(1).toFunction();
  return c->arg(0);
}

} // namespace callbacks

TypeSystemImpl::TypeSystemImpl(Engine *e)
  : engine(e)
{

}

TypeSystemImpl::~TypeSystemImpl()
{

}

std::unique_ptr<TypeSystemImpl> TypeSystemImpl::create(Engine* e)
{
  return std::unique_ptr<TypeSystemImpl>(new TypeSystemImpl(e));
}

template<typename T>
void reserve_range(std::vector<T>& list, const T& value, size_t begin, size_t end)
{
  if (list.size() < end)
    list.resize(end);

  for (size_t i(begin); i < end; ++i)
  {
    if (!list.at(i).isNull() && list.at(i) != value)
      continue; /// TODO: should we do something ?
    list[i] = value;
  }
}

void TypeSystemImpl::reserveTypes(int begin, int end)
{
  const Type begin_type{ begin };
  const Type end_type{ end };

  const int index_mask = Type::EnumFlag - 1;

  begin = begin & index_mask;
  end = end & index_mask;

  if (begin_type.isEnumType())
    reserve_range(this->enums, this->reservations.enum_type, begin, end);
  else if (begin_type.isObjectType())
    reserve_range(this->classes, this->reservations.class_type, begin, end);
}

void TypeSystemImpl::reserveTypes()
{
  reserveTypes(Type::FirstClassType, Type::LastClassType);
  reserveTypes(Type::FirstEnumType, Type::LastEnumType);
}

ClosureType TypeSystemImpl::newLambda()
{
  const int id = static_cast<int>(this->lambdas.size()) | Type::LambdaFlag;
  // const int index = id & 0xFFFF;
  ClosureType l{ std::make_shared<ClosureTypeImpl>(id, this->engine) };
  this->lambdas.push_back(l);
  notify_creation(l.id());
  return l;
}

void TypeSystemImpl::register_class(Class & c, int id)
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

  notify_creation(c.id());
}

void TypeSystemImpl::register_enum(Enum & e, int id)
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

  notify_creation(e.id());
}

void TypeSystemImpl::destroy(const Type& t)
{
  if (t.isEnumType())
  {
    destroy(this->enums[t.data() & 0xFFFF]);
  }
  else if (t.isObjectType())
  {
    destroy(this->classes[t.data() & 0xFFFF]);
  }
  else if (t.isClosureType())
  {
    unregister_closure(this->lambdas[t.data() & 0xFFFF]);
  }
  else if (t.isFunctionType())
  {
    unregister_function(this->prototypes[t.data() & 0xFFFF]);
  }
  else
  {
    throw std::runtime_error{ "Not implemented" };
  }
}

void TypeSystemImpl::destroy(Enum e)
{
  e.impl()->name.insert(0, "deleted_");
  unregister_enum(e);
}

void TypeSystemImpl::destroy(Class c)
{
  for (const auto & v : c.staticDataMembers())
  {
    engine->destroy(v.second.value);
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

void TypeSystemImpl::unregister_class(Class &c)
{
  const int index = c.id() & 0xFFFF;
  this->classes[index] = Class();
  squeeze(this->classes);
  notify_destruction(c.id());
}

void TypeSystemImpl::unregister_enum(Enum &e)
{
  const int index = e.id() & 0xFFFF;
  this->enums[index] = Enum();
  squeeze(this->enums);
  notify_destruction(e.id());
}

void TypeSystemImpl::unregister_closure(ClosureType &c)
{
  const int index = c.id() & 0xFFFF;
  this->lambdas[index] = ClosureType();
  squeeze(this->lambdas);
  notify_destruction(c.id());
}

void TypeSystemImpl::unregister_function(FunctionType& ft)
{
  const int index = ft.type().data() & 0xFFFF;
  this->prototypes[index] = FunctionType();
  squeeze(this->prototypes);
  notify_destruction(ft.type());
}

void TypeSystemImpl::notify_creation(const Type& t)
{
  for (const auto& l : listeners)
  {
    l->created(t);
  }
}

void TypeSystemImpl::notify_destruction(const Type& t)
{
  for (const auto& l : listeners)
  {
    l->destroyed(t);
  }
}


/*!
 * \class TypeSystem
 * \brief Represents the type system
 */

TypeSystem::~TypeSystem()
{

}

TypeSystem::TypeSystem(std::unique_ptr<TypeSystemImpl>&& impl)
  : d(std::move(impl))
{
  d->reservations.class_type = Class{ std::make_shared<ClassImpl>(0, "__reserved_class__", nullptr) };
  d->reservations.enum_type = Enum{ std::make_shared<EnumImpl>(0, "__reserved_enum__", nullptr) };

  d->reserveTypes();
}

/*!
 * \fn Engine* engine() const
 * \brief Returns the engine that owns this type-system
 */
Engine* TypeSystem::engine() const
{
  return d->engine;
}

/*!
 * \fn bool exists(const Type & t) const
 * \param input type
 * \brief Returns whether a given type exists.
 *
 */
bool TypeSystem::exists(const Type& t) const
{
  if (t.isFundamentalType())
    return true;

  const size_t index = t.data() & 0xFFFF;
  if (t.isObjectType())
    return d->classes.size() > index && !d->classes.at(index).isNull();
  else if (t.isEnumType())
    return d->enums.size() > index && !d->enums.at(index).isNull();
  else if (t.isClosureType())
    return d->lambdas.size() > index&& d->lambdas.at(index).impl() != nullptr;
  else if (t.isFunctionType())
    return d->prototypes.size() > index && !d->prototypes.at(index).assignment().isNull();

  return false;
}

/*!
 * \fn Class getClass(Type id) const
 * \param input type
 * \brief Returns the Class for the given type.
 */
Class TypeSystem::getClass(Type id) const
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
Enum TypeSystem::getEnum(Type id) const
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
ClosureType TypeSystem::getLambda(Type id) const
{
  if (!id.isClosureType())
    return ClosureType{};

  int index = id.data() & 0xFFFF;
  return d->lambdas[index];
}

/*!
 * \fn FunctionType getFunctionType(Type id) const
 * \param function type
 * \brief Returns type info for the given type id.
 *
 */
FunctionType TypeSystem::getFunctionType(Type id) const
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
FunctionType TypeSystem::getFunctionType(const Prototype& proto)
{
  for (const auto& ft : d->prototypes)
  {
    if (ft.prototype() == proto)
      return ft;
  }

  /* Create new function type */

  const int id = d->prototypes.size();
  Type type{ id | Type::PrototypeFlag };

  BinaryOperatorPrototype assign_proto{ Type::ref(type), Type::ref(type), Type::cref(type) };
  auto assign_op = std::make_shared<BinaryOperatorImpl>(AssignmentOperator, assign_proto, engine(), FunctionFlags{});
  assign_op->set_impl(callbacks::function_variable_assignment);

  FunctionType ret{ type, proto, Operator{ assign_op } };
  d->prototypes.push_back(ret);

  d->notify_creation(ret.type());

  return ret;
}

/*!
 * \fn bool isDefaultConstructible(const Type & t) const
 * \param input type
 * \brief Returns whether a type is default constructible.
 */
bool TypeSystem::isDefaultConstructible(const Type& t) const
{
  if (t.isFundamentalType())
    return true;
  else if (t.isEnumType())
    return false;
  else if (t.isObjectType())
  {
    Class cla = getClass(t);
    const Function& ctor = cla.defaultConstructor();
    return !ctor.isNull() && !ctor.isDeleted();
  }
  else if (t.isFunctionType())
    return false;
  else if (t.isClosureType())
    return false;

  return false;
}

/*!
 * \fn bool isCopyConstructible(const Type & t) const
 * \param input type
 * \brief Returns whether a given type can be copied.
 *
 * Note that the \c const and \c{&} qualifiers of \c t are ignored.
 */
bool TypeSystem::isCopyConstructible(const Type& t) const
{
  if (t.isFundamentalType())
    return true;
  else if (t.isEnumType())
    return true;
  else if (t.isObjectType())
  {
    Class cla = getClass(t);
    return cla.isCopyConstructible();
  }
  else if (t.isFunctionType())
    return true;
  else if (t.isClosureType())
    return true;

  return false;
}

/*!
 * \fn bool isMoveConstructible(const Type & t) const
 * \param input type
 * \brief Returns whether a given type can be move-constructed.
 *
 */
bool TypeSystem::isMoveConstructible(const Type& t) const
{
  if (t.isFundamentalType())
    return true;
  else if (t.isEnumType())
    return true;
  else if (t.isObjectType())
  {
    Class cla = getClass(t);
    return cla.isMoveConstructible();
  }
  else if (t.isFunctionType())
    return true;
  else if (t.isClosureType())
    return true;

  return false;
}

/*!
 * \fn Conversion conversion(const Type& src, const Type& dest) const
 * \param source type
 * \param destination type
 */
Conversion TypeSystem::conversion(const Type& src, const Type& dest) const
{
  return Conversion::compute(src, dest, engine());
}

/*!
 * \fn bool canConvert(const Type& srcType, const Type& destType) const
 * \param source type
 * \param destination type
 * \brief Returns whether conversion from a type to another is possible.
 * 
 * This function computes the actual Conversion with \m conversion() and checks 
 * that its rank is not \c{ConversionRank::NotConvertible}.
 */
bool TypeSystem::canConvert(const Type& srcType, const Type& destType) const
{
  return conversion(srcType, destType).rank() != ConversionRank::NotConvertible;
}

/*!
 * \fn Type typeId(const std::string & typeName, const Scope & scope) const
 * \param type name
 * \param scope
 * \brief Searchs for a type by name.
 *
 * Throws \t UnknownTypeError if the type could not be resolved.
 */
Type TypeSystem::typeId(const std::string& typeName) const
{
  return typeId(typeName, Scope{ engine()->rootNamespace() });
}

Type TypeSystem::typeId(const std::string& typeName, const Scope& scope) const
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

  if(scope.isNull())
    throw UnknownTypeError{};

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
std::string TypeSystem::typeName(Type t) const
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
 * \fn bool isInitializerList(const Type & t) const
 * \param input type
 * \brief Returns whether a type is an initializer list type.
 *
 */
bool TypeSystem::isInitializerList(const Type& t) const
{
  if (!t.isObjectType())
    return false;

  return getClass(t).isTemplateInstance() 
    && getClass(t).instanceOf() == engine()->implementation()->templates.initializer_list;
}

/*!
 * \fn size_t reserve(Type::TypeFlag flag, size_t count)
 * \param flag indicating what kind of types are to be reserved
 * \param number of type ids to reserve
 * \brief reserves type ids
 * \returns the offset of the first reserved id
 *
 * Note that for now, only class and enum type id can be reserved.
 */
size_t TypeSystem::reserve(Type::TypeFlag flag, size_t count)
{
  if (flag == Type::ObjectFlag)
  {
    size_t off = d->classes.size();
    d->classes.resize(off + count);
    reserve_range(d->classes, d->reservations.class_type, off, off + count);
    return off;
  }
  else if (flag == Type::EnumFlag)
  {
    size_t off = d->enums.size();
    d->enums.resize(off + count);
    reserve_range(d->enums, d->reservations.enum_type, off, off + count);
    return off;
  }
  else
  {
    // @TODO: throw ?
    return std::numeric_limits<size_t>::max();
  }
}

/*!
 * \fn void addListener(TypeSystemListener* listener)
 * \param listener
 * \brief Adds a listener to the typesystem.
 *
 * The TypeSystem takes ownership of the listener.
 */
void TypeSystem::addListener(TypeSystemListener* listener)
{
  d->listeners.push_back(std::unique_ptr<TypeSystemListener>(listener));
  listener->m_typesystem = this;
}

/*!
 * \fn void removeListener(TypeSystemListener* listener)
 * \param listener
 * \brief Removes a listener.
 *
 * Ownership is transferred back to the caller of this function.
 */
void TypeSystem::removeListener(TypeSystemListener* listener)
{
  for (auto it = d->listeners.begin(); it != d->listeners.end(); ++it)
  {
    if (it->get() == listener)
    {
      it->release();
      listener->m_typesystem = nullptr;
      d->listeners.erase(it);
      return;
    }
  }
}

bool TypeSystem::hasActiveTransaction() const
{
  return d->active_transaction != nullptr;
}

TypeSystemImpl* TypeSystem::impl() const
{
  return d.get();
}


} // namespace script
