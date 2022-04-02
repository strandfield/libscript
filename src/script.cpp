// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/script.h"
#include "script/private/script_p.h"

#include "script/ast.h"
#include "script/engine.h"
#include "script/symbol.h"

#include "script/program/statements.h"

namespace script
{

ScriptImpl::ScriptImpl(int index, Engine *e, const SourceFile & src)
  : NamespaceImpl("", e)
  , id(index)
  , loaded(false)
  , source(src)
  , astlock(false)
{

}


void ScriptImpl::register_global(const Type & t, std::string name)
{
  this->global_types.push_back(t);
  this->globalNames.insert({ std::move(name), int(this->global_types.size() - 1) });
}

static void add_breakpoint_to_list(std::vector<std::shared_ptr<program::Breakpoint>>& breakpoints, std::shared_ptr<program::Breakpoint> bp)
{
  if (breakpoints.empty() || breakpoints.back()->line < bp->line)
  {
    breakpoints.push_back(bp);
    bp->leading = true;
  }
  else
  {
    auto it = std::find_if(breakpoints.begin(), breakpoints.end(), [bp](const std::shared_ptr<program::Breakpoint>& other) {
      return other->line >= bp->line;
      });

    if (it != breakpoints.end() && (*it)->line == bp->line)
      return;

    breakpoints.insert(it, bp);
    bp->leading = true;
  }
}

void ScriptImpl::add_breakpoint(script::Function f, std::shared_ptr<program::Breakpoint> bp)
{
  add_breakpoint_to_list(this->breakpoints_map[f.impl()], bp);

}

/*!
 * \class Script
 * \brief Represents an executable script
 */

Script::Script(const std::shared_ptr<ScriptImpl> & impl)
  : d(impl)
{

}

int Script::id() const
{
  return d->id;
}

const std::string & Script::path() const
{
  return d->source.filepath();
}

/**
 * \fn void attach(FunctionCreator& fcreator)
 * \brief attach a function creator to this script
 * 
 * This creator will be used while compiling the script.
 * The creator does not need to outlive the Script but must be alive 
 * when \m compile() is called.
 */
void Script::attach(FunctionCreator& fcreator)
{
  d->function_creator = &fcreator;
}

/*!
 * \fn bool compile(CompileMode mode, FunctionCreator* fcreator = nullptr)
 * \brief Compiles the script
 * Returns true on success, false otherwise. 
 * If the compilation failed, use messages() to retrieve the error messages.
 * Warning: Calling this function while a script is compiling is undefined behavior.
 */
bool Script::compile(CompileMode mode, FunctionCreator* fcreator)
{
  Engine *e = d->engine;
  d->function_creator = fcreator ? fcreator : d->function_creator;
  return e->compile(*this, mode);
}

bool Script::isReady() const
{
  return !d->program.isNull();
}

void Script::run()
{
  if (d->program.isNull())
    throw std::runtime_error{ "Script was not compiled" };

  d->globals.clear();

  d->program.invoke({});
}

void Script::clear()
{
  // @TODO: unregister types
  d->typedefs.clear();
  d->static_variables.clear();
  d->variables.clear();
  d->templates.clear();
  d->classes.clear();
  d->enums.clear();
  d->functions.clear();
  d->operators.clear();
  d->literal_operators.clear();
  d->namespaces.clear();
  d->attributes.clear();
}

const SourceFile & Script::source() const
{
  return d->source;
}

Namespace Script::rootNamespace() const
{
  return Namespace{ d };
}

const std::map<std::string, int> & Script::globalNames() const
{
  return d->globalNames;
}

const std::vector<Value> & Script::globals() const
{
  return d->globals;
}

const std::vector<diagnostic::DiagnosticMessage> & Script::messages() const
{
  return d->messages;
}

Scope Script::exports() const
{
  return d->exports;
}

Attributes Script::getAttributes(const Symbol& sym) const
{
  return d->attributes.getAttributesFor(sym.impl().get());
}

Attributes Script::getAttributes(const Function& f) const
{
  return d->attributes.getAttributesFor(f.impl().get());
}

/*!
 * \fn DefaultArguments getDefaultArguments(const Function& f) const
 * \brief returns the default arguments for a given function
 */
DefaultArguments Script::getDefaultArguments(const Function& f) const
{
  return d ? d->defaultarguments.get(f.impl().get()) : DefaultArguments();
}

/*!
 * \fn Ast ast() const
 * \brief Returns the script ast.
 */
Ast Script::ast() const
{
  return Ast{ d->ast };
}

/*!
 * \fn void clearAst()
 * \brief Destroys the script ast.
 *
 * Note that this does nothing if the ast is marked as locked.
 * An ast is locked by the implementation when it may be needed later in time; 
 * for example when the script contains templates.
 */
void Script::clearAst()
{
  if (d->astlock)
    return;

  d->ast = nullptr;
}

class BreakpointFetcher
{
public:
  const Script& script;
  int line;
  int closest_dist = std::numeric_limits<int>::max();
  std::vector<std::pair<Function, std::shared_ptr<program::Breakpoint>>> result;

  BreakpointFetcher(const Script& s, int l)
    : script(s), line(l)
  {

  }

  void operator()()
  {
    for (auto it = script.impl()->breakpoints_map.begin(); it != script.impl()->breakpoints_map.end(); ++it)
    {
      const auto& elem = *it;
      script::Function f{ elem.first };

      if (f.isNull())
        continue;

      (*this)(f, elem.second);
    }
  }

  void operator()(script::Function& f, const std::vector<std::shared_ptr<program::Breakpoint>>& bps)
  {
    auto it = std::find_if(bps.begin(), bps.end(), [this](const std::shared_ptr<program::Breakpoint>& bp) {
      return bp->line >= line;
      });

    if (it == bps.end())
      return;

    std::shared_ptr<program::Breakpoint> bp = *it;

    if ((bp->line - line) < closest_dist)
    {
      closest_dist = bp->line - line;
      result.clear();
    }
    
    if (bp->line - line == closest_dist)
    {
      result.push_back(std::make_pair(f, bp));
    }
  }
};

/*!
 * \fn std::vector<std::pair<Function, std::shared_ptr<program::Breakpoint>>> breakpoints(int line) const
 * \brief returns breakpoints associated to a given line from the script
 */
std::vector<std::pair<Function, std::shared_ptr<program::Breakpoint>>> Script::breakpoints(int line) const
{
  BreakpointFetcher fetcher{ *this, line };
  fetcher();
  return fetcher.result;
}

Engine * Script::engine() const
{
  return d->engine;
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
