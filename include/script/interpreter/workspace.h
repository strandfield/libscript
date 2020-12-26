// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_INTERPRETER_WORKSPACE_H
#define LIBSCRIPT_INTERPRETER_WORKSPACE_H

#include "script/value.h"

#include <memory>
#include <vector>

namespace script
{

namespace compiler
{
class DebugInfoBlock;
} // namespace compiler

namespace interpreter
{

class FunctionCall;

class LIBSCRIPT_API Workspace
{
private:
  FunctionCall* m_call = nullptr;
  size_t m_offset = 0;
  std::vector<std::shared_ptr<compiler::DebugInfoBlock>> m_infos;

public:
  Workspace() = default;
  Workspace(const Workspace&) = default;
  ~Workspace() = default;

  explicit Workspace(FunctionCall* c);

  FunctionCall* call() const;
  void reset(FunctionCall* c);
  void refresh();

  size_t size() const;
  Value valueAt(size_t i) const;
  Type varTypeAt(size_t i) const;
  const std::string& nameAt(size_t i) const;
  size_t stackOffsetAt(size_t i) const;
};

} // namespace interpreter

} // namespace script

#endif // LIBSCRIPT_INTERPRETER_WORKSPACE_H
