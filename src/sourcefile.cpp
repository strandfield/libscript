// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/sourcefile.h"
#include "script/private/sourcefile_p.h"

#include <fstream>
#include <limits>
#include <sstream>

namespace script
{

SourceFileImpl::SourceFileImpl(const std::string & path)
  : filepath(path)
  , open(false)
  , lock(false)
{

}

/*!
 * \class SourceFile
 * \brief Represents a source file.
 *
 * The SourceFile class represents a source file for a \t Script.
 * The actual source can currently be either a file on the local file system 
 * or an in-memory string (see \m fromString method).
 *
 * The scripting engine assumes that source files are encoded in UTF-8, 
 * and thus uses a \t{std::string} as the underlying storage type.
 *
 * If the input is a local file, it is not loaded until \m load is called.
 * After a call to \m load, the entire file is stored in memory.
 * Memory can be released by calling \m unload unless the source file 
 * \m isLocked, meaning that the system needs the source to be available
 * (this is the case, for example, if your script contains templates).
 *
 * The SourceFile class is an implicitely shared class: it can be null constructed 
 * and copy and assignment do not create true copies but rather new reference to the 
 * same object.
 */

/*!
 * \fn SourceFile()
 * \brief Null-constructs a source file.
 *
 * The only thing you can do safely on a null source file is to 
 * test for nullity.
 * \sa isNull.
 */

/*!
 * \fn SourceFile(const SourceFile &)
 * \brief Copy constructor
 *
 * Constructs a new reference to the same source file.
 */

/*!
 * \fn SourceFile(const std::string & path)
 * \param path on the local file system
 * \brief Constructs a sourcefile.
 *
 */
SourceFile::SourceFile(const std::string & path)
  : d(std::make_shared<SourceFileImpl>(path))
{

}

/*!
 * \fn SourceFile(const std::shared_ptr<SourceFileImpl> & impl)
 * \param implementation
 * \brief Constructs a sourcefile from its implementation).
 *
 */
SourceFile::SourceFile(const std::shared_ptr<SourceFileImpl> & impl)
  : d(impl)
{

}

/*!
 * \fn bool isNull() const
 * \brief Returns whether this source file is null.
 */

/*!
 * \fn const std::string & filepath() const
 * \brief Returns the filepath of this sourcefile.
 *
 * If the source file was constructed using \m fromString, this returns an 
 * empty string.
 */
const std::string & SourceFile::filepath() const
{
  return d->filepath;
}

/*!
 * \fn SourceFile::Position SourceFile::map(Offset off) const
 * \param an offset in the file
 * \brief maps an offset to a (line, column)
 */
SourceFile::Position SourceFile::map(Offset off) const
{
  Position result;
  result.pos = off;

  if (off <= 0)
  {
    result.line = 0;
    result.col = 0;
    return result;
  }

  size_t p = off;

  while (p-- > 0 && d->content[p] != '\n');

  if (p == std::numeric_limits<size_t>::max())
  {
    result.line = 0;
    result.col = static_cast<decltype(result.col)>(off);
    return result;
  }
  else
  {
    result.col = static_cast<decltype(result.col)>(off - 1 - p);
  }

  result.line = 0;

  for (size_t i(0); i <= p; ++i)
  {
    if (d->content[i] == '\n')
      result.line += 1;
  }

  return result;
}

/*!
 * \fn void load()
 * \brief Loads the source file.
 *
 * This does nothing if the sourcefile is already loaded or if it was 
 * created from an in-memory string.
 * Throws std::runtime_error on failure.
 */
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

/*!
 * \fn bool isLoaded() const
 * \brief Returns whether the sourcefile is loaded.
 *
 */
bool SourceFile::isLoaded() const
{
  return d->open;
}

/*!
 * \fn bool isLocked() const
 * \brief Returns whether the sourcefile is locked.
 *
 * A source file is locked by the system if it needs the source to be 
 * available later after a script has been compiled.
 */
bool SourceFile::isLocked() const
{
  return d->lock;
}

/*!
 * \fn void unload()
 * \brief Unloads the source file.
 *
 * This does nothing if the source file is locked.
 * \sa isLocked.
 */
void SourceFile::unload()
{
  if (d->lock)
    return;

  d->content = std::string{};
  d->open = false;
}

/*!
 * \fn const char * data() const
 * \brief Returns a pointer to the source file content.
 *
 */
const char * SourceFile::data() const
{
  return d->content.data();
}

/*!
 * \fn const std::string & content() const
 * \brief Returns the source file content.
 *
 * If the source file is not loaded, the string is empty.
 */
const std::string & SourceFile::content() const
{
  return d->content;
}

/*!
 * \fn static SourceFile fromString(const std::string & src)
 * \param source file content
 * \brief Creates a source file from an in-memory string.
 *
 * Such source file is automatically considered to be loaded.
 * Unloading such source file is possible, but it cannot be reloaded afterwards.
 */
SourceFile SourceFile::fromString(const std::string & src)
{
  auto impl = std::make_shared<SourceFileImpl>(std::string{});
  impl->content = src;
  impl->open = true;
  return SourceFile{ impl };
}

} // namespace script
