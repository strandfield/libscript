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


static void BM_For_Loop_Operations(benchmark::State &state) {
  using namespace script;

  const char *src =
    "  int a = 0;                      "
    "  int b = 0;                      "
    "  int c = 0;                      "
    "  int d = 1;                      "
    "  for(int i = 0; i < 1000; ++i)   "
    "  {                               "
    "    a = b * c + d;                "
    "    b = a + 1;                    "
    "    c = b - 1;                    "
    "    d = a / 2;                    "
    "  }                               ";

  Engine e;
  e.setup();
  Script s = e.newScript(SourceFile::fromString(src));
  s.compile();

  while (state.KeepRunning()) {
    s.run();
  }
}

BENCHMARK(BM_For_Loop_Operations);


static void BM_For_Loop_Calls(benchmark::State &state) {
  using namespace script;

  const char *src =
    "  int incr(int n) { return n+1; }         "
    "  int add(const int &a, const int &b)     " 
    "  { return a + b; }                       "
    "  int sub(const int &a, const int &b)     "
    "  { return a - b; }                       "
    "                                          "
    "  int a = 0;                              "
    "  int b = 0;                              "
    "  for(int i = 0; i < 1000; ++i)           "
    "  {                                       "
    "    a = b;                                "
    "    b = add(incr(a), sub(incr(a), 1));    "
    "  }                                       ";

  Engine e;
  e.setup();
  Script s = e.newScript(SourceFile::fromString(src));
  s.compile();

  while (state.KeepRunning()) {
    s.run();
  }
}

BENCHMARK(BM_For_Loop_Calls);


static void BM_NewFundamentals(benchmark::State &state) {
  using namespace script;

  Engine e;
  e.setup();

  std::vector<Value> buffer{ 512, Value{} };

  while (state.KeepRunning()) {
    
    for (int i(0); i < (512 >> 2); ++i)
    {
      Value a = e.newInt(0);
      Value b = e.newFloat(1.f);
      Value c = e.newBool(true);
      Value d = e.newDouble(3.14);

      buffer[i * 4] = a;
      buffer[i * 4 + 1] = b;
      buffer[i * 4 + 2] = c;
      buffer[i * 4 + 3] = d;
    }

    for (int i(0); i < 512; ++i)
      e.destroy(buffer[i]);
  }
}

BENCHMARK(BM_NewFundamentals);


BENCHMARK_MAIN();