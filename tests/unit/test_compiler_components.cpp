// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"

#include "script/compiler/compiler.h"
#include "script/compiler/functioncompiler.h"
#include "script/compiler/literalprocessor.h"
#include "script/compiler/scriptcompiler.h"


TEST(CompilerComponentsTests, literal_processor) {
  using namespace script;

  std::string str = "Hello World !";
  compiler::LiteralProcessor::postprocess(str);
  ASSERT_EQ(str, "Hello World !");

  str = "\\\\ \\t \\n \\r ";
  compiler::LiteralProcessor::postprocess(str);
  ASSERT_EQ(str, "\\ \t \n \r ");

  str = " \\ ";
  ASSERT_ANY_THROW(compiler::LiteralProcessor::postprocess(str));

  str = "128s";
  std::string suffix = compiler::LiteralProcessor::take_suffix(str);
  ASSERT_EQ(suffix, "s");
  ASSERT_EQ(str, "128");

  str = "\"Hello\"b";
  suffix = compiler::LiteralProcessor::take_suffix(str);
  ASSERT_EQ(suffix, "b");
  ASSERT_EQ(str, "\"Hello\"");

  str = "128e+10i";
  suffix = compiler::LiteralProcessor::take_suffix(str);
  ASSERT_EQ(suffix, "i");
  ASSERT_EQ(str, "128e+10");

  Engine e;
  e.setup();
  
  str = "\"Hello\"";
  Value val = compiler::LiteralProcessor::generate(&e, str);
  ASSERT_EQ(val.type(), Type::String);
  ASSERT_EQ(val.toString(), "Hello");
  e.destroy(val);

  str = "44";
  val = compiler::LiteralProcessor::generate(&e, str);
  ASSERT_EQ(val.type(), Type::Int);
  ASSERT_EQ(val.toInt(), 44);
  e.destroy(val);

  str = "3.14e0";
  val = compiler::LiteralProcessor::generate(&e, str);
  ASSERT_EQ(val.type(), Type::Double);
  ASSERT_EQ(val.toDouble(), 3.14);
  e.destroy(val);
}
