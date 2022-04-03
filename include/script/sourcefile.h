// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SOURCE_FILE_H
#define LIBSCRIPT_SOURCE_FILE_H

#include "libscriptdefs.h"

namespace script {

struct SourceFileImpl;

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

class LIBSCRIPT_API SourceFile
{
public:

  /*!
   * \fn SourceFile()
   * \brief Null-constructs a source file.
   *
   * The only thing you can do safely on a null source file is to
   * test for nullity.
   * \sa isNull.
   */
  SourceFile() = default;
  
  /*!
   * \fn SourceFile(const SourceFile &)
   * \brief Copy constructor
   *
   * Constructs a new reference to the same source file.
   */
  SourceFile(const SourceFile &) = default;

  ~SourceFile() = default;
  
  explicit SourceFile(const std::string& path);
  explicit SourceFile(const std::shared_ptr<SourceFileImpl> & impl);

  inline bool isNull() const { return d == nullptr; }

  typedef size_t Offset;

  struct Position {
    Offset pos;
    uint16 line;
    uint16 col;
  };

  const std::string& filepath() const;

  Position map(Offset off) const;

  void load();
  bool isLoaded() const;
  bool isLocked() const;
  void unload();

  const char* data() const;
  const std::string& content() const;

  static SourceFile fromString(const std::string& src);

  inline const std::shared_ptr<SourceFileImpl> & impl() const { return d; }

private:
  std::shared_ptr<SourceFileImpl> d;
};

/*!
 * \endclass
 */

struct LIBSCRIPT_API SourceLocation
{
  SourceFile m_source;
  SourceFile::Position m_pos;
};

} // namespace script

#endif // LIBSCRIPT_SOURCE_FILE_H
