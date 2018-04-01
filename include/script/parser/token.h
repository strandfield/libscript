// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_TOKEN_H
#define LIBSCRIPT_TOKEN_H

#include "libscriptdefs.h"

namespace script
{

namespace parser
{


class Token
{
public:
  Token();
  Token(const Token & other) = default;
  ~Token() = default;


  enum Kind {
    Punctuator = 0x010000,
    Literal    = 0x020000,
    OperatorToken   = 0x040000,
    Identifier = 0x080000,
    Keyword    = 0x100000 | Identifier,
  };

  enum Type {
    Invalid = 0,
    // literals
    IntegerLiteral = Literal | 1,
    DecimalLiteral = Literal | 2,
    BinaryLiteral = Literal | 3,
    OctalLiteral = Literal | 4,
    HexadecimalLiteral = Literal | 5,
    /// TODO : do we need to create a CharLiteral ?
    StringLiteral = Literal | 6,
    // punctuators
    LeftPar = Punctuator | 7,
    RightPar = Punctuator | 8,
    LeftBracket = Punctuator | 9,
    RightBracket = Punctuator | 10,
    LeftBrace = Punctuator | 11,
    RightBrace = Punctuator | 12,
    Semicolon = Punctuator | 13,
    Colon = Punctuator | 14,
    Dot = Punctuator | 15,
    QuestionMark = Punctuator | 16,
    SlashSlash = Punctuator | 17,
    SlashStar = Punctuator | 18,
    StarSlash = Punctuator | 19,
    // keywords
    /// TODO : shoudl some of these also be marked as identifier ?
    Auto = Keyword | 20,
    Bool = Keyword | 21,
    Break = Keyword | 22,
    Char = Keyword | 23,
    Class = Keyword | 24,
    Const = Keyword | 25,
    Continue = Keyword | 26,
    Default = Keyword | 27,
    Delete = Keyword | 28,
    Double = Keyword | 29,
    Else = Keyword | 30,
    Enum = Keyword | 31,
    Explicit = Keyword | 32,
    False = Literal | Keyword | 33,
    Float = Keyword | 34,
    For = Keyword | 35,
    If = Keyword | 36,
    Int = Keyword | 37,
    Mutable = Keyword | 38,
    Namespace = Keyword | 39,
    Operator = Keyword | 40,
    Private = Keyword | 41,
    Protected = Keyword | 42,
    Public = Keyword | 43,
    Return = Keyword | 44,
    Static = Keyword | 45,
    Struct = Keyword | 46,
    Template = Keyword | 47,
    This = Keyword | 48,
    True = Literal | Keyword | 49,
    Typedef = Keyword | 50,
    Typeid = Keyword | 51,
    Typename = Keyword | 52,
    Using = Keyword | 53,
    Virtual = Keyword | 54,
    Void = Keyword | 55,
    While = Keyword | 56,
    // Operators,
    ScopeResolution = OperatorToken | 57,
    PlusPlus = OperatorToken | 58,
    MinusMinus = OperatorToken | 59,
    Plus = OperatorToken | Punctuator | 60,
    Minus = OperatorToken | Punctuator | 61,
    LogicalNot = OperatorToken | Punctuator | 62,
    BitwiseNot = OperatorToken | Punctuator | 63,
    Mul = OperatorToken | Punctuator | 64,
    Div = OperatorToken | Punctuator | 65,
    Remainder = OperatorToken | Punctuator | 66,
    LeftShift = OperatorToken | 67,
    RightShift = OperatorToken | 68,
    Less = OperatorToken | Punctuator | 69,
    GreaterThan = OperatorToken | Punctuator | 70,
    LessEqual = OperatorToken | 71,
    GreaterThanEqual = OperatorToken | 72,
    EqEq = OperatorToken | 73,
    Neq = OperatorToken | 74,
    BitwiseAnd = OperatorToken | Punctuator | 75,
    BitwiseOr = OperatorToken | Punctuator | 76,
    BitwiseXor = OperatorToken | Punctuator | 77,
    LogicalAnd = OperatorToken | 78,
    LogicalOr = OperatorToken | 79,
    Eq = OperatorToken | 80,
    MulEq = OperatorToken | 81,
    DivEq = OperatorToken | 82,
    AddEq = OperatorToken | 83,
    SubEq = OperatorToken | 84,
    RemainderEq = OperatorToken | 85,
    LeftShiftEq = OperatorToken | 86,
    RightShiftEq = OperatorToken | 87,
    BitAndEq = OperatorToken | 88,
    BitOrEq = OperatorToken | 89,
    BitXorEq = OperatorToken | 90,
    Comma = OperatorToken | 91,
    // misc
    UserDefinedName = Identifier | 92,
    UserDefinedLiteral = Literal | 93,
    SingleLineComment = 94,
    LeftRightPar = 95,
    LeftRightBracket = 96,
    // perhaps it would be better to have two tokens for= ,
    // multiline comments : an opening token and a closing one = ,
    MultiLineComment = 97,
    // alias
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

  Token(Type t, int pos, int size, int line, int column, int src = 0);

  Type type;
  int pos : 24;
  int src : 8;
  int length;
  uint16 line;
  uint16 column;

  inline bool isValid() const { return type != Invalid; }

  inline bool isOperator() const { return type & OperatorToken; }
  inline bool isIdentifier() const { return type & Identifier; }
  inline bool isKeyword() const { return type & Keyword; }
  inline bool isLiteral() const { return type & Literal; }

  bool operator==(const Token & other) const;
  inline bool operator!=(const Token & other) const { return !operator==(other); }
  bool operator==(Type tok) const;
  inline bool operator!=(Type tok) const { return !operator==(tok); }

  Token & operator=(const Token & other) = default;

};


} // parser

} // namespace script


#endif // LIBSCRIPT_TOKEN_H
