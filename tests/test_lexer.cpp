// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/parser/lexer.h"
#include "script/parser/token.h"


TEST(LexerTests, tokens) {
  using namespace script;
  using namespace parser;

  auto tok = [](const Token::Type & t) -> Token { return Token{ t, 0, 0, 0, 0 }; };

  ASSERT_TRUE(tok(Token::Mul).isOperator());
  ASSERT_TRUE(tok(Token::Int).isKeyword());
}


TEST(LexerTests, keywords) {
  using namespace script;
  using namespace parser;
  
  const char *source =
    "bool char int float double for while if else class struct auto using typedef namespace public protected private ";

  Lexer lex{ SourceFile::fromString(source) };
  ASSERT_EQ(lex.read(), Token::Bool);
  ASSERT_EQ(lex.read(), Token::Char);
  ASSERT_EQ(lex.read(), Token::Int);
  ASSERT_EQ(lex.read(), Token::Float);
  ASSERT_EQ(lex.read(), Token::Double);
  ASSERT_EQ(lex.read(), Token::For);
  ASSERT_EQ(lex.read(), Token::While);
  ASSERT_EQ(lex.read(), Token::If);
  ASSERT_EQ(lex.read(), Token::Else);
  ASSERT_EQ(lex.read(), Token::Class);
  ASSERT_EQ(lex.read(), Token::Struct);
  ASSERT_EQ(lex.read(), Token::Auto);
  ASSERT_EQ(lex.read(), Token::Using);
  ASSERT_EQ(lex.read(), Token::Typedef);
  ASSERT_EQ(lex.read(), Token::Namespace);
  ASSERT_EQ(lex.read(), Token::Public);
  ASSERT_EQ(lex.read(), Token::Protected);
  ASSERT_EQ(lex.read(), Token::Private);

  ASSERT_TRUE(lex.atEnd());
}


TEST(LexerTests, literals) {
  using namespace script;
  using namespace parser;

  const char *source =
    "0 5 3.14 0x1 0xFF 0xF3e 0b1010 5f 3. 3.14 5.f 5e210 5e10f 5";

  Lexer lex{ SourceFile::fromString(source) };
  ASSERT_EQ(lex.read().type, Token::OctalLiteral);
  ASSERT_EQ(lex.read().type, Token::IntegerLiteral);
  ASSERT_EQ(lex.read().type, Token::DecimalLiteral);
  ASSERT_EQ(lex.read().type, Token::HexadecimalLiteral);
  ASSERT_EQ(lex.read().type, Token::HexadecimalLiteral);
  ASSERT_EQ(lex.read().type, Token::HexadecimalLiteral);
  ASSERT_EQ(lex.read().type, Token::BinaryLiteral);
  ASSERT_EQ(lex.read().type, Token::DecimalLiteral);
  ASSERT_EQ(lex.read().type, Token::DecimalLiteral);
  ASSERT_EQ(lex.read().type, Token::DecimalLiteral);
  ASSERT_EQ(lex.read().type, Token::DecimalLiteral);
  ASSERT_EQ(lex.read().type, Token::DecimalLiteral);
  ASSERT_EQ(lex.read().type, Token::DecimalLiteral);
  ASSERT_EQ(lex.read().type, Token::IntegerLiteral);

  ASSERT_TRUE(lex.atEnd());
}

TEST(LexerTests, stringliterals) {
  using namespace script;
  using namespace parser;

  const char *source =
    "\"Hello, there\"    \"H\\\"a\" ";

  Lexer lex{ SourceFile::fromString(source) };
  ASSERT_EQ(lex.read().type, Token::StringLiteral);
  ASSERT_EQ(lex.read().type, Token::StringLiteral);

  ASSERT_TRUE(lex.atEnd());
}

TEST(LexerTests, userdefined_literals) {
  using namespace script;
  using namespace parser;

  const char *source =
    " 125km 10m 60s 26ms 3.14i";

  Lexer lex{ SourceFile::fromString(source) };
  ASSERT_EQ(lex.read().type, Token::UserDefinedLiteral);
  ASSERT_EQ(lex.read().type, Token::UserDefinedLiteral);
  ASSERT_EQ(lex.read().type, Token::UserDefinedLiteral);
  ASSERT_EQ(lex.read().type, Token::UserDefinedLiteral);
  ASSERT_EQ(lex.read().type, Token::UserDefinedLiteral);

  ASSERT_TRUE(lex.atEnd());
}

TEST(LexerTests, punctuators) {
  using namespace script;
  using namespace parser;

  const char *source =
    " ( ) [ ] {} ? : :: ,; ";

  Lexer lex{ SourceFile::fromString(source) };
  ASSERT_EQ(lex.read(), Token::LeftPar);
  ASSERT_EQ(lex.read(), Token::RightPar);
  ASSERT_EQ(lex.read(), Token::LeftBracket);
  ASSERT_EQ(lex.read(), Token::RightBracket);
  ASSERT_EQ(lex.read(), Token::LeftBrace);
  ASSERT_EQ(lex.read(), Token::RightBrace);
  ASSERT_EQ(lex.read(), Token::QuestionMark);
  ASSERT_EQ(lex.read(), Token::Colon);
  ASSERT_EQ(lex.read(), Token::ScopeResolution);
  ASSERT_EQ(lex.read(), Token::Comma);
  ASSERT_EQ(lex.read(), Token::Semicolon);

  ASSERT_TRUE(lex.atEnd());
}


TEST(LexerTests, operators) {
  using namespace script;
  using namespace parser;

  const char *source =
    " ++ -- + - * / % = += -= *= /= %= "
    " << >> <<= >>= "
    " == != < > <= >= "
    " && || ! | & ^ |= &= ^= ~";

  Lexer lex{ SourceFile::fromString(source) };
  ASSERT_EQ(lex.read(), Token::PlusPlus);
  ASSERT_EQ(lex.read(), Token::MinusMinus);
  ASSERT_EQ(lex.read(), Token::Plus);
  ASSERT_EQ(lex.read(), Token::Minus);
  ASSERT_EQ(lex.read(), Token::Mul);
  ASSERT_EQ(lex.read(), Token::Div);
  ASSERT_EQ(lex.read(), Token::Remainder);
  ASSERT_EQ(lex.read(), Token::Eq);
  ASSERT_EQ(lex.read(), Token::AddEq);
  ASSERT_EQ(lex.read(), Token::SubEq);
  ASSERT_EQ(lex.read(), Token::MulEq);
  ASSERT_EQ(lex.read(), Token::DivEq);
  ASSERT_EQ(lex.read(), Token::RemainderEq);
  ASSERT_EQ(lex.read(), Token::LeftShift);
  ASSERT_EQ(lex.read(), Token::RightShift);
  ASSERT_EQ(lex.read(), Token::LeftShiftEq);
  ASSERT_EQ(lex.read(), Token::RightShiftEq);
  ASSERT_EQ(lex.read(), Token::EqEq);
  ASSERT_EQ(lex.read(), Token::Neq);
  ASSERT_EQ(lex.read(), Token::Less);
  ASSERT_EQ(lex.read(), Token::GreaterThan); 
  ASSERT_EQ(lex.read(), Token::LessEqual);
  ASSERT_EQ(lex.read(), Token::GreaterThanEqual);
  ASSERT_EQ(lex.read(), Token::LogicalAnd);
  ASSERT_EQ(lex.read(), Token::LogicalOr);
  ASSERT_EQ(lex.read(), Token::LogicalNot);
  ASSERT_EQ(lex.read(), Token::BitwiseOr);
  ASSERT_EQ(lex.read(), Token::BitwiseAnd);
  ASSERT_EQ(lex.read(), Token::BitwiseXor);
  ASSERT_EQ(lex.read(), Token::BitOrEq);
  ASSERT_EQ(lex.read(), Token::BitAndEq);
  ASSERT_EQ(lex.read(), Token::BitXorEq);
  ASSERT_EQ(lex.read(), Token::BitwiseNot);

  ASSERT_TRUE(lex.atEnd());
}


TEST(LexerTests, identifiers) {
  using namespace script;
  using namespace parser;

  const char *source =
    " n id order66 _member _1 _ ";

  Lexer lex{ SourceFile::fromString(source) };
  ASSERT_EQ(lex.read(), Token::UserDefinedName);
  ASSERT_EQ(lex.read(), Token::UserDefinedName);
  ASSERT_EQ(lex.read(), Token::UserDefinedName);
  ASSERT_EQ(lex.read(), Token::UserDefinedName);
  ASSERT_EQ(lex.read(), Token::UserDefinedName);
  ASSERT_EQ(lex.read(), Token::UserDefinedName);

  ASSERT_TRUE(lex.atEnd());
}



TEST(LexerTests, mix1) {
  using namespace script;
  using namespace parser;

  const char *source =
    " int a = 5; ";

  Lexer lex{ SourceFile::fromString(source) };
  ASSERT_EQ(lex.read(), Token::Int);
  ASSERT_EQ(lex.read(), Token::UserDefinedName);
  ASSERT_EQ(lex.read(), Token::Eq);
  ASSERT_EQ(lex.read(), Token::IntegerLiteral);
  ASSERT_EQ(lex.read(), Token::Semicolon);

  ASSERT_TRUE(lex.atEnd());
}


TEST(LexerTests, mix2) {
  using namespace script;
  using namespace parser;

  const char *source =
    " for(int i(0); i < size(); ++i) { } ";

  Lexer lex{ SourceFile::fromString(source) };
  ASSERT_EQ(lex.read(), Token::For);
  ASSERT_EQ(lex.read(), Token::LeftPar);
  ASSERT_EQ(lex.read(), Token::Int);
  ASSERT_EQ(lex.read(), Token::UserDefinedName);
  ASSERT_EQ(lex.read(), Token::LeftPar);
  ASSERT_EQ(lex.read(), Token::OctalLiteral);
  ASSERT_EQ(lex.read(), Token::RightPar);
  ASSERT_EQ(lex.read(), Token::Semicolon);
  ASSERT_EQ(lex.read(), Token::UserDefinedName);
  ASSERT_EQ(lex.read(), Token::Less);
  ASSERT_EQ(lex.read(), Token::UserDefinedName);
  ASSERT_EQ(lex.read(), Token::LeftPar);
  ASSERT_EQ(lex.read(), Token::RightPar);
  ASSERT_EQ(lex.read(), Token::Semicolon);
  ASSERT_EQ(lex.read(), Token::PlusPlus);
  ASSERT_EQ(lex.read(), Token::UserDefinedName);
  ASSERT_EQ(lex.read(), Token::RightPar);
  ASSERT_EQ(lex.read(), Token::LeftBrace);
  ASSERT_EQ(lex.read(), Token::RightBrace);

  ASSERT_TRUE(lex.atEnd());
}
