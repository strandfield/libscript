// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "string_p.h"

#include "script/engine.h"
#include "engine_p.h"
#include "script/class.h"
#include "script/function.h"
#include "script/functionbuilder.h"

#include "script/interpreter/executioncontext.h"
#include "value_p.h"

namespace script
{

static int charref_id = 0;

struct charref_t
{
  String *string;
  size_t pos;
};

namespace callbacks
{

namespace string
{

// String();
Value default_ctor(FunctionCall *c)
{
  Value that = c->thisObject();
  that.impl()->setString(String{});
  return that;
}

// String(const String & other);
Value copy_ctor(FunctionCall *c)
{
  Value that = c->thisObject();
  that.impl()->setString(c->arg(0).toString());
  return that;
}

// String(char c);
Value char_ctor(FunctionCall *c)
{
  Value that = c->thisObject();
  that.impl()->setString(String{ c->arg(0).toChar() });
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
  const auto & self = that.impl()->getString();

  const int position = c->arg(1).toInt();

  return c->engine()->newChar(self.at(position));
}

// int String::capacity() const;
Value capacity(FunctionCall *c)
{
  Value that = c->thisObject();
  return c->engine()->newInt(that.impl()->getString().capacity());
}

// void String::clear();
Value clear(FunctionCall *c)
{
  Value that = c->thisObject();
  that.impl()->getString().clear();
  return Value::Void;
}

// bool empty() const;
Value empty(FunctionCall *c)
{
  Value that = c->thisObject();
  return c->engine()->newBool(that.impl()->getString().empty());
}

// String & erase(int position, int n);
Value erase(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->getString();

  const int position = c->arg(1).toInt();
  const int n = c->arg(2).toInt();

  self.erase(position, n);

  return that;
}

// String & String::insert(int position, const String & str);
Value insert(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->getString();

  const int position = c->arg(1).toInt();
  const String & str = c->arg(2).impl()->getString();

  self.insert(position, str);

  return that;
}

// int String::length() const;
// int String::size() const;
Value length(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->getString();
  return c->engine()->newInt(self.size());
}

// String & String::replace(int position, int n, const String & after);
Value replace(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->getString();

  const int pos = c->arg(1).toInt();
  const int count = c->arg(2).toInt();
  const String & str = c->arg(3).impl()->getString();

  self.replace(pos, count, str);

  return that;
}

// void swap(String & other);
Value swap(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->getString();

  auto & other = c->arg(1).impl()->getString();

  self.swap(other);

  return Value::Void;
}

namespace operators
{

// bool String::operator==(const String & other) const;
Value eq(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->getString();

  const auto & other = c->arg(1).impl()->getString();

  return c->engine()->newBool(self == other);
}

// bool String::operator!=(const String & other) const;
Value neq(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->getString();

  const auto & other = c->arg(1).impl()->getString();

  return c->engine()->newBool(self != other);
}

// bool String::operator>(const String & other) const;
Value greater(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->getString();

  const auto & other = c->arg(1).impl()->getString();

  return c->engine()->newBool(self > other);
}

// bool String::operator>=(const String & other) const;
Value geq(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->getString();

  const auto & other = c->arg(1).impl()->getString();

  return c->engine()->newBool(self >= other);
}

// bool String::operator<(const String & other) const;
Value less(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->getString();

  const auto & other = c->arg(1).impl()->getString();

  return c->engine()->newBool(self < other);
}

// bool String::operator<=(const String & other) const;
Value leq(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->getString();

  const auto & other = c->arg(1).impl()->getString();

  return c->engine()->newBool(self <= other);
}

// String & String::operator=(const String & other);
Value assign(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->getString();

  const auto & other = c->arg(1).impl()->getString();

  self = other;

  return that;
}

// String String::operator+(const String & other) const;
Value add(FunctionCall *c)
{
  Value that = c->thisObject();
  const auto & self = that.impl()->getString();

  const auto & other = c->arg(1).impl()->getString();

  return c->engine()->newString(self + other);
}

// charref String::operator[](int index);
Value subscript(FunctionCall *c)
{
  Value that = c->thisObject();
  auto & self = that.impl()->getString();

  const int pos = c->arg(1).toInt();

  Value ret = c->engine()->implementation()->buildValue(charref_id);
  ret.impl()->data.builtin.data = new charref_t{ &self, (size_t)pos };
  return ret;
}

} // namespace operators

} // namespace string

namespace charref
{

// charref(String & str, int pos);
Value ctor(FunctionCall *c)
{
  Value that = c->thisObject();

  auto & str = c->arg(0).impl()->getString();
  const int pos = c->arg(1).toInt();

  that.impl()->data.builtin.data = new charref_t{ &str, (size_t)pos };

  return that;
}

// charref(const charref & other);
Value copy_ctor(FunctionCall *c)
{
  Value that = c->thisObject();

  charref_t *other = static_cast<charref_t*>(c->arg(0).impl()->data.builtin.data);

  that.impl()->data.builtin.data = new charref_t{ other->string, other->pos };

  return that;
}

// ~charref();
Value dtor(FunctionCall *c)
{
  Value that = c->thisObject();

  charref_t *content = static_cast<charref_t*>(that.impl()->data.builtin.data);
  delete content;
  that.impl()->data.builtin.data = nullptr;

  return that;
}

// operator const char();
Value operator_char(FunctionCall *c)
{
  Value that = c->thisObject();
  charref_t *self = static_cast<charref_t*>(that.impl()->data.builtin.data);
  return c->engine()->newChar(self->string->at(self->pos));
}

// charref & operator=(char c);
Value assign(FunctionCall *c)
{
  Value that = c->thisObject();
  charref_t *self = static_cast<charref_t*>(that.impl()->data.builtin.data);

  char character = c->arg(1).toChar();
  auto & str = *self->string;
  str[self->pos] = character;

  return that;
}

} // namespace charref

} // namespace callbacks

std::string get_string_typename()
{
  return "String";
}

Type register_charref_type(Engine *e)
{
  ClassBuilder opts = ClassBuilder::New("charref");
  Class charref = e->newClass(opts);

  charref_id = charref.id();

  FunctionBuilder fb = FunctionBuilder::Constructor(charref, callbacks::charref::ctor)
    .addParam(Type::ref(Type::String)).addParam(Type::Int);
  charref.newConstructor(fb);

  fb = FunctionBuilder::Constructor(charref, callbacks::charref::copy_ctor)
    .addParam(Type::cref(charref.id()));
  charref.newConstructor(fb);

  charref.newDestructor(callbacks::charref::dtor);

  fb = FunctionBuilder::Operator(Operator::AssignmentOperator, Type::ref(charref.id()), Type::ref(charref.id() | Type::ThisFlag), Type::Char, callbacks::charref::assign);
  charref.newOperator(fb);

  fb = FunctionBuilder::Cast(Type::cref(charref.id()), Type{ Type::Char, Type::ConstFlag }, callbacks::charref::operator_char);
  charref.newCast(fb);

  return charref.id();
}

void register_string_type(Class string)
{
  FunctionBuilder fb = FunctionBuilder::Constructor(string, callbacks::string::default_ctor);
  string.newConstructor(fb);

  fb = FunctionBuilder::Constructor(string, callbacks::string::copy_ctor)
    .addParam(Type::cref(string.id()));
  string.newConstructor(fb);

  fb = FunctionBuilder::Constructor(string, callbacks::string::char_ctor)
    .addParam(Type::Char);
  string.newConstructor(fb);

  string.newDestructor(callbacks::string::dtor);

  fb = FunctionBuilder::Method(string, "at", callbacks::string::at)
    .setConst()
    .setReturnType(Type::Char)
    .addParam(Type::Int);
  string.newMethod(fb);

  fb = FunctionBuilder::Method(string, "capacity", callbacks::string::capacity)
    .setConst()
    .setReturnType(Type::Int);
  string.newMethod(fb);

  fb = FunctionBuilder::Method(string, "clear", callbacks::string::clear);
  string.newMethod(fb);

  fb = FunctionBuilder::Method(string, "empty", callbacks::string::empty)
    .setConst()
    .setReturnType(Type::Boolean);
  string.newMethod(fb);

  fb = FunctionBuilder::Method(string, "erase", callbacks::string::erase)
    .setReturnType(Type::ref(string.id()))
    .addParam(Type::Int).addParam(Type::Int);
  string.newMethod(fb);

  fb = FunctionBuilder::Method(string, "insert", callbacks::string::insert)
    .setReturnType(Type::ref(string.id()))
    .addParam(Type::Int).addParam(Type::cref(string.id()));
  string.newMethod(fb);

  fb = FunctionBuilder::Method(string, "length", callbacks::string::length)
    .setConst()
    .setReturnType(Type::Int);
  string.newMethod(fb);

  fb = FunctionBuilder::Method(string, "size", callbacks::string::length)
    .setConst()
    .setReturnType(Type::Int);
  string.newMethod(fb);

  fb = FunctionBuilder::Method(string, "insert", callbacks::string::insert)
    .setReturnType(Type::ref(string.id()))
    .addParam(Type::Int).addParam(Type::Int).addParam(Type::cref(string.id()));
  string.newMethod(fb);
  
  fb = FunctionBuilder::Method(string, "swap", callbacks::string::swap)
    .addParam(Type::ref(string.id()));
  string.newMethod(fb);

  fb = FunctionBuilder::Operator(Operator::EqualOperator, Type::Boolean, Type::cref(string.id() | Type::ThisFlag), Type::cref(string.id()), callbacks::string::operators::eq);
  string.newOperator(fb);
  fb = FunctionBuilder::Operator(Operator::InequalOperator, Type::Boolean, Type::cref(string.id() | Type::ThisFlag), Type::cref(string.id()), callbacks::string::operators::neq);
  string.newOperator(fb);
  fb = FunctionBuilder::Operator(Operator::GreaterOperator, Type::Boolean, Type::cref(string.id() | Type::ThisFlag), Type::cref(string.id()), callbacks::string::operators::greater);
  string.newOperator(fb);
  fb = FunctionBuilder::Operator(Operator::GreaterEqualOperator, Type::Boolean, Type::cref(string.id() | Type::ThisFlag), Type::cref(string.id()), callbacks::string::operators::geq);
  string.newOperator(fb);
  fb = FunctionBuilder::Operator(Operator::LessOperator, Type::Boolean, Type::cref(string.id() | Type::ThisFlag), Type::cref(string.id()), callbacks::string::operators::less);
  string.newOperator(fb);
  fb = FunctionBuilder::Operator(Operator::LessEqualOperator, Type::Boolean, Type::cref(string.id() | Type::ThisFlag), Type::cref(string.id()), callbacks::string::operators::leq);
  string.newOperator(fb);

  fb = FunctionBuilder::Operator(Operator::AssignmentOperator, Type::ref(string.id()), Type::ref(string.id() | Type::ThisFlag), Type::cref(string.id()), callbacks::string::operators::assign);
  string.newOperator(fb);

  fb = FunctionBuilder::Operator(Operator::AdditionOperator, Type(string.id()), Type::cref(string.id() | Type::ThisFlag), Type::cref(string.id()), callbacks::string::operators::add);
  string.newOperator(fb);

  fb = FunctionBuilder::Operator(Operator::SubscriptOperator, Type::Char, Type::cref(string.id() | Type::ThisFlag), Type::Int, callbacks::string::at);
  string.newOperator(fb);

  Type charref = register_charref_type(string.engine());

  fb = FunctionBuilder::Operator(Operator::SubscriptOperator, charref, Type::ref(string.id() | Type::ThisFlag), Type::Int, callbacks::string::operators::subscript);
  string.newOperator(fb);
}

} // namespace script

