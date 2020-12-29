// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/ast.h"
#include "script/ast/visitor.h"

#include <iostream>

class AstVisitorZeroCounter : public script::ast::AstVisitor
{
public:

  int n = 0;

  void visit(What w, script::ast::NodeRef n) override
  {
    recurse(n);
  }

  void visit(What w, script::parser::Token tok) override
  {
    if (tok.text().size() == 1 && tok.text().at(0) == '0')
      ++n;
  }
};

int main()
{
  using namespace script;

  const char* src = 
    "int n = 0;\n"
    "int main() { return 0; } \n"
    "class A { virtual void work() = 0; }; \n";

  script::SourceFile srcfile = script::SourceFile::fromString(src);

  script::Ast syntaxtree = script::ast::parse(srcfile);

  AstVisitorZeroCounter visitor;
  script::ast::visit(visitor, syntaxtree.root());

  std::cout << "I found " << visitor.n << " zeros within the ast." << std::endl;

  return 0;
}
