// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_LITERAL_PROCESSOR_H
#define LIBSCRIPT_LITERAL_PROCESSOR_H

#include "script/value.h"

#include "script/ast/forwards.h"

namespace script
{

namespace compiler
{

class LIBSCRIPT_API LiteralProcessor
{
public:

  static Value generate(Engine *e, const std::shared_ptr<ast::Literal> & l);
  static Value generate(Engine *e, std::string & str);

 // static bool generate(const std::shared_ptr<ast::BoolLiteral> & bl);
  static int generate(const std::shared_ptr<ast::IntegerLiteral> & il);
 // static std::string generate(const std::shared_ptr<ast::StringLiteral> & sl);

  static std::string take_suffix(std::string & str);

  static void postprocess(std::string & sl);
};

} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_LITERAL_PROCESSOR_H
