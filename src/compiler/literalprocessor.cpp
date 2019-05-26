// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/literalprocessor.h"

#include "script/engine.h"
#include "script/string.h"

#include "script/ast/node.h"

#include "script/compiler/compilererrors.h"

namespace script
{

namespace compiler
{

Value LiteralProcessor::generate(Engine *e, const std::shared_ptr<ast::Literal> & l)
{
  switch (l->type())
  {
  case ast::NodeType::BoolLiteral:
    return e->newBool(l->token == parser::Token::True);
  case ast::NodeType::IntegerLiteral:
    return e->newInt(generate(std::static_pointer_cast<ast::IntegerLiteral>(l)));
  case ast::NodeType::FloatingPointLiteral:
  {
    std::string str = l->toString();
    if (str.back() == 'f')
    {
      str.pop_back();
      float fval = std::stof(str);
      return e->newFloat(fval);
    }
    return e->newDouble(std::stod(str));
  }
  case ast::NodeType::StringLiteral:
  {
    std::string str = l->toString();
    postprocess(str);
    if (str.front() == '"')
      return e->newString(StringBackend::convert(std::string(str.begin() + 1, str.end() - 1)));
    if (str.size() != 3)
      throw InvalidCharacterLiteral{ };
    return e->newChar(str.at(1));
  }
  case ast::NodeType::UserDefinedLiteral:
  {
    throw NotImplemented{ "LiteralProcessor::generate() : User defined literal not supported here" };
  }
  default:
    break;
  }

  throw NotImplemented{ "LiteralProcessor::generate() : kind of literal not implemented" };
}

Value LiteralProcessor::generate(Engine *e, std::string & str)
{
  if (str.front() == '\'' || str.front() == '"')
  {
    postprocess(str);
    if (str.front() == '"')
      return e->newString(StringBackend::convert(std::string(str.begin() + 1, str.end() - 1)));
    if (str.size() != 3)
      throw InvalidCharacterLiteral{};
    return e->newChar(str.at(1));
  }
  else if (str.find('.') != str.npos || str.find('e') != str.npos)
  {
    double dval = std::stod(str);
    return e->newDouble(dval);
  }

  int ival = std::stoi(str);
  return e->newInt(ival);
}

int LiteralProcessor::generate(const std::shared_ptr<ast::IntegerLiteral> & il)
{
  std::string i = il->toString();
  if (i.find("0x") == 0)
    return std::stoi(i.substr(2), nullptr, 16);
  else if (i.find("0b") == 0)
    return std::stoi(i.substr(2), nullptr, 2);
  else if (i.find("0") == 0)
    return std::stoi(i, nullptr, 8);
  return std::stoi(i, nullptr, 10);
}

std::string LiteralProcessor::take_suffix(std::string & str)
{
  auto it = str.end() - 1;
  while (parser::Lexer::isLetter(*it) || *it == '_')
    --it;
  ++it;
  std::string suffix = std::string(it, str.end());
  str.erase(it, str.end());
  return suffix;
}

void LiteralProcessor::postprocess(std::string & sl)
{
  auto read = sl.begin();
  auto write = sl.begin();

  while (read != sl.end())
  {
    if (*read != '\\') {
      *write = *read;
      ++read;
      ++write;
      continue;
    }

    auto next = read + 1;
    
    if (next == sl.end())
      return; /// TODO : should we throw ?

    switch (*next)
    {
    case '\\':
      *write = '\\';
      break;
    case 'n':
      *write = '\n';
      break;
    case 't':
      *write = '\t';
      break;
    case 'r':
      *write = '\r';
      break;
    case '0':
      *write = '\0';
      break;
    default:
      throw NotImplemented{ "Invalid escaped character" };
    }

    ++write;
    ++read; ++read;
  }

  sl.resize(std::distance(sl.begin(), write));
}

} // namespace compiler

} // namespace script

