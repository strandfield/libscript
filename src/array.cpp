// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/array.h"
#include "script/arraytemplate.h"

#include "script/engine.h"
#include "script/classtemplateinstancebuilder.h"
#include "script/classtemplatenativebackend.h"
#include "script/functionbuilder.h"
#include "script/template.h"
#include "script/templatebuilder.h"
#include "script/typesystem.h"

#include "script/private/array_p.h"
#include "script/private/engine_p.h"
#include "script/private/value_p.h"

#include <algorithm>

namespace script
{

namespace callbacks
{

namespace array
{

// Array<T>();
Value default_ctor(FunctionCall *c)
{
  auto array_data = std::dynamic_pointer_cast<SharedArrayData>(c->callee().memberOf().data());
  auto array_impl = std::make_shared<ArrayImpl>(array_data->data, c->engine());
  c->thisObject() = Value(new ArrayValue(Array(array_impl)));
  return c->thisObject();
}

// Array<T>(const Array<T> & other);
Value copy_ctor(FunctionCall *c)
{
  Array other = c->arg(1).toArray();
  other.detach();
  c->thisObject() = Value(new ArrayValue(other));
  return c->thisObject();
}

// Array<T>(const int & size);
Value size_ctor(FunctionCall *c)
{
  auto array_data = std::dynamic_pointer_cast<SharedArrayData>(c->callee().memberOf().data());

  const int size = c->arg(1).toInt();
  auto array_impl = std::make_shared<ArrayImpl>(array_data->data, c->engine());
  array_impl->resize(size);

  c->thisObject() = Value(new ArrayValue(Array(array_impl)));
  return c->thisObject();
}

// ~Array<T>();
Value dtor(FunctionCall *c)
{
  return script::Value::Void;
}

// int Array<T>::size() const;
Value size(FunctionCall *c)
{
  return c->engine()->newInt(c->arg(0).toArray().size());
}

// void Array<T>::resize(const int & newSize);
Value resize(FunctionCall *c)
{
  Array self = c->arg(0).toArray();
  self.resize(c->arg(1).toInt());
  return Value::Void;
}


// T & Array<T>::operator[](const int & index);
// const T & Array<T>::operator[](const int & index) const;
Value subscript(FunctionCall *c)
{
  Value that = c->arg(0);
  auto array_impl = that.toArray().impl();

  return array_impl->elements[c->arg(1).toInt()];
}

// Array<T> & Array<T>::operator=(const Array<T> & other);
Value assign(FunctionCall *c)
{
  Array self = c->arg(0).toArray();
  Array other = c->arg(1).toArray();
  self.assign(other);
  return c->arg(0);
}

} // namespace array


} // namespace callbacks


Class ArrayTemplate::instantiate(ClassTemplateInstanceBuilder& builder)
{
  const auto& arguments = builder.arguments();

  if (arguments.size() != 1)
    throw TemplateInstantiationError{ TemplateInstantiationError::InvalidArgumentCount };

  if (arguments.at(0).kind != TemplateArgument::TypeArgument)
    throw TemplateInstantiationError{ TemplateInstantiationError::ArgumentMustBeAType };

  const Type element_type = arguments.at(0).type.baseType();

  if (element_type.isEnumType())
    throw TemplateInstantiationError{ TemplateInstantiationError::ArgumentCannotBeAnEnumeration };

  Engine * e = builder.getTemplate().engine();
  ArrayData data;
  data.elementType = element_type;

  if (element_type.isObjectType())
  {
    Class element_class = e->typeSystem()->getClass(element_type);
    data.constructor = element_class.defaultConstructor();
    data.copyConstructor = element_class.copyConstructor();
    data.destructor = element_class.destructor();

    if (data.constructor.isNull())
      throw TemplateInstantiationError{ TemplateInstantiationError::TypeMustBeDefaultConstructible };
    if (data.copyConstructor.isNull())
      throw TemplateInstantiationError{ TemplateInstantiationError::TypeMustBeCopyConstructible };
    if (data.destructor.isNull())
      throw TemplateInstantiationError{ TemplateInstantiationError::TypeMustBeDestructible };
  }


  builder.name = std::string("Array<") + e->typeSystem()->typeName(element_type) + std::string(">");

  auto shared_data = std::make_shared<SharedArrayData>(data);
  builder.setData(shared_data);

  Class array_class = builder.get();
  shared_data->data.typeId = array_class.id();
  Type array_type = array_class.id();

  FunctionBuilder::Constructor(array_class).setCallback(callbacks::array::default_ctor).create();

  FunctionBuilder::Constructor(array_class).setCallback(callbacks::array::copy_ctor).params(Type::cref(array_type)).create();

  FunctionBuilder::Constructor(array_class).setCallback(callbacks::array::size_ctor).setExplicit().params(Type::cref(Type::Int)).create();

  FunctionBuilder::Destructor(array_class).setCallback(callbacks::array::dtor).create();

  FunctionBuilder::Fun(array_class, "size").setCallback(callbacks::array::size)
    .setConst().returns(Type::Int).create();

  FunctionBuilder::Fun(array_class, "resize").setCallback(callbacks::array::resize)
    .params(Type::cref(Type::Int)).create();

  FunctionBuilder::Op(array_class, AssignmentOperator).setCallback(callbacks::array::assign)
    .returns(Type::ref(array_type))
    .params(Type::cref(array_type)).create();

  FunctionBuilder::Op(array_class, SubscriptOperator).setCallback(callbacks::array::subscript)
    .returns(Type::ref(element_type))
    .params(Type::cref(Type::Int)).create();

  FunctionBuilder::Op(array_class, SubscriptOperator).setCallback(callbacks::array::subscript)
    .setConst()
    .returns(Type::cref(element_type))
    .params(Type::cref(Type::Int)).create();

  return array_class;
}

ClassTemplate ArrayImpl::register_array_template(Engine *e)
{
  Namespace root = e->rootNamespace();

  std::vector<TemplateParameter> params{
    TemplateParameter{ TemplateParameter::TypeParameter{}, "T" },
  };

  ClassTemplate array_template = ClassTemplateBuilder(Symbol(root), "Array")
    .setParams(std::move(params))
    .setScope(Scope{ root })
    .withBackend<ArrayTemplate>()
    .get();

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
  
  ret->assign(*this);

  return ret;
}

void ArrayImpl::destroy()
{
  for (int i(0); i < this->size; ++i)
    this->engine->destroy(this->elements[i]);

  delete[] this->elements;
  this->elements = nullptr;
  this->size = 0;
}

void ArrayImpl::allocate(int n)
{
  this->size = n;
  if (n == 0)
    return;
  this->elements = new Value[n];
}

void ArrayImpl::resize(int s)
{
  s = std::max(s, 0);

  destroy();
  allocate(s);
  
  for (int i(0); i < s; ++i)
    this->elements[i] = engine->implementation()->default_construct(this->data.elementType, this->data.constructor);
}

void ArrayImpl::assign(const ArrayImpl & other)
{
  assert(other.data.typeId == this->data.typeId);

  destroy();
  allocate(other.size);
  
  for (int i(0); i < other.size; ++i)
    this->elements[i] = engine->implementation()->copy(other.elements[i], this->data.copyConstructor);
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

void Array::resize(int newsize)
{
  d->resize(newsize);
}

void Array::assign(const Array & other)
{
  if (other.impl() == d)
    return;

  d->assign(*other.impl());
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
