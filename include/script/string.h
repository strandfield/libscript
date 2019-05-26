// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_STRING_H
#define LIBSCRIPT_STRING_H

#include "libscriptdefs.h"

#define LIBSCRIPT_USE_BUILTIN_STRING_BACKEND

#include <string>

namespace script
{

class Class;

/*!
 * \class StringBackend
 * \brief Provides string-related functionnalities to the library
 * 
 * This class provides everything the Engine needs to create the 
 * String type as exposed in the scripting language.
 */

class StringBackend
{
public:
  /*!
   * \typedef string_type
   * \brief Typedef for the C++ string type that is going to be used in the scripting language.
   */
  typedef std::string string_type;

public:
  /*!
   * \fn static const string_type& convert(const std::string& str)
   * \brief Provides conversion from std::string to \t string_type.
   */
  static const string_type& convert(const std::string& str) { return str; }

  /*!
   * \fn static std::string class_name()
   * \brief Returns the name of the string type as used in the scripting language.
   */
  static std::string class_name() { return "String"; }

  /*!
   * \fn static void register_string_type(Class& string)
   * \brief Callbacks used by the engine to fill the string type.
   */
  static void register_string_type(Class& string);
};

using String = StringBackend::string_type;

} // namespace script

#endif // LIBSCRIPT_STRING_H
