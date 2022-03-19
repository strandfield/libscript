// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/template.h"
#include "script/private/template_p.h"

#include "script/classtemplate.h"
#include "script/classtemplateinstancebuilder.h"
#include "script/engine.h"
#include "script/functiontemplate.h"
#include "script/name.h"
#include "script/private/symbol_p.h"
#include "script/private/templateargumentscope_p.h"
#include "script/symbol.h"

#include "script/compiler/compiler.h"

#include <algorithm> // std::max

namespace script
{

TemplateImpl::TemplateImpl(std::vector<TemplateParameter>&& params, const Scope& scp, Engine* e, std::shared_ptr<SymbolImpl> es)
  : SymbolImpl(es)
  , parameters(std::move(params))
  , scope(scp)
  , engine(e)
{

}

Name TemplateImpl::get_name() const
{
  return Name(SymbolKind::Template, this->name());
}


FunctionTemplateImpl::FunctionTemplateImpl(const std::string& n, std::vector<TemplateParameter>&& params, const Scope& scp, std::unique_ptr<FunctionTemplateNativeBackend>&& back,
  Engine* e, std::shared_ptr<SymbolImpl> es)
  : TemplateImpl(std::move(params), scp, e, es),
  function_name(n),
  backend(std::move(back))
{

}

FunctionTemplateImpl::~FunctionTemplateImpl()
{

}

const std::string& FunctionTemplateImpl::name() const
{
  return function_name;
}

ClassTemplateImpl::ClassTemplateImpl(const std::string& n,
  std::vector<TemplateParameter>&& params,
  const Scope& scp,
  std::unique_ptr<ClassTemplateNativeBackend>&& back,
  Engine* e, std::shared_ptr<SymbolImpl> es)
  : TemplateImpl(std::move(params), scp, e, es)
  , class_name(n)
  , backend(std::move(back))
{

}

ClassTemplateImpl::~ClassTemplateImpl()
{

}

const std::string& ClassTemplateImpl::name() const
{
  return class_name;
}

const std::vector<PartialTemplateSpecialization>& ClassTemplateImpl::specializations() const
{
  static const std::vector<PartialTemplateSpecialization> static_instance = {};

  ScriptClassTemplateBackend* back = dynamic_cast<ScriptClassTemplateBackend*>(this->backend.get());

  return back ? back->specializations : static_instance;
}

Class ScriptClassTemplateBackend::instantiate(ClassTemplateInstanceBuilder& builder)
{
  ClassTemplate ct = builder.getTemplate();
  Engine* e = ct.engine();
  compiler::Compiler* cc = e->compiler();
  Class ret = cc->instantiate(ct, builder.arguments());
  return ret;
}

PartialTemplateSpecializationImpl::PartialTemplateSpecializationImpl(const ClassTemplate& ct, std::vector<TemplateParameter>&& params, const Scope& scp, Engine* e, std::shared_ptr<SymbolImpl> es)
  : TemplateImpl(std::move(params), scp, e, es),
  class_template(ct.impl())
{

}

const std::string& PartialTemplateSpecializationImpl::name() const
{
  return class_template.lock()->name();
}

TemplateArgument::TemplateArgument()
  : kind(UnspecifiedArgument), integer(0), boolean(false) { }

TemplateArgument::TemplateArgument(const Type& t)
  : kind(TypeArgument), type(t), integer(0), boolean(false) { }

TemplateArgument::TemplateArgument(const Type::BuiltInType& t)
  : kind(TypeArgument), type(t), integer(0), boolean(false) { }

TemplateArgument::TemplateArgument(int n)
  : kind(IntegerArgument), integer(n), boolean(false) { }

TemplateArgument::TemplateArgument(bool b)
  : kind(BoolArgument), integer(0), boolean(b) { }

TemplateArgument::TemplateArgument(std::vector<TemplateArgument>&& args)
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

int TemplateArgumentComparison::compare(const TemplateArgument& a, const TemplateArgument& b)
{
  if (a.kind < b.kind)
    return -1;
  else if (a.kind > b.kind)
    return 1;

  const TemplateArgument::Kind common_kind = a.kind;

  if (common_kind == TemplateArgument::BoolArgument)
    return script::compare(a.boolean, b.boolean);
  else if (common_kind == TemplateArgument::IntegerArgument)
    return script::compare(a.integer, b.integer);
  else if (common_kind == TemplateArgument::TypeArgument)
    return script::compare(a.type.data(), b.type.data());

  assert(common_kind == TemplateArgument::PackArgument);

  if (a.pack->size() != b.pack->size())
    return script::compare((int)a.pack->size(), (int)b.pack->size());

  for (size_t i(0); i < a.pack->size(); ++i)
  {
    const int c = compare(a.pack->at(i), b.pack->at(i));
    if (c != 0)
      return c;
  }

  return 0;
}

bool TemplateArgumentComparison::operator()(const TemplateArgument& a, const TemplateArgument& b) const
{
  return compare(a, b) < 0;
}

bool TemplateArgumentComparison::operator()(const std::vector<TemplateArgument>& a, const std::vector<TemplateArgument>& b) const
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


bool operator==(const TemplateArgument& lhs, const TemplateArgument& rhs)
{
  return TemplateArgumentComparison::compare(lhs, rhs) == 0;
}

namespace errors
{

class TemplateInstantiationCategory : public std::error_category
{
public:

  const char* name() const noexcept override
  {
    return "template-instantiation-category";
  }

  std::string message(int) const override
  {
    return "template-instantiation-error";
  }
};

const std::error_category& template_instantiation_category() noexcept
{
  static const TemplateInstantiationCategory static_instance = {};
  return static_instance;
}

} // namespace errors


TemplateInstantiationError::TemplateInstantiationError(ErrorCode ec)
  : Exceptional(ec)
{

}

TemplateInstantiationError::TemplateInstantiationError(ErrorCode ec, std::string mssg)
  : Exceptional(ec, std::move(mssg))
{

}

Template::Template(const std::shared_ptr<TemplateImpl>& impl)
  : d(impl)
{

}

bool Template::isNull() const
{
  return d == nullptr;
}

Script Template::script() const
{
  return Symbol{ d->enclosing_symbol.lock() }.script();
}

Engine* Template::engine() const
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

const std::string& Template::name() const
{
  return d->name();
}

const std::vector<TemplateParameter>& Template::parameters() const
{
  return d->parameters;
}

Scope Template::scope() const
{
  return d->scope;
}

Scope Template::argumentScope(const std::vector<TemplateArgument>& args) const
{
  auto ret = std::make_shared<TemplateArgumentScope>(*this, args);
  ret->parent = d->scope.impl();
  return Scope{ ret };
}

Scope Template::parameterScope() const
{
  auto tparamscope = std::make_shared<TemplateParameterScope>(*this);
  tparamscope->parent = scope().impl();
  return Scope{ tparamscope };
}

TemplateArgument Template::get(const std::string& name, const std::vector<TemplateArgument>& args) const
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

Symbol Template::enclosingSymbol() const
{
  return Symbol{ d->enclosing_symbol.lock() };
}

std::weak_ptr<TemplateImpl> Template::weakref() const
{
  return std::weak_ptr<TemplateImpl>(d);
}

const std::map<std::type_index, Template>& Template::get_template_map(Engine* e)
{
  return e->templateMap();
}

bool Template::operator==(const Template& other) const
{
  return d == other.d;
}

} // namespace script
