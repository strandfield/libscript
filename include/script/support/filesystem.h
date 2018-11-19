// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SUPPORT_FILESYSTEM_H
#define LIBSCRIPT_SUPPORT_FILESYSTEM_H

#if __cplusplus >= 201703L
#include <filesystem>
#elif !defined(LIBSCRIPT_USE_FILESYSTEM_SUPPORT_LIB)
#include <experimental/filesystem>
#else
#include <memory>
#include <string>
#endif // __cplusplus >= 201703L

namespace script
{

namespace support
{

#if __cplusplus >= 201703L
namespace filesystem = std::filesystem;
#elif !defined(LIBSCRIPT_USE_FILESYSTEM_SUPPORT_LIB)
namespace filesystem = std::experimental::filesystem;
#else

namespace filesystem
{

struct path
{
  path() = default;
  path(const path &) = default;

  path(const std::string & str)
    : path_(str)
  {

  }

  std::string extension() const;

  inline const std::string & string() const { return path_; }

  path & operator/=(const std::string & str);

  path & operator+=(const std::string & str)
  {
    path_ += str;
    return *this;
  }

#if defined(_WIN32) || defined(WIN32)
  typedef wchar_t value_type;
#else
  typedef char value_type;
#endif
  static const value_type preferred_separator;

  std::string path_;
};

path operator/(const path & lhs, const path & rhs);
bool operator==(const path & lhs, const path & rhs);

path current_path();
bool exists(const path & p);
bool is_directory(const path & p);

struct directory_entry
{
  directory_entry() = default;

  directory_entry(const path & p)
    : path_(p)
  {

  }

  inline operator const path &() const { return path_; }

  path path_;
};

struct directory_iterator_impl;

struct directory_iterator
{
  directory_iterator();
  directory_iterator(const directory_iterator & other);
  directory_iterator(const path & p);
  ~directory_iterator();

  directory_iterator& operator++();

  const directory_entry& operator*() const { return current_; }
  const directory_entry* operator->() const { return &current_; }

  path directory_;
  directory_entry current_;
  std::unique_ptr<directory_iterator_impl> impl_;
};

bool operator!=(const directory_iterator & lhs, const directory_iterator & rhs);

inline directory_iterator begin(directory_iterator iter) { return iter; }
inline directory_iterator end(const directory_iterator &) { return directory_iterator{}; }

} // namespace filsystem

#endif // __cplusplus >= 201703L

} // namespace support

} // namespace script

#endif // LIBSCRIPT_SUPPORT_FILESYSTEM_H
