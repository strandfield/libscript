// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <benchmark/benchmark.h>

#include "script/engine.h"
#include "script/script.h"

static void BM_Engine_Setup(benchmark::State &state) {
  using namespace script;

  while (state.KeepRunning()) {
    Engine e;
    e.setup();
  }
}

BENCHMARK(BM_Engine_Setup);


static void BM_Compile_Assignment(benchmark::State &state) {
  using namespace script;

  const char *src =
    "int a = 2; a = a + 1; ";

  Engine e;
  e.setup();

  while (state.KeepRunning()) {
    Script s = e.newScript(SourceFile::fromString(src));
    s.compile();
  }
}

BENCHMARK(BM_Compile_Assignment);


static void BM_For_Loop(benchmark::State &state) {
  using namespace script;

  const char *src =
    "  int n = 0;                      "
    "  for(int i = 0; i < 10000; ++i)  "
    "  {                               "
    "    n = n + i;                    "
    "  }                               ";

  Engine e;
  e.setup();
  Script s = e.newScript(SourceFile::fromString(src));
  s.compile();

  while (state.KeepRunning()) {
    s.run();
  }
}

BENCHMARK(BM_For_Loop);



BENCHMARK_MAIN();