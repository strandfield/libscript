// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/template.h"
#include "template_p.h"

#include "script/engine.h"
#include "function_p.h"
#include "script/functionbuilder.h"
#include "script/namelookup.h"
#include "namelookup_p.h"
#include "script/script.h"

#include "script/program/expression.h"

namespace script
{

TemplateImpl::TemplateImpl(const std::string & n, NativeTemplateDeductionFunction deduc, Engine *e, std::shared_ptr<ScriptImpl> s)
  : name(n)
  , deduction(deduc)
  , engine(e)
  , script(s)
{

}

FunctionTemplateImpl::FunctionTemplateImpl(const std::string & n, NativeTemplateDeductionFunction deduc, 
  NativeFunctionTemplateSubstitutionCallback sub, NativeFunctionTemplateInstantiationCallback inst,
  Engine *e, std::shared_ptr<ScriptImpl> s)
  : TemplateImpl(n, deduc, e, s)
  , substitute(sub)
  , instantiate(inst)
{

}

FunctionTemplateImpl::~FunctionTemplateImpl()
{

}

ClassTemplateImpl::ClassTemplateImpl(const std::string & n,
  NativeClassTemplateInstantiationFunction inst,
  Engine *e, std::shared_ptr<ScriptImpl> s)
  : TemplateImpl(n, nullptr, e, s)
  , instantiate(inst)
{

}

ClassTemplateImpl::~ClassTemplateImpl()
{

}


TemplateArgument TemplateArgument::make(const Type & t)
{
  TemplateArgument ret{ TypeArgument, t, 0, false };
  return ret;
}

TemplateArgument TemplateArgument::make(int val)
{
  TemplateArgument ret{ IntegerArgument, Type{}, val, false };
  return ret;
}

TemplateArgument TemplateArgument::make(bool val)
{
  TemplateArgument ret{ IntegerArgument, Type{}, 0, val };
  return ret;
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

bool Template::deduce(std::vector<TemplateArgument> & result, const std::vector<Type> & args)
{
  return d->deduction(result, args);
}

std::weak_ptr<TemplateImpl> Template::weakref() const
{
  return std::weak_ptr<TemplateImpl>(d);
}

bool Template::operator==(const Template & other) const
{
  return d == other.d;
}


FunctionTemplate::FunctionTemplate(const std::shared_ptr<FunctionTemplateImpl> & impl)
  : Template(impl)
{

}

void FunctionTemplate::complete(NameLookup & lookup, const std::vector<TemplateArgument> & targs, const std::vector<std::shared_ptr<program::Expression>> & args)
{
  auto l = lookup.impl();
  if (l->functionTemplateResult.empty())
    return;

  std::vector<Type> types;
  for (const auto & a : args)
    types.push_back(a->type());

  complete(lookup, targs, types);
}

void FunctionTemplate::complete(NameLookup & lookup, const std::vector<TemplateArgument> & targs, const std::vector<Type> & args)
{
  auto l = lookup.impl();
  
  // Removing duplicates, important to perform overload resolution
  auto & vec = l->functionTemplateResult;
  std::sort(vec.begin(), vec.end());
  vec.erase(std::unique(vec.begin(), vec.end()), vec.end());

  for (size_t i(0); i < vec.size(); ++i)
  {
    auto template_args = targs;
    FunctionTemplate ft = vec.at(i);
    /// TODO : if we can ensure that deduce() does not modify template_args on failure,
    // we could save some copying of targs.
    bool success = ft.deduce(template_args, args);
    if (!success)
      continue;
    Function f;
    if (!ft.hasInstance(template_args, &f))
    {
      f = ft.substitute(template_args);
    }
    if (f.isNull())
      continue;
    l->functions.push_back(f);
  }

  vec.clear();
}

Function FunctionTemplate::substitute(const std::vector<TemplateArgument> & targs)
{
  return impl()->substitute(*this, targs);
}

void FunctionTemplate::instantiate(Function & f)
{
  if (!f.isTemplateInstance())
    throw std::runtime_error{ "Function is not a template instance" };

  impl()->instantiate(*this, f);

  impl()->instances[f.arguments()] = f;
}

Function FunctionTemplate::build(const FunctionBuilder & builder, const std::vector<TemplateArgument> & args)
{
  auto impl = std::make_shared<FunctionTemplateInstance>(*this, args, builder.name, builder.proto, engine(), builder.flags);
  impl->implementation.callback = builder.callback;
  impl->data = builder.data;
  return Function{ impl };
}

void FunctionTemplate::setInstanceData(Function & f, const std::shared_ptr<UserData> & data)
{
  f.implementation()->data = data;
}

bool FunctionTemplate::hasInstance(const std::vector<TemplateArgument> & args, Function *value) const
{
  auto d = impl();
  auto it = d->instances.find(args);
  if (it == d->instances.end())
    return false;
  if (value != nullptr)
    *value = it->second;
  return true;
}

Function FunctionTemplate::getInstance(const std::vector<TemplateArgument> & args)
{
  Function ret;
  if (hasInstance(args, &ret))
    return ret;

  auto d = impl();
  ret = d->substitute(*this, args);
  ret = d->instantiate(*this, ret);
  if(ret.isNull())
    throw TemplateInstantiationError{std::string{"An error occurred while instantiating the '"} 
    + d->name + std::string{"' function template"} };

  d->instances[args] = ret;
  return ret;
}

Function FunctionTemplate::addSpecialization(const std::vector<TemplateArgument> & args, const FunctionBuilder & opts)
{
  auto d = impl();
  Function ret = d->engine->newFunction(opts);
  if (ret.isNull())
    return ret;
  d->instances[args] = ret;
  return ret;
}

const std::map<std::vector<TemplateArgument>, Function, TemplateArgumentComparison> & FunctionTemplate::instances() const
{
  auto d = impl();
  return d->instances;
}

std::shared_ptr<FunctionTemplateImpl> FunctionTemplate::impl() const
{
  return std::dynamic_pointer_cast<FunctionTemplateImpl>(d);
}


ClassTemplate::ClassTemplate(const std::shared_ptr<ClassTemplateImpl> & impl)
  : Template(impl)
{

}

bool ClassTemplate::hasInstance(const std::vector<TemplateArgument> & args, Class *value) const
{
  auto d = impl();
  auto it = d->instances.find(args);
  if (it == d->instances.end())
    return false;
  if (value != nullptr)
    *value = it->second;
  return true;
}

Class ClassTemplate::getInstance(const std::vector<TemplateArgument> & args)
{
  Class ret;
  if (hasInstance(args, &ret))
    return ret;

  auto d = impl();
  ret = d->instantiate(*this, args);
  if (ret.isNull())
    throw TemplateInstantiationError{ std::string{ "An error occurred while instantiating the '" }
                                      + d->name + std::string{ "' class template" } };

  d->instances[args] = ret;
  return ret;
}

Class ClassTemplate::addSpecialization(const std::vector<TemplateArgument> & args, const ClassBuilder & opts)
{
  auto d = impl();
  Class ret = d->engine->newClass(opts);
  if (ret.isNull())
    return ret;
  d->instances[args] = ret;
  return ret;
}

const std::map<std::vector<TemplateArgument>, Class, TemplateArgumentComparison> & ClassTemplate::instances() const
{
  auto d = impl();
  return d->instances;
}

std::shared_ptr<ClassTemplateImpl> ClassTemplate::impl() const
{
  return std::dynamic_pointer_cast<ClassTemplateImpl>(d);
}

} // namespace script
