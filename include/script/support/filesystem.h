// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SUPPORT_FILESYSTEM_H
#define LIBSCRIPT_SUPPORT_FILESYSTEM_H

#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <experimental/filesystem>
#endif // __cplusplus >= 201703L

namespace script
{

namespace support
{

#if __cplusplus >= 201703L
namespace filesystem = std::filesystem;
#else
namespace filesystem = std::experimental::filesystem;
#endif // __cplusplus >= 201703L

} // namespace support

} // namespace script

#endif // LIBSCRIPT_SUPPORT_FILESYSTEM_H
