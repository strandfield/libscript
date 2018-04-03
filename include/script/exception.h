// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_EXCEPTION_H
#define LIBSCRIPT_EXCEPTION_H

namespace script
{

class Exception
{
public:
  Exception() = default;
  Exception(const Exception &) = default;
  ~Exception() = default;
};

} // namespace script

#endif // LIBSCRIPT_EXCEPTION_H
