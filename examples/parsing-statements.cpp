// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/parser/parser.h"

#include <iostream>

void pretty_print(const script::ast::Node& node, std::string indent = "", bool islast = true)
{
  std::cout << indent;

  if (islast)
  {
    std::cout << "\\- ";
    indent += "  ";
  }
  else
  {
    std::cout << "|- ";
    indent += "| ";
  }

  using namespace script;
  using script::ast::NodeType;

  switch (node.type())
  {
  case NodeType::CompoundStatement:
  {
    std::cout << "{}" << std::endl;

    const auto& stmts = node.as<ast::CompoundStatement>();

    for (size_t i(0); i < stmts.statements.size(); ++i)
    {
      pretty_print(*stmts.statements.at(i), indent, i == stmts.statements.size() - 1);
    }
  }
  break;
  case NodeType::WhileLoop:
  {
    std::cout << "while" << std::endl;

    const auto& whle = node.as<ast::WhileLoop>();

    pretty_print(*whle.condition, indent, false);
    pretty_print(*whle.body, indent, true);
  }
  break;
  default:
    std::cout << node.base_token().toString() << " (unexposed)" << std::endl;
    break;
  }
}

int main()
{
  using namespace script;

  const char* src = 
    "int n = 0;\n"
    "while(true) { return false; } \n"
    "class A { }; \n";

  parser::Parser p{ src };

  std::cout << "Parser is working on " << p.tokens().size() << " tokens." << std::endl;

  while (!p.atEnd())
  {
    auto stmt = p.parse();
   
    pretty_print(*stmt);
    std::cout << "---" << std::endl;
  }
}
