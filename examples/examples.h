// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <string>
#include <vector>

namespace script
{
class Engine;
} // namespace script

typedef void(*ExampleSetupFunction)(script::Engine*);

class Example 
{
public:
  Example(int id, const std::string & n);
  ~Example() = default;

  int id;
  std::string name;
  ExampleSetupFunction init;

  Example* setInit(ExampleSetupFunction init);

  static const std::vector<std::shared_ptr<Example>> & list();
  static Example* registerExample(const std::string & name);
};


#define DECLARE_EXAMPLE(name) static Example*  name##_static_ptr = Example::registerExample(#name)