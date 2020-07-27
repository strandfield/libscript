// Copyright (C) 2020 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/parser/parser.h"
#include "script/parser/lexer.h"

#include <iostream>

void recursive_print(script::parser::TokenReader reader, int depth = 0)
{
  while (!reader.atEnd())
  {
    script::parser::Token tok = reader.peek();

    std::cout << std::string(static_cast<size_t>(2 * depth), ' ');
    std::cout << tok.text().toString() << std::endl;

    if (tok == script::parser::Token::LeftPar)
    {
      script::parser::TokenReader subreader = reader.subfragment<script::parser::Fragment::DelimiterPair>();
      recursive_print(subreader, depth + 1);
      reader.seek(subreader.end());
    }
    else
    {
      reader.read();
    }
  }
}

int main()
{
  const char* src = "int n = (1+ (2+3) );";

  std::vector<script::parser::Token> tokens = script::parser::tokenize(src);

  script::parser::TokenReader reader{ src, tokens };

  recursive_print(reader);
}
