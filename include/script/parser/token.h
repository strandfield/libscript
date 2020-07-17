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

class Token
{
public:
  Token()
    : id(Token::Invalid)
    , flags(0)
    , pos(-1)
    , length(0)
    , line(-1)
    , column(-1)
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
  Token(Id t, int flags_, int pos_, int size_, int line_, int column_)
    : id(t)
    , flags(flags_)
    , pos(pos_)
    , length(size_)
    , line(line_)
    , column(column_)
  {

  }


  Id id;
  int flags;
  int pos;
  int length;
  uint16 line;
  uint16 column;

  bool isValid() const { return id != Invalid; }

  bool isOperator() const { return flags & OperatorToken; }
  bool isIdentifier() const { return flags & Identifier; }
  bool isKeyword() const { return flags & Keyword; }
  bool isLiteral() const { return flags & Literal; }

  bool isZero() const { return id == OctalLiteral && length == 1; }

  Token & operator=(const Token &) = default;

};

inline bool operator==(const Token& lhs, const Token& rhs)
{
  return lhs.id == rhs.id && lhs.length == rhs.length
    && lhs.pos == rhs.pos;
}

inline bool operator!=(const Token& lhs, const Token& rhs) { return !(lhs == rhs); }

inline bool operator==(const Token& lhs, Token::Id rhs) { return lhs.id == rhs; }
inline bool operator!=(const Token& lhs, Token::Id rhs) { return !(lhs == rhs); }

LIBSCRIPT_API const std::string& to_string(Token::Id toktype);

} // parser

} // namespace script


#endif // LIBSCRIPT_TOKEN_H
