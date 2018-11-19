// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/support/filesystem.h"

#if defined(LIBSCRIPT_USE_FILESYSTEM_SUPPORT_LIB)

#ifndef WIN32
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <windows.h>
#endif


namespace script
{

namespace support
{

namespace filesystem
{


#if defined(_WIN32) || defined(WIN32)
const path::value_type path::preferred_separator = '\\';
#else
const path::value_type path::preferred_separator = '/';
#endif

std::string path::extension() const
{
  auto p = path_.find_last_of('.');
  if (p == std::string::npos)
    return std::string{};
  return path_.substr(p);
}

path & path::operator/=(const std::string & str)
{
  path_ += std::string{ (char)preferred_separator } + str;
  return *this;
}

path operator/(const path & lhs, const path & rhs)
{
  path result = lhs;
  result /= rhs.string();
  return result;
}

bool operator==(const path & lhs, const path & rhs)
{
  return lhs.string() == rhs.string();
}

path current_path()
{
#if defined(WIN32) || defined(_WIN32)

  char buffer[256] = { '\0' };
  size_t size = GetCurrentDirectoryA(256, buffer);
  return path{ std::string{buffer, size} };

#else

  char buffer[256] = { '\0' };
  getcwd(buffer, 256);
  return path{ std::string{ buffer } };

#endif
}

bool exists(const path & p)
{
#if defined(WIN32) || defined(_WIN32)

  DWORD fa = GetFileAttributesA(p.path_.c_str());
  if (fa == INVALID_FILE_ATTRIBUTES)
    return false;
  return true;

#else

  struct stat info;

  if (stat(p.path_.c_str(), &info) != 0)
    return false;

  if ((info.st_mode & S_IFDIR) || (info.st_mode & S_IFREG))
    return true;

  return false;

#endif
}

bool is_directory(const path & p)
{
#if defined(WIN32) || defined(_WIN32)

  DWORD fa = GetFileAttributesA(p.path_.c_str());
  if (fa == INVALID_FILE_ATTRIBUTES)
    return false;
  return (fa & FILE_ATTRIBUTE_DIRECTORY);

#else

  struct stat info;

  if (stat(p.path_.c_str(), &info) != 0)
    return false;

  if ((info.st_mode & S_IFDIR))
    return true;

  return false;

#endif
}

#if defined(WIN32) || defined(_WIN32)

struct directory_iterator_impl
{
  HANDLE handle;
  WIN32_FIND_DATAA find_data;
};

#else

struct directory_iterator_impl
{
  DIR *dir;
  size_t pos;
};

#endif

directory_iterator::directory_iterator()
{

}

directory_iterator::directory_iterator(const directory_iterator & other)
  : directory_(other.directory_)
  , current_(other.current_)
  , impl_(std::unique_ptr<directory_iterator_impl>(new directory_iterator_impl{*other.impl_}))
{
#if defined(WIN32) || defined(_WIN32)

  impl_->handle = FindFirstFileA(directory_.string().c_str(), &(impl_->find_data));
  impl_->find_data = other.impl_->find_data;

#else

  impl_->dir = opendir(directory_.string().c_str());
  seekdir(impl_->dir, impl_->pos);

#endif
}

directory_iterator::directory_iterator(const path & p)
  : directory_(p)
  , impl_(std::unique_ptr<directory_iterator_impl>(new directory_iterator_impl{ }))
{
#if defined(WIN32) || defined(_WIN32)

  impl_->handle = FindFirstFileA(directory_.string().c_str(), &(impl_->find_data));
  current_ = directory_entry{ directory_ / path{impl_->find_data.cFileName} };

#else

  impl_->dir = opendir(directory_.string().c_str());
  auto *entry = readdir(impl_->dir);
  if (entry != nullptr)
  {
    current_ = directory_entry(directory_ / path{ entry->d_name });
    impl_->pos = telldir(impl_->dir);
  }

#endif
}

directory_iterator::~directory_iterator()
{
#if defined(WIN32) || defined(_WIN32)

  FindClose(impl_->handle);

#else

  closedir(impl_->dir);

#endif
}

directory_iterator& directory_iterator::operator++()
{
#if defined(WIN32) || defined(_WIN32)

  bool result = FindNextFileA(impl_->handle, &(impl_->find_data));
  if (result)
  {
    current_ = directory_entry{ directory_ / path{ impl_->find_data.cFileName } };
  }
  else
  {
    current_ = directory_entry{};
  }

#else

  auto *entry = readdir(impl_->dir);
  if (entry == nullptr)
  {
    current_ = directory_entry{};
  }
  else
  {
    current_ = directory_entry(directory_ / path{ entry->d_name });
    impl_->pos = telldir(impl_->dir);
  }

#endif

  return *this;
}

bool operator!=(const directory_iterator & lhs, const directory_iterator & rhs)
{
  return lhs.current_.path_.string() != rhs.current_.path_.string();
}

} // namespace filsystem

} // namespace support

} // namespace script

#endif // LIBSCRIPT_USE_FILESYSTEM_SUPPORT_LIB
