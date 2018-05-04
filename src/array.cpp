// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/array.h"

#include "array_p.h"
#include "script/engine.h"
#include "engine_p.h"
#include "script/functionbuilder.h"
#include "script/template.h"
#include "value_p.h"


namespace script
{

namespace callbacks
{

namespace array
{

// Array<T>();
Value default_ctor(FunctionCall *c)
{
  Value that = c->thisObject();
  auto array_data = std::dynamic_pointer_cast<SharedArrayData>(c->engine()->getClass(that.type()).data());
  auto array_impl = std::make_shared<ArrayImpl>(array_data->data, c->engine());
  that.impl()->setArray(Array{ array_impl });
  return that;
}

// Array<T>(const Array<T> & other);
Value copy_ctor(FunctionCall *c)
{
  Value that = c->thisObject();
  Array other = c->arg(0).toArray();
  other.detach();
  that.impl()->setArray(other);
  return that;
}

// Array<T>(const int & size);
Value size_ctor(FunctionCall *c)
{
  Value that = c->thisObject();
  auto array_data = std::dynamic_pointer_cast<SharedArrayData>(c->engine()->getClass(that.type()).data());

  const int size = c->arg(0).toInt() > 0 ? c->arg(0).toInt() : 0;
  auto array_impl = std::make_shared<ArrayImpl>(array_data->data, c->engine());

  if (size > 0)
  {
    array_impl->size = size;
    array_impl->elements = new Value[size];

    auto engine = c->engine()->implementation();

    for (int i(0); i < size; ++i)
      array_impl->elements[i] = engine->default_construct(array_data->data.elementType, array_data->data.constructor);
  }

  that.impl()->setArray(Array{ array_impl });
  return that;
}

// ~Array<T>();
Value dtor(FunctionCall *c)
{
  Value that = c->thisObject();
  that.impl()->clear();
  return that;
}

// int Array<T>::size() const;
Value size(FunctionCall *c)
{
  return c->engine()->newInt(c->thisObject().toArray().size());
}

// void Array<T>::resize(const int & newSize);
Value resize(FunctionCall *c)
{
  Value that = c->thisObject();
  auto array_data = std::dynamic_pointer_cast<SharedArrayData>(c->engine()->getClass(that.type()).data());

  const int new_size = c->arg(1).toInt() > 0 ? c->arg(1).toInt() : 0;
  auto array_impl = that.toArray().impl();

  auto engine = c->engine()->implementation();

  for (int i(0); i < array_impl->size; ++i)
    engine->destroy(array_impl->elements[i], array_data->data.destructor);

  delete[] array_impl->elements;
  array_impl->elements = nullptr;

  if (new_size > 0)
  {
    array_impl->size = new_size;
    array_impl->elements = new Value[new_size];
    for (int i(0); i < new_size; ++i)
      array_impl->elements[i] = engine->default_construct(array_data->data.elementType, array_data->data.constructor);
  }
  else
  {
    array_impl->size = 0;
  }

  return Value::Void;
}


// T & Array<T>::operator[](const int & index);
// const T & Array<T>::operator[](const int & index) const;
Value subscript(FunctionCall *c)
{
  Value that = c->thisObject();
  auto array_impl = that.toArray().impl();

  return array_impl->elements[c->arg(1).toInt()];
}

// Array<T> & Array<T>::operator=(const Array<T> & other);
Value assign(FunctionCall *c)
{
  Value that = c->thisObject();
  auto array_data = std::dynamic_pointer_cast<SharedArrayData>(c->engine()->getClass(that.type()).data());
  
  Array other = c->arg(1).toArray();

  auto array_impl = that.toArray().impl();
  auto engine = c->engine()->implementation();

  for (int i(0); i < array_impl->size; ++i)
    engine->destroy(array_impl->elements[i], array_data->data.destructor);

  delete[] array_impl->elements;
  array_impl->elements = nullptr;

  if (other.size() > 0)
  {
    array_impl->size = other.size();
    array_impl->elements = new Value[array_impl->size];
    for (int i(0); i < array_impl->size; ++i)
      array_impl->elements[i] = engine->copy(other.at(i), array_data->data.copyConstructor);
  }
  else
  {
    array_impl->size = 0;
  }

  return that;
}

} // namespace array


} // namespace callbacks


Class instantiate_array_class(ClassTemplate tplt, const std::vector<TemplateArgument> & arguments)
{
  if (arguments.size() != 1)
    throw TemplateInstantiationError{"Invalid argument count"};

  if (arguments.at(0).kind != TemplateArgument::TypeArgument)
    throw TemplateInstantiationError{ "Argument must be a type" };

  const Type element_type = arguments.at(0).type.baseType();

  if(element_type.isEnumType())
    throw TemplateInstantiationError{ "Argument cannot be an enumeration" };

  Engine *e = tplt.engine();
  ArrayData data;
  data.elementType = element_type;

  if (element_type.isObjectType())
  {
    Class element_class = e->getClass(element_type);
    data.constructor = element_class.defaultConstructor();
    data.copyConstructor = element_class.copyConstructor();
    data.destructor = element_class.destructor();

    if (data.constructor.isNull())
      throw TemplateInstantiationError{ "Type must be default-cosntructible" };
    if (data.copyConstructor.isNull())
      throw TemplateInstantiationError{ "Type must be copy-cosntructible" };
    if (data.destructor.isNull())
      throw TemplateInstantiationError{ "Type must be destructible" };
  }
  

  ClassBuilder class_builder = ClassBuilder::New(std::string("Array<") + e->typeName(element_type) + std::string(">"));
  auto shared_data = std::make_shared<SharedArrayData>(data);
  class_builder.setData(shared_data);
  Class array_class = tplt.build(class_builder, arguments);
  shared_data->data.typeId = array_class.id();

  FunctionBuilder function_builder = FunctionBuilder::Constructor(array_class, Prototype{}, callbacks::array::default_ctor);
  array_class.newConstructor(function_builder);

  function_builder = FunctionBuilder::Constructor(array_class, Prototype{Type::cref(array_class.id()), Type::cref(array_class.id()) }, callbacks::array::copy_ctor);
  array_class.newConstructor(function_builder);

  function_builder = FunctionBuilder::Constructor(array_class, Prototype{ Type::cref(array_class.id()), Type::cref(Type::Int) }, callbacks::array::size_ctor);
  array_class.newConstructor(function_builder);

  array_class.newDestructor(callbacks::array::dtor);

  function_builder = FunctionBuilder::Function("size", Prototype{ Type::Int, Type::cref(array_class.id() | Type::ThisFlag) }, callbacks::array::size);
  array_class.newMethod(function_builder);

  function_builder = FunctionBuilder::Function("resize", Prototype{ Type::Void, Type::ref(array_class.id() | Type::ThisFlag), Type::cref(Type::Int) }, callbacks::array::resize);
  array_class.newMethod(function_builder);

  function_builder = FunctionBuilder::Operator(Operator::AssignmentOperator, Type::ref(array_class.id()), Type::ref(array_class.id() | Type::ThisFlag), Type::cref(array_class.id()), callbacks::array::assign);
  array_class.newOperator(function_builder);

  function_builder = FunctionBuilder::Operator(Operator::SubscriptOperator, Type::ref(element_type), Type::ref(array_class.id() | Type::ThisFlag), Type::cref(Type::Int), callbacks::array::subscript);
  array_class.newOperator(function_builder);
  function_builder = FunctionBuilder::Operator(Operator::SubscriptOperator, Type::cref(element_type), Type::cref(array_class.id() | Type::ThisFlag), Type::cref(Type::Int), callbacks::array::subscript);
  array_class.newOperator(function_builder);

  return array_class;
}

ClassTemplate ArrayImpl::register_array_template(Engine *e)
{
  Namespace root = e->rootNamespace();

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
  };

  ClassTemplate array_template = e->newClassTemplate("Array", std::move(params), Scope{ root }, instantiate_array_class);
  root.addTemplate(array_template);
  return array_template;
}


SharedArrayData::SharedArrayData(const ArrayData & d)
  : data(d)
{

}

ArrayImpl::ArrayImpl()
  : size(0)
  , engine(nullptr)
  , elements(nullptr)
{

}

ArrayImpl::ArrayImpl(const ArrayData & d, Engine *e)
  : data(d)
  , size(0)
  , engine(e)
  , elements(nullptr)
{

}

ArrayImpl::~ArrayImpl()
{
  delete[] elements;
}

ArrayImpl * ArrayImpl::copy() const
{
  if (engine == nullptr)
    return new ArrayImpl{};

  auto ret = new ArrayImpl{ this->data, this->engine };
  if (this->size == 0)
    return ret;

  ret->size = this->size;
  ret->elements = new Value[ret->size];

  for (int i(0); i < this->size; ++i)
  {
    ret->elements[i] = this->engine->implementation()->copy(this->elements[i], this->data.copyConstructor);
  }

  return ret;
}

void ArrayImpl::resize(int s)
{
  for (int i(0); i < this->size; ++i)
    this->engine->destroy(this->elements[i]);

  delete[] this->elements;

  this->size = s;
  this->elements = new Value[s];
}


Array::Array()
  : d(nullptr)
{

}

Array::~Array()
{
  d = nullptr;
}

Array::Array(const std::shared_ptr<ArrayImpl> & impl)
  : d(impl)
{

}

Engine* Array::engine() const
{
  return d->engine;
}

Type Array::typeId() const
{
  return d->data.typeId;
}

Type Array::elementTypeId() const
{
  return d->data.elementType;
}

int Array::size() const
{
  return d->size;
}

const Value & Array::at(int index) const
{
  return d->elements[index];
}

Value & Array::operator[](int index)
{
  return d->elements[index];
}

void Array::detach()
{
  if (d == nullptr || d.use_count() == 1)
    return;

  d = std::shared_ptr<ArrayImpl>(d->copy());
}

} // namespace script
