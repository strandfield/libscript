// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/parser/token.h"

#include <vector>

namespace script
{

namespace parser
{

Token::Token()
  : type(Token::Invalid)
  , pos(-1)
  , length(0)
  , line(-1)
  , column(-1)
{

}

Token::Token(Type t, int pos, int size, int line, int column)
  : type(t)
  , pos(pos)
  , length(size)
  , line(line)
  , column(column)
{

}

bool Token::operator==(const Token & other) const
{
  return this->type == other.type && this->length == other.length
    && this->pos == other.pos;
}

bool Token::operator==(Type tok) const
{
  return this->type == tok;
}

static std::vector<std::string> build_token_type_strings()
{
  std::vector<std::string> result;
  result.resize(static_cast<int>(parser::Token::MultiLineComment) + 1);

  auto set_tokens = [&result](std::initializer_list<std::pair<parser::Token::Type, std::string>>&& list)
  {
    for (auto& p : list)
      result[static_cast<int>(p.first) & 0xFFFF] = std::move(p.second);
  };

  set_tokens({
    { parser::Token::LeftPar, "(" },
    { parser::Token::RightPar, ")" },
    { parser::Token::LeftBracket, "[" },
    { parser::Token::RightBracket, "]" },
    { parser::Token::LeftBrace, "{" },
    { parser::Token::RightBrace, "}" },
    { parser::Token::Semicolon, " }," },
    { parser::Token::Colon, ":" },
    { parser::Token::Dot, "." },
    { parser::Token::QuestionMark, "?" },
    { parser::Token::SlashSlash, "//" },
    { parser::Token::SlashStar, "/*" },
    { parser::Token::StarSlash, "*/" },
    // keywords 
    { parser::Token::Auto, "auto" },
    { parser::Token::Bool, "bool" },
    { parser::Token::Break, "break" },
    { parser::Token::Char, "char" },
    { parser::Token::Class, "class" },
    { parser::Token::Const, "const" },
    { parser::Token::Continue, "continue" },
    { parser::Token::Default, "default" },
    { parser::Token::Delete, "delete" },
    { parser::Token::Double, "double" },
    { parser::Token::Else, "else" },
    { parser::Token::Enum, "enum" },
    { parser::Token::Explicit, "explicit" },
    { parser::Token::Export, "export" },
    { parser::Token::False, "false" },
    { parser::Token::Float, "float" },
    { parser::Token::For, "for" },
    { parser::Token::Friend, "friend" },
    { parser::Token::If, "if" },
    { parser::Token::Import, "import" },
    { parser::Token::Int, "int" },
    { parser::Token::Mutable, "mutable" },
    { parser::Token::Namespace, "namespace" },
    { parser::Token::Operator, "operator" },
    { parser::Token::Private, "private" },
    { parser::Token::Protected, "protected" },
    { parser::Token::Public, "public" },
    { parser::Token::Return, "return" },
    { parser::Token::Static, "static" },
    { parser::Token::Struct, "struct" },
    { parser::Token::Template, "template" },
    { parser::Token::This, "this" },
    { parser::Token::True, "true" },
    { parser::Token::Typedef, "typedef" },
    { parser::Token::Typeid, "typeid" },
    { parser::Token::Typename, "typename" },
    { parser::Token::Using, "using" },
    { parser::Token::Virtual, "virtual" },
    { parser::Token::Void, "void" },
    { parser::Token::While, "while" },
    //Operators
    { parser::Token::ScopeResolution, "::" },
    { parser::Token::PlusPlus, "++" },
    { parser::Token::MinusMinus, "--" },
    { parser::Token::Plus, "+" },
    { parser::Token::Minus, "-" },
    { parser::Token::LogicalNot, "!" },
    { parser::Token::BitwiseNot, "~" },
    { parser::Token::Mul, "*" },
    { parser::Token::Div, "/" },
    { parser::Token::Remainder, "%" },
    { parser::Token::LeftShift, "<<" },
    { parser::Token::RightShift, ">>" },
    { parser::Token::Less, "<" },
    { parser::Token::GreaterThan, ">" },
    { parser::Token::LessEqual, "<=" },
    { parser::Token::GreaterThanEqual, ">=" },
    { parser::Token::EqEq, "==" },
    { parser::Token::Neq, "!=" },
    { parser::Token::BitwiseAnd, "&" },
    { parser::Token::BitwiseOr, "|" },
    { parser::Token::BitwiseXor, "^" },
    { parser::Token::LogicalAnd, "&&" },
    { parser::Token::LogicalOr, "||" },
    { parser::Token::Eq, "=" },
    { parser::Token::MulEq, "*=" },
    { parser::Token::DivEq, "/=" },
    { parser::Token::AddEq, "+=" },
    { parser::Token::SubEq, "-=" },
    { parser::Token::RemainderEq, "%=" },
    { parser::Token::LeftShiftEq, "<<=" },
    { parser::Token::RightShiftEq, ">>=" },
    { parser::Token::BitAndEq, "&=" },
    { parser::Token::BitOrEq, "|=" },
    { parser::Token::BitXorEq, "^=" },
    { parser::Token::Comma, "," },
    { parser::Token::LeftRightPar, "()" },
    { parser::Token::LeftRightBracket, "[]" },
    { parser::Token::Zero, "0" },
    });

  return result;
}

const std::string& to_string(Token::Type toktype)
{
  static const std::vector<std::string> static_map = build_token_type_strings();
  return static_map.at(static_cast<int>(toktype) & 0xFFFF);
}

} // namespace parser

} // namespace script

