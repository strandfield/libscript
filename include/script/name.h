// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_NAME_H
#define LIBSCRIPT_NAME_H

#include "script/operators.h"
#include "script/symbol-kind.h"
#include "script/types.h"

namespace script
{

/*!
 * \class Name
 * \brief universal class for naming symbols
 * 
 * This class can be used to store symbols name as 
 * a string, an operator name or a type.
 */
class LIBSCRIPT_API Name
{
public:
  Name();
  Name(const Name& other);
  Name(Name&& other) noexcept;
  ~Name();

  Name(script::OperatorName op);
  Name(SymbolKind k, const std::string& str);
  Name(SymbolKind k, const Type& t);

  SymbolKind kind() const;

  const std::string& string() const;
  script::OperatorName operatorName() const;
  Type type() const;

  Name& operator=(const Name& other);
  Name& operator=(Name&& other) noexcept;

  friend LIBSCRIPT_API bool operator==(const Name & lhs, const Name & rhs);

private:
  bool holdsString() const;

private:
  SymbolKind kind_;
  union Storage {
    Storage();
    ~Storage();
    std::string string;
    script::OperatorName operation;
    Type type;
  };
  Storage data_;
};

/*!
 * \fn SymbolKind kind() const
 * \brief returns the kind of symbol that this name names
 */
inline SymbolKind Name::kind() const 
{ 
  return kind_; 
}

/*!
 * \fn const std::string& string() const
 * \brief returns the name stored as a string
 */
inline const std::string& Name::string() const 
{ 
  return data_.string; 
}

/*!
 * \fn script::OperatorName operatorName() const 
 * \brief returns the name of the operator
 */
inline script::OperatorName Name::operatorName() const 
{ 
  return data_.operation; 
}

/*!
 * \fn Type type() const 
 * \brief returns the type associated with this name
 */
inline Type Name::type() const 
{ 
  return data_.type; 
}

/*!
 * \endlcass
 */

LIBSCRIPT_API bool operator==(const Name & lhs, const Name & rhs);
inline bool operator!=(const Name & lhs, const Name & rhs) { return !(lhs == rhs); }

} // namespace script

#endif // LIBSCRIPT_NAME_H
