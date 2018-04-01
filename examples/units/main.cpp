// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <iostream>

#include "script/engine.h"

int main()
{
  using namespace script;

  Engine e;
  e.setup();

  Script s = e.newScript(SourceFile{ "units.script" });
  const bool result = s.compile();
  if (result)
  {
    s.run();
    Value eq = s.globals().back();
    std::cout << eq.toBool() << std::endl;
  }
  else
  {
    std::cout << "Could not compile script " << s.source().filepath() << std::endl;
    const auto & messages = s.messages();
    for (const auto & m : messages)
      std::cout << m.to_string() << std::endl;
  }
}