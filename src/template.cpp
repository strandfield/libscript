// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/template.h"
#include "template_p.h"

#include "script/classtemplate.h"
#include "script/functiontemplate.h"

#include "class_p.h"
#include "script/engine.h"
#include "engine_p.h"
#include "function_p.h"
#include "script/functionbuilder.h"
#include "script/namelookup.h"
#include "namelookup_p.h"
#include "script/script.h"

#include "script/program/expression.h"

namespace script
{

TemplateImpl::TemplateImpl(const std::string & n, std::vector<TemplateParameter> && params, NativeTemplateDeductionFunction deduc, Engine *e, std::shared_ptr<ScriptImpl> s)
  : name(n)
  , parameters(std::move(params))
  , deduction(deduc)
  , engine(e)
  , script(s)
{

}

FunctionTemplateImpl::FunctionTemplateImpl(const std::string & n, std::vector<TemplateParameter> && params, NativeTemplateDeductionFunction deduc,
  NativeFunctionTemplateSubstitutionCallback sub, NativeFunctionTemplateInstantiationCallback inst,
  Engine *e, std::shared_ptr<ScriptImpl> s)
  : TemplateImpl(n, std::move(params), deduc, e, s)
  , substitute(sub)
  , instantiate(inst)
{

}

FunctionTemplateImpl::~FunctionTemplateImpl()
{

}

ClassTemplateImpl::ClassTemplateImpl(const std::string & n, 
  std::vector<TemplateParameter> && params,
  NativeClassTemplateInstantiationFunction inst,
  Engine *e, std::shared_ptr<ScriptImpl> s)
  : TemplateImpl(n, std::move(params), nullptr, e, s)
  , instantiate(inst)
{

}

ClassTemplateImpl::~ClassTemplateImpl()
{

}

TemplateArgument::TemplateArgument()
  : kind(UnspecifiedArgument), integer(0), boolean(false) { }

TemplateArgument::TemplateArgument(const Type & t)
  : kind(TypeArgument), type(t), integer(0), boolean(false) { }

TemplateArgument::TemplateArgument(int n)
  : kind(IntegerArgument), integer(n), boolean(false) { }

TemplateArgument::TemplateArgument(bool b)
  : kind(BoolArgument), integer(0), boolean(b) { }

TemplateArgument::TemplateArgument(std::vector<TemplateArgument> && args)
  : kind(PackArgument), integer(0), boolean(false)
{
  pack = std::make_shared<TemplateArgumentPack>(std::move(args));
}

inline static int compare(bool a, bool b)
{
  return a ? (b ? 0 : 1) : (b ? -1 : 0);
}

inline static int compare(int a, int b)
{
  return  a < b ? -1 : (a == b ? 0 : 1);
}

int TemplateArgumentComparison::compare(const TemplateArgument & a, const TemplateArgument & b)
{
  if (a.kind == TemplateArgument::BoolArgument)
  {
    if (b.kind != TemplateArgument::BoolArgument)
      return -1;

    return script::compare(a.boolean, b.boolean);
  }
  else if (a.kind == TemplateArgument::IntegerArgument)
  {
    if (b.kind == TemplateArgument::BoolArgument)
      return 1;
    else if (b.kind == TemplateArgument::TypeArgument)
      return -1;

    return script::compare(a.integer, b.integer);
  }

  assert(a.kind == TemplateArgument::TypeArgument);

  if (b.kind != TemplateArgument::TypeArgument)
    return 1;

  return script::compare(a.type.data(), b.type.data());
}

bool TemplateArgumentComparison::operator()(const TemplateArgument & a, const TemplateArgument & b) const
{
  return compare(a, b) < 0;
}

bool TemplateArgumentComparison::operator()(const std::vector<TemplateArgument> & a, const std::vector<TemplateArgument> & b) const
{
  if (a.size() != b.size())
    return a.size() < b.size();

  for (size_t i(0); i < a.size(); ++i)
  {
    const int c = compare(a.at(i), b.at(i));
    if (c != 0)
      return c < 0;
  }

  return false; // a == b
}


bool operator==(const TemplateArgument & lhs, const TemplateArgument & rhs)
{
  return TemplateArgumentComparison::compare(lhs, rhs) == 0;
}




Template::Template(const std::shared_ptr<TemplateImpl> & impl)
  : d(impl)
{

}

bool Template::isNull() const
{
  return d == nullptr;
}

Script Template::script() const
{
  return Script{ d->script.lock() };
}

Engine * Template::engine() const
{
  return d->engine;
}

bool Template::isClassTemplate() const
{
  return dynamic_cast<ClassTemplateImpl*>(d.get()) != nullptr;
}

bool Template::isFunctionTemplate() const
{
  return dynamic_cast<FunctionTemplateImpl*>(d.get()) != nullptr;
}


ClassTemplate Template::asClassTemplate() const
{
  return ClassTemplate{ std::dynamic_pointer_cast<ClassTemplateImpl>(d) };
}

FunctionTemplate Template::asFunctionTemplate() const
{
  return FunctionTemplate{ std::dynamic_pointer_cast<FunctionTemplateImpl>(d) };
}

const std::string & Template::name() const
{
  return d->name;
}

const std::vector<TemplateParameter> & Template::parameters() const
{
  return d->parameters;
}

TemplateArgument Template::get(const std::string & name, const std::vector<TemplateArgument> & args) const
{
  /// TODO : should we throw if the sizes are different ?
  size_t s = std::max(d->parameters.size(), args.size());

  for (size_t i(0); i < s; ++i)
  {
    if (d->parameters.at(i).name() == name)
      return args.at(i);
  }

  throw std::runtime_error{ "Template::get() : no such argument" };
}

bool Template::deduce(std::vector<TemplateArgument> & result, const std::vector<Type> & args)
{
  return d->deduction(*this, result, args);
}

std::weak_ptr<TemplateImpl> Template::weakref() const
{
  return std::weak_ptr<TemplateImpl>(d);
}

bool Template::operator==(const Template & other) const
{
  return d == other.d;
}

} // namespace script
