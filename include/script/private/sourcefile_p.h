// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SOURCE_FILE_P_H
#define LIBSCRIPT_SOURCE_FILE_P_H

#include "libscriptdefs.h"

namespace script {

struct SourceFileImpl
{
  std::string filepath;
  std::string content;
  bool open;

public:
  SourceFileImpl(const std::string & path);
  ~SourceFileImpl() = default;
};

} // namespace script

#endif // LIBSCRIPT_SOURCE_FILE_P_H
