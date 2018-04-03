// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/enum.h"
#include "script/conversions.h"

#include "script/functionbuilder.h"
#include "script/functiontype.h"

TEST(Conversions, standard) {
  using namespace script;

  Engine e;

  StandardConversion conv = StandardConversion::compute(Type::Int, Type::cref(Type::Int), &e);
  ASSERT_TRUE(conv.isReferenceInitialization());
  ASSERT_FALSE(conv.isCopyInitialization());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());

  conv = StandardConversion::compute(Type::Int, Type::Boolean, &e);
  ASSERT_FALSE(conv.isReferenceInitialization());
  ASSERT_TRUE(conv.isCopyInitialization());
  ASSERT_TRUE(conv.isNarrowing());
  ASSERT_TRUE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.numericConversion(), BooleanConversion);

  conv = StandardConversion::compute(Type::Int, Type::Float, &e);
  ASSERT_FALSE(conv.isReferenceInitialization());
  ASSERT_TRUE(conv.isCopyInitialization());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_TRUE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
  ASSERT_EQ(conv.numericPromotion(), FloatingPointPromotion);


  conv = StandardConversion::compute(Type::Int, Type::ref(Type::Int), &e);
  ASSERT_FALSE(conv == StandardConversion::NotConvertible());
  ASSERT_TRUE(conv.isReferenceInitialization());
  ASSERT_FALSE(conv.isCopyInitialization());
  ASSERT_FALSE(conv.isNarrowing());
  ASSERT_FALSE(conv.isNumericConversion());
  ASSERT_FALSE(conv.isNumericPromotion());
  ASSERT_FALSE(conv.isDerivedToBaseConversion());
}



TEST(Conversions, comparison) {
  using namespace script;

  Engine e;

  StandardConversion a{ FloatingPointPromotion };
  StandardConversion b;

  ASSERT_TRUE(a.rank() == StandardConversion::Rank::Promotion);
  ASSERT_TRUE(b.rank() == StandardConversion::Rank::ExactMatch);
  ASSERT_TRUE(b < a);
}

TEST(Conversions, enum_to_int) {
  using namespace script;

  Engine e;
  e.setup();

  Enum A = e.rootNamespace().newEnum("A");
  A.addValue("AA", 0);
  A.addValue("AB", 1);
  A.addValue("AC", 2);

  ConversionSequence conv = ConversionSequence::compute(A.id(), Type::Int, &e);
  ASSERT_FALSE(conv == ConversionSequence::NotConvertible());
  ASSERT_FALSE(conv.isUserDefinedConversion());

  conv = ConversionSequence::compute(A.id(), Type::ref(Type::Int), &e);
  ASSERT_TRUE(conv == ConversionSequence::NotConvertible());

  conv = ConversionSequence::compute(Type::ref(A.id()), Type::ref(Type::Int), &e);
  ASSERT_TRUE(conv == ConversionSequence::NotConvertible());
}

TEST(Conversions, user_defined_cast) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = e.rootNamespace().newClass(ClassBuilder::New("A"));
  Cast to_int = A.newCast(FunctionBuilder::Cast(Type::cref(A.id()), Type::Int));

  ConversionSequence conv = ConversionSequence::compute(A.id(), Type::Int, &e);
  ASSERT_FALSE(conv == ConversionSequence::NotConvertible());
  ASSERT_TRUE(conv.isUserDefinedConversion());
  ASSERT_EQ(conv.function, to_int);
}


TEST(Conversions, user_defined_converting_constructor) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = e.rootNamespace().newClass(ClassBuilder::New("A"));
  Function ctor = A.newConstructor(FunctionBuilder::Constructor(A).addParam(Type::Float));

  ConversionSequence conv = ConversionSequence::compute(Type::Float, A.id(), &e);
  ASSERT_FALSE(conv == ConversionSequence::NotConvertible());
  ASSERT_TRUE(conv.isUserDefinedConversion());
  ASSERT_EQ(conv.function, ctor);
}


TEST(Conversions, function_type) {
  using namespace script;

  Engine e;
  e.setup();

  auto ft = e.getFunctionType(Prototype{ Type::Void, Type::Int });

  ConversionSequence conv = ConversionSequence::compute(ft.type(), ft.type(), &e);
  ASSERT_FALSE(conv == ConversionSequence::NotConvertible());
  ASSERT_FALSE(conv.isUserDefinedConversion());
  ASSERT_TRUE(conv.conv1.isCopyInitialization());

  conv = ConversionSequence::compute(ft.type(), ft.type().withFlag(Type::ReferenceFlag), &e);
  ASSERT_FALSE(conv == ConversionSequence::NotConvertible());
  ASSERT_FALSE(conv.isUserDefinedConversion());
  ASSERT_TRUE(conv.conv1.isReferenceInitialization());

  auto ft2 = e.getFunctionType(Prototype{ Type::Void, Type::Float });

  conv = ConversionSequence::compute(ft.type(), ft2.type(), &e);
  ASSERT_TRUE(conv == ConversionSequence::NotConvertible());
}

TEST(Conversions, no_converting_constructor) {
  using namespace script;

  Engine e;
  e.setup();

  Class A = e.rootNamespace().newClass(ClassBuilder::New("A"));

  ConversionSequence conv = ConversionSequence::compute(Type::Float, A.id(), &e);
  ASSERT_TRUE(conv == ConversionSequence::NotConvertible());
}