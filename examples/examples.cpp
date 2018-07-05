// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "examples.h"

static std::vector<std::shared_ptr<Example>> static_examples;

static void dummy_example_init(script::Engine*)
{

}

Example::Example(int i, const std::string & n)
  : id(i)
  , name(n)
  , init(dummy_example_init)
{

}

Example* Example::setInit(ExampleSetupFunction init)
{
  this->init = init;
  return this;
}

const std::vector<std::shared_ptr<Example>> & Example::list()
{
  return static_examples;
}

Example* Example::registerExample(const std::string & name)
{
  std::shared_ptr<Example> ex = std::make_shared<Example>(static_examples.size(), name);
  static_examples.push_back(ex);
  return ex.get();
}
