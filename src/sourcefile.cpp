// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/sourcefile.h"
#include "sourcefile_p.h"

#include <fstream>
#include <sstream>

namespace script
{

SourceFileImpl::SourceFileImpl(const std::string & path)
  : filepath(path)
  , open(false)
{

}

SourceFile::SourceFile(const std::string & path)
  : d(std::make_shared<SourceFileImpl>(path))
{

}

SourceFile::SourceFile(const std::shared_ptr<SourceFileImpl> & impl)
  : d(impl)
{

}

const std::string & SourceFile::filepath() const
{
  return d->filepath;
}

void SourceFile::load()
{
  if (isLoaded())
    return;

  if (d->filepath.empty())
    throw std::runtime_error{ "SourceFile not associated with a local file" };

  std::ifstream file{ d->filepath };
  if (!file.is_open())
    throw std::runtime_error{ "Could not open file ..." };

  std::stringstream stream;
  stream << file.rdbuf();
  d->content = stream.str();
  file.close();
}

bool SourceFile::isLoaded() const
{
  return d->open;
}

void SourceFile::unload()
{
  d->content = std::string{};
  d->open = false;
}

const char * SourceFile::data() const
{
  return d->content.data();
}

const std::string & SourceFile::content() const
{
  return d->content;
}

SourceFile SourceFile::fromString(const std::string & src)
{
  auto impl = std::make_shared<SourceFileImpl>(std::string{});
  impl->content = src;
  impl->open = true;
  return SourceFile{ impl };
}

} // namespace script
