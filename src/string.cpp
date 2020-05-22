// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/string.h"

#if defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)

#include "script/engine.h"
#include "script/private/engine_p.h"
#include "script/castbuilder.h"
#include "script/class.h"
#include "script/classbuilder.h"
#include "script/constructorbuilder.h"
#include "script/destructorbuilder.h"
#include "script/function.h"
#include "script/functionbuilder.h"
#include "script/operatorbuilder.h"

#include "script/interpreter/executioncontext.h"
#include "script/private/value_p.h"

namespace script
{

namespace callbacks
{

namespace string
{

// String();
Value default_ctor(FunctionCall *c)
{
  Value that = c->thisObject();
  that.impl()->set_string(String{});
  return that;
}

// String(const String & other);
Value copy_ctor(FunctionCall *c)
{
  Value that = c->thisObject();
  that.impl()->set_string(c->arg(1).toString());
  return that;
}

// String(char c);
Value char_ctor(FunctionCall *c)
{
  Value that = c->thisObject();
  that.impl()->set_string(String{ c->arg(1).toChar() });
  return that;
}

// ~String();
Value dtor(FunctionCall *c)
{
  Value that = c->thisObject();
  that.impl()->clear();
  return that;
}

// char String::at(int index) const;
Value at(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->get_string();

  const int position = c->arg(1).toInt();

  return c->engine()->newChar(self.at(position));
}

// int String::capacity() const;
Value capacity(FunctionCall *c)
{
  Value that = c->thisObject();
  return c->engine()->newInt(that.impl()->get_string().capacity());
}

// void String::clear();
Value clear(FunctionCall *c)
{
  Value that = c->thisObject();
  that.impl()->get_string().clear();
  return Value::Void;
}

// bool empty() const;
Value empty(FunctionCall *c)
{
  Value that = c->thisObject();
  return c->engine()->newBool(that.impl()->get_string().empty());
}

// String & erase(int position, int n);
Value erase(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->get_string();

  const int position = c->arg(1).toInt();
  const int n = c->arg(2).toInt();

  self.erase(position, n);

  return that;
}

// String & String::insert(int position, const String & str);
Value insert(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->get_string();

  const int position = c->arg(1).toInt();
  const String & str = c->arg(2).impl()->get_string();

  self.insert(position, str);

  return that;
}

// int String::length() const;
// int String::size() const;
Value length(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->get_string();
  return c->engine()->newInt(self.size());
}

// String & String::replace(int position, int n, const String & after);
Value replace(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->get_string();

  const int pos = c->arg(1).toInt();
  const int count = c->arg(2).toInt();
  const String & str = c->arg(3).impl()->get_string();

  self.replace(pos, count, str);

  return that;
}

// void swap(String & other);
Value swap(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->get_string();

  auto & other = c->arg(1).impl()->get_string();

  self.swap(other);

  return Value::Void;
}

namespace operators
{

// bool String::operator==(const String & other) const;
Value eq(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->get_string();

  const auto & other = c->arg(1).impl()->get_string();

  return c->engine()->newBool(self == other);
}

// bool String::operator!=(const String & other) const;
Value neq(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->get_string();

  const auto & other = c->arg(1).impl()->get_string();

  return c->engine()->newBool(self != other);
}

// bool String::operator>(const String & other) const;
Value greater(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->get_string();

  const auto & other = c->arg(1).impl()->get_string();

  return c->engine()->newBool(self > other);
}

// bool String::operator>=(const String & other) const;
Value geq(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->get_string();

  const auto & other = c->arg(1).impl()->get_string();

  return c->engine()->newBool(self >= other);
}

// bool String::operator<(const String & other) const;
Value less(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->get_string();

  const auto & other = c->arg(1).impl()->get_string();

  return c->engine()->newBool(self < other);
}

// bool String::operator<=(const String & other) const;
Value leq(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->get_string();

  const auto & other = c->arg(1).impl()->get_string();

  return c->engine()->newBool(self <= other);
}

// String & String::operator=(const String & other);
Value assign(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->get_string();

  const auto & other = c->arg(1).impl()->get_string();

  self = other;

  return that;
}

// String String::operator+(const String & other) const;
Value add(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->get_string();

  const auto & other = c->arg(1).impl()->get_string();

  return c->engine()->newString(self + other);
}

// char& String::operator[](int index);
Value subscript(FunctionCall *c)
{
  std::string& str = script::get<std::string>(c->arg(0));
  char& ch = str[script::get<int>(c->arg(1))];
  return c->engine()->expose(ch);
}

} // namespace operators

} // namespace string

} // namespace callbacks

void StringBackend::register_string_type(Class& string)
{
  string.newConstructor(callbacks::string::default_ctor).create();
  string.newConstructor(callbacks::string::copy_ctor).params(Type::cref(string.id())).create();
  string.newConstructor(callbacks::string::char_ctor).setExplicit().params(Type::Char).create();

  string.newDestructor(callbacks::string::dtor).create();

  string.newMethod("at", callbacks::string::at).setConst().returns(Type::Char).params(Type::Int).create();
  string.newMethod("capacity", callbacks::string::capacity).setConst().returns(Type::Int).create();
  string.newMethod("clear", callbacks::string::clear).create();
  string.newMethod("empty", callbacks::string::empty).setConst().returns(Type::Boolean).create();
  string.newMethod("erase", callbacks::string::erase).returns(Type::ref(string.id())).params(Type::Int, Type::Int).create();
  string.newMethod("insert", callbacks::string::insert).returns(Type::ref(string.id())).params(Type::Int, Type::cref(string.id())).create();
  string.newMethod("length", callbacks::string::length).setConst().returns(Type::Int).create();
  string.newMethod("size", callbacks::string::length).setConst().returns(Type::Int).create();
  string.newMethod("replace", callbacks::string::replace).returns(Type::ref(string.id())).params(Type::Int, Type::Int, Type::cref(string.id())).create();
  string.newMethod("swap", callbacks::string::swap).params(Type::ref(string.id())).create();

  string.newOperator(EqualOperator, callbacks::string::operators::eq).setConst().returns(Type::Boolean).params(Type::cref(string.id())).create();
  string.newOperator(InequalOperator, callbacks::string::operators::neq).setConst().returns(Type::Boolean).params(Type::cref(string.id())).create();
  string.newOperator(GreaterOperator, callbacks::string::operators::greater).setConst().returns(Type::Boolean).params(Type::cref(string.id())).create();
  string.newOperator(GreaterEqualOperator, callbacks::string::operators::geq).setConst().returns(Type::Boolean).params(Type::cref(string.id())).create();
  string.newOperator(LessOperator, callbacks::string::operators::less).setConst().returns(Type::Boolean).params(Type::cref(string.id())).create();
  string.newOperator(LessEqualOperator, callbacks::string::operators::leq).setConst().returns(Type::Boolean).params(Type::cref(string.id())).create();

  string.newOperator(AssignmentOperator, callbacks::string::operators::assign).returns(Type::ref(string.id())).params(Type::cref(string.id())).create();

  string.newOperator(AdditionOperator, callbacks::string::operators::add).setConst().returns(string.id()).params(Type::cref(string.id())).create();

  string.newOperator(SubscriptOperator, callbacks::string::at).setConst().returns(Type::Char).params(Type::Int).create();

  string.newOperator(SubscriptOperator, callbacks::string::operators::subscript).returns(Type::ref(Type::Char)).params(Type::Int).create();
}

} // namespace script

#endif // defined(LIBSCRIPT_USE_BUILTIN_STRING_BACKEND)
