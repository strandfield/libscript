// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <iostream>

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/functiontype.h"
#include "script/enumerator.h"

#include "script/compiler/compiler.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/functioncompiler.h"
#include "script/compiler/scriptcompiler.h"

#include "script/parser/parser.h"
#include "script/parser/parsererrors.h"

TEST(CompilerErrors, funvar_not_initialized) {
  using namespace script;

  const char *source =
    " int(int) func; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::FunctionVariablesMustBeInitialized);
}

TEST(CompilerErrors, not_default_constructible) {
  using namespace script;

  const char *source =
    " class A {}; A a; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::VariableCannotBeDefaultConstructed);
}

TEST(CompilerErrors, invalid_param_count_op_overload) {
  using namespace script;

  const char *source =
    " class A {}; int operator+(const A & a, const A & b, const A & c) { return 0; } ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::InvalidParamCountInOperatorOverload);
}

TEST(CompilerErrors, data_member_auto) {
  using namespace script;

  const char *source =
    " class A { auto x; }; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::DataMemberCannotBeAuto);
}

TEST(CompilerErrors, missing_static_data_member_init) {
  using namespace script;

  const char *source =
    " class A { static int x; }; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::MissingStaticInitialization);
}

TEST(CompilerErrors, invalid_base_class) {
  using namespace script;

  const char *source =
    " class A : B {}; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::InvalidBaseClass);
}

TEST(CompilerErrors, invalid_default_arg) {
  using namespace script;

  const char *source =
    " int sum(int a = 0, int b) { return a + b; } ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::InvalidUseOfDefaultArgument);
}

TEST(CompilerErrors, array_elem_not_convertible) {
  using namespace script;

  const char *source =
    " class A { A() = default; }; auto a = [1, A{}]; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::ArrayElementNotConvertible);
}

TEST(CompilerErrors, invalid_array_subscript) {
  using namespace script;

  const char *source =
    " int a = 5; int b = a[10]; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::ArraySubscriptOnNonObject);
}

TEST(CompilerErrors, too_many_args_in_init_1) {
  using namespace script;

  const char *source =
    " int a{1, 2}; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::TooManyArgumentInVariableInitialization);
}

TEST(CompilerErrors, base_no_copy_ctor) {
  using namespace script;

  const char *source =
    "  class A { };                                  "
    "  class B : A { B(const B &) = default; };      ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::ParentHasNoCopyConstructor);
}


TEST(CompilerErrors, base_deleted_move_ctor) {
  using namespace script;

  const char *source =
    "  class A { A(A &&) = delete; };           "
    "  class B : A { B(B &&) = default; };      ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::ParentHasDeletedMoveConstructor);
}

TEST(CompilerErrors, no_valid_literal_operator) {
  using namespace script;

  const char *source =
    "  auto d = 3km;  ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::CouldNotFindValidLiteralOperator);
}

TEST(CompilerErrors, narrowing_conversion) {
  using namespace script;

  const char *source =
    "  int a{3.14};  ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().code(), CompilerError::NarrowingConversionInBraceInitialization);
}
