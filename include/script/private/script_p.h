// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SCRIPT_P_H
#define LIBSCRIPT_SCRIPT_P_H

#include "script/namespace.h"
#include "script/private/namespace_p.h"
#include "script/sourcefile.h"
#include "script/diagnosticmessage.h"

namespace script
{

class ScriptImpl : public NamespaceImpl
{
public:
  ScriptImpl(Engine *e, const SourceFile & src);
  ~ScriptImpl() = default;

  bool loaded;
  SourceFile source;
  Function program;
  std::vector<Value> globals;
  std::vector<Type> global_types;
  std::map<std::string, int> globalNames;
  std::vector<diagnostic::Message> messages;

  void register_global(const Type & t, const std::string & name);
};

} // namespace script

#endif // LIBSCRIPT_SCRIPT_P_H
