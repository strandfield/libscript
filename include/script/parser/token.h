// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TOKEN_H
#define LIBSCRIPT_TOKEN_H

#include "libscriptdefs.h"

#include <string>

namespace script
{

namespace parser
{

class StringView
{
private:
  const char* m_data;
  size_t m_size;

public:
  StringView()
    : m_data(""), m_size(0)
  {

  }

  StringView(const StringView&) = default;

  StringView(const char* str, size_t s)
    : m_data(str), m_size(s)
  {

  }

  size_t size() const { return m_size; }
  const char* data() const { return m_data; }

  std::string toString() const { return std::string(data(), data() + size()); }

  StringView& operator=(const StringView&) = default;
};

inline bool operator==(const StringView& lhs, const StringView& rhs)
{
  return lhs.size() == rhs.size() &&
    (lhs.data() == rhs.data() || std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0);
}

inline bool operator!=(const StringView& lhs, const StringView& rhs)
{
  return !(lhs == rhs);
}

inline bool operator==(const StringView& lhs, const char* rhs)
{
  size_t offset = 0;

  for (; offset < lhs.size() && *rhs != '\0'; ++offset, ++rhs)
  {
    if (*rhs != lhs.data()[offset])
      return false;
  }

  return offset == lhs.size() && *rhs == '\0';
}

inline bool operator!=(const StringView& lhs, const char* rhs)
{
  return !(lhs == rhs);
}

class Token
{
public:
  Token()
    : id(Token::Invalid)
    , flags(0)
  {

  }

  Token(const Token &) = default;
  ~Token() = default;


  enum Kind {
    Punctuator = 0x010000,
    Literal    = 0x020000,
    OperatorToken   = 0x040000,
    Identifier = 0x080000,
    Keyword    = 0x100000 | Identifier,
  };

  enum Id {
    Invalid,
    /* Literals */
    IntegerLiteral,
    DecimalLiteral,
    BinaryLiteral,
    OctalLiteral,
    HexadecimalLiteral,
    // @TODO: do we need a CharLiteral ?
    StringLiteral,
    /* Punctuators */
    LeftPar,
    RightPar,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    Semicolon,
    Colon,
    Dot,
    QuestionMark,
    SlashSlash,
    SlashStar,
    StarSlash,
    /* Keywords */
    Auto,
    Bool,
    Break,
    Char,
    Class,
    Const,
    Continue,
    Default,
    Delete,
    Double,
    Else,
    Enum,
    Explicit,
    Export,
    False,
    Float,
    For,
    Friend,
    If,
    Import,
    Int,
    Mutable,
    Namespace,
    Operator,
    Private,
    Protected,
    Public,
    Return,
    Static,
    Struct,
    Template,
    This,
    True,
    Typedef,
    Typeid,
    Typename,
    Using,
    Virtual,
    Void,
    While,
    /* Operators */
    ScopeResolution,
    PlusPlus,
    MinusMinus,
    Plus,
    Minus,
    LogicalNot,
    BitwiseNot,
    Mul,
    Div,
    Remainder,
    LeftShift,
    RightShift,
    Less,
    GreaterThan,
    LessEqual,
    GreaterThanEqual,
    EqEq,
    Neq,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,
    LogicalAnd,
    LogicalOr,
    Eq,
    MulEq,
    DivEq,
    AddEq,
    SubEq,
    RemainderEq,
    LeftShiftEq,
    RightShiftEq,
    BitAndEq,
    BitOrEq,
    BitXorEq,
    Comma,
    UserDefinedName,
    UserDefinedLiteral,
    SingleLineComment,
    LeftRightPar,
    LeftRightBracket,
    MultiLineComment,
    LastTokenId,
    /* Alias */
    Ampersand = BitwiseAnd,
    Ref = Ampersand,
    RefRef = LogicalAnd,
    LeftAngle = Less,
    RightAngle = GreaterThan,
    LeftLeftAngle = LeftShift,
    RightRightAngle = RightShift,
    Tilde = BitwiseNot,
    Asterisk = Mul,
    Star = Asterisk,
  };

  // @TODO: add a constructor that computes the 'flags' automatically using a built-in table
  Token(Id t, int flags_, StringView str)
    : id(t)
    , flags(flags_)
    , m_text(str)
  {

  }


  Id id;
  int flags;
  StringView m_text;

  bool isValid() const { return id != Invalid; }

  bool isOperator() const { return flags & OperatorToken; }
  bool isIdentifier() const { return flags & Identifier; }
  bool isKeyword() const { return flags & Keyword; }
  bool isLiteral() const { return flags & Literal; }

  bool isZero() const { return id == OctalLiteral && m_text.size() == 1; }

  StringView text() const { return m_text; }
  std::string toString() const { return text().toString(); }

  Token & operator=(const Token &) = default;

};

inline bool operator==(const Token& lhs, const Token& rhs)
{
  return lhs.id == rhs.id && lhs.text() == rhs.text();
}

inline bool operator!=(const Token& lhs, const Token& rhs) { return !(lhs == rhs); }

inline bool operator==(const Token& lhs, Token::Id rhs) { return lhs.id == rhs; }
inline bool operator!=(const Token& lhs, Token::Id rhs) { return !(lhs == rhs); }

LIBSCRIPT_API const std::string& to_string(Token::Id toktype);

} // parser

} // namespace script


#endif // LIBSCRIPT_TOKEN_H
