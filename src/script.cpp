// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/script.h"
#include "script/private/script_p.h"

#include "script/engine.h"

namespace script
{

ScriptImpl::ScriptImpl(Engine *e, const SourceFile & src)
  : loaded(false)
  , engine(e)
  , source(src)
{

}


void ScriptImpl::register_global(const Type & t, const std::string & name)
{
  this->global_types.push_back(t);
  this->globalNames[name] = this->global_types.size() - 1;
}

Script::Script(const std::shared_ptr<ScriptImpl> & impl)
  : d(impl)
{

}

int Script::id() const
{
  throw std::runtime_error{ "Not implemented" };
}

const std::string & Script::name() const
{
  throw std::runtime_error{ "Not implemented" };
}

void Script::setName(const std::string & name)
{
  throw std::runtime_error{ "Not implemented" };
}

const std::string & Script::path() const
{
  return d->source.filepath();
}

bool Script::compile()
{
  Engine *e = d->engine;
  return e->compile(*this);
}

bool Script::isReady() const
{
  return !d->program.isNull();
}

void Script::run()
{
  if (d->program.isNull())
    throw std::runtime_error{ "Script was not compiled" };

  for (const auto & v : d->globals)
    engine()->manage(v);
  d->globals.clear();

  engine()->call(d->program, {});
}

void Script::clear()
{
  throw std::runtime_error{ "Not implemented" };

}

const SourceFile & Script::source() const
{
  return d->source;
}

const Namespace & Script::rootNamespace() const
{
  return d->root;
}

const std::map<std::string, int> & Script::globalNames() const
{
  return d->globalNames;
}

const std::vector<Value> & Script::globals() const
{
  return d->globals;
}

const std::vector<Class> & Script::classes() const
{
  return d->root.classes();
}

const std::vector<Operator> & Script::operators() const
{
  return d->root.operators();
}

const std::vector<diagnostic::Message> & Script::messages() const
{
  return d->messages;
}


Engine * Script::engine() const
{
  return d->engine;
}

ScriptImpl * Script::implementation() const
{
  return d.get();
}

std::weak_ptr<ScriptImpl> Script::weakref() const
{
  return std::weak_ptr<ScriptImpl>(d);
}

bool Script::operator==(const Script & other) const
{
  return d == other.d;
}

bool Script::operator!=(const Script & other) const
{
  return d != other.d;
}

} // namespace script
