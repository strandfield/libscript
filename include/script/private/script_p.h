// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SCRIPT_P_H
#define LIBSCRIPT_SCRIPT_P_H

#include "script/namespace.h"
#include "script/private/namespace_p.h"
#include "script/scope.h"
#include "script/sourcefile.h"
#include "script/diagnosticmessage.h"

namespace script
{

namespace ast
{
class AST;
} // namespace ast

class ScriptImpl : public NamespaceImpl
{
public:
  ScriptImpl(int id, Engine *e, const SourceFile & src);
  ~ScriptImpl() = default;

  int id;
  bool loaded;
  SourceFile source;
  Function program;
  std::vector<Value> globals;
  std::vector<Type> global_types;
  std::map<std::string, int> globalNames;
  std::vector<diagnostic::DiagnosticMessage> messages;
  bool astlock;
  std::shared_ptr<ast::AST> ast;
  Scope exports;

  void register_global(const Type & t, const std::string & name);
};

} // namespace script

#endif // LIBSCRIPT_SCRIPT_P_H
