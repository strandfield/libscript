// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/interpreter/workspace.h"

#include "script/compiler/debug-info.h"
#include "script/interpreter/executioncontext.h"
#include "script/program/statements.h"

namespace script
{

namespace interpreter
{

Workspace::Workspace(FunctionCall* c)
  : m_call(c)
{
  refresh();
}

FunctionCall* Workspace::call() const
{
  return m_call;
}

void Workspace::reset(FunctionCall* c)
{
  m_call = c;
  refresh();
}

void Workspace::refresh()
{
  m_infos.clear();
  m_offset = 0;

  if (m_call && m_call->last_breakpoint)
  {
    auto info = m_call->last_breakpoint->debug_info;

    while (info)
    {
      m_infos.push_back(info);
      info = info->prev;
    }

    if (!m_infos.empty() && m_infos.back()->varname == "return-value")
    {
      m_offset = 1;
      m_infos.pop_back();
    }

    std::reverse(m_infos.begin(), m_infos.end());
  }
}

size_t Workspace::size() const
{
  return m_infos.size();
}

Value Workspace::valueAt(size_t i) const
{
  size_t off = i;
  size_t sp = m_call->stackOffset();
  interpreter::Stack& stack = m_call->executionContext()->stack;
  return stack[off + sp + m_offset];
}

Type Workspace::varTypeAt(size_t i) const
{
  return m_infos.at(i)->vartype;
}

const std::string& Workspace::nameAt(size_t i) const
{
  return m_infos.at(i)->varname;
}

size_t Workspace::stackOffsetAt(size_t i) const
{
  size_t off = i;
  size_t sp = m_call->stackOffset();
  return off + sp + m_offset;
}

} // namespace interpreter

} // namespace script

