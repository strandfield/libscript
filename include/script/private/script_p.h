// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SCRIPT_P_H
#define LIBSCRIPT_SCRIPT_P_H

#include "script/namespace.h"
#include "script/sourcefile.h"
#include "script/diagnosticmessage.h"

namespace script
{

class ScriptImpl
{
public:
  ScriptImpl(Engine *e, const SourceFile & src);

  bool loaded;
  Engine *engine;
  SourceFile source;
  Function program;
  Namespace root;
  std::vector<Value> globals;
  std::vector<Type> global_types;
  std::map<std::string, int> globalNames;
  std::vector<diagnostic::Message> messages;

  void register_global(const Type & t, const std::string & name);
};

} // namespace script

#endif // LIBSCRIPT_SCRIPT_P_H
