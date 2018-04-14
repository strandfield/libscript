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
    // do we need to create a CharLiteral 
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
    Friend = Keyword | 36,
    If = Keyword | 37,
    Int = Keyword | 38,
    Mutable = Keyword | 39,
    Namespace = Keyword | 40,
    Operator = Keyword | 41,
    Private = Keyword | 42,
    Protected = Keyword | 43,
    Public = Keyword | 44,
    Return = Keyword | 45,
    Static = Keyword | 46,
    Struct = Keyword | 47,
    Template = Keyword | 48,
    This = Keyword | 49,
    True = Literal | Keyword | 50,
    Typedef = Keyword | 51,
    Typeid = Keyword | 52,
    Typename = Keyword | 53,
    Using = Keyword | 54,
    Virtual = Keyword | 55,
    Void = Keyword | 56,
    While = Keyword | 57,
    // Operators 
    ScopeResolution = OperatorToken | 58,
    PlusPlus = OperatorToken | 59,
    MinusMinus = OperatorToken | 60,
    Plus = OperatorToken | Punctuator | 61,
    Minus = OperatorToken | Punctuator | 62,
    LogicalNot = OperatorToken | Punctuator | 63,
    BitwiseNot = OperatorToken | Punctuator | 64,
    Mul = OperatorToken | Punctuator | 65,
    Div = OperatorToken | Punctuator | 66,
    Remainder = OperatorToken | Punctuator | 67,
    LeftShift = OperatorToken | 68,
    RightShift = OperatorToken | 69,
    Less = OperatorToken | Punctuator | 70,
    GreaterThan = OperatorToken | Punctuator | 71,
    LessEqual = OperatorToken | 72,
    GreaterThanEqual = OperatorToken | 73,
    EqEq = OperatorToken | 74,
    Neq = OperatorToken | 75,
    BitwiseAnd = OperatorToken | Punctuator | 76,
    BitwiseOr = OperatorToken | Punctuator | 77,
    BitwiseXor = OperatorToken | Punctuator | 78,
    LogicalAnd = OperatorToken | 79,
    LogicalOr = OperatorToken | 80,
    Eq = OperatorToken | 81,
    MulEq = OperatorToken | 82,
    DivEq = OperatorToken | 83,
    AddEq = OperatorToken | 84,
    SubEq = OperatorToken | 85,
    RemainderEq = OperatorToken | 86,
    LeftShiftEq = OperatorToken | 87,
    RightShiftEq = OperatorToken | 88,
    BitAndEq = OperatorToken | 89,
    BitOrEq = OperatorToken | 90,
    BitXorEq = OperatorToken | 91,
    Comma = OperatorToken | 92,
    UserDefinedName = Identifier | 93,
    UserDefinedLiteral = Literal | 94,
    SingleLineComment = 95,
    LeftRightPar = 96,
    LeftRightBracket = 97,
    // perhaps it would be better to have two tokens for  
    // multiline comments : an opening token and a closing one
    MultiLineComment = 98,
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
