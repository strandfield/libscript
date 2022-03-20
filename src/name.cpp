// Copyright (C) 2018-2022 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/name.h"

namespace script
{

/*!
 * \class Name
 */

Name::Storage::Storage()
{

}

Name::Storage::~Storage()
{

}

Name::Name()
  : kind_(SymbolKind::NotASymbol)
{

}

Name::Name(const Name & other)
  : kind_(other.kind())
{
  switch (kind_)
  {
  case SymbolKind::NotASymbol:
    break;
  case SymbolKind::Namespace:
  case SymbolKind::Class:
  case SymbolKind::Function:
  case SymbolKind::LiteralOperator:
  case SymbolKind::Template:
    new (&data_.string) std::string{ other.data_.string };
    break;
  case SymbolKind::Operator:
    data_.operation = other.data_.operation;
    break;
  case SymbolKind::Cast:
  case SymbolKind::Constructor:
  case SymbolKind::Destructor:
    data_.type = other.data_.type;
    break;
  }
}

Name::Name(Name&& other) noexcept
  : kind_(other.kind())
{
  switch (kind_)
  {
  case SymbolKind::NotASymbol:
    break;
  case SymbolKind::Namespace:
  case SymbolKind::Class:
  case SymbolKind::Function:
  case SymbolKind::LiteralOperator:
  case SymbolKind::Template:
    new (&data_.string) std::string{ std::move(other.data_.string) };
    other.data_.string.~basic_string();
    break;
  case SymbolKind::Operator:
    data_.operation = other.data_.operation;
    break;
  case SymbolKind::Cast:
  case SymbolKind::Constructor:
  case SymbolKind::Destructor:
    data_.type = other.data_.type;
    break;
  }

  other.kind_ = SymbolKind::NotASymbol;
}

Name::~Name()
{
  switch (kind_)
  {
  case SymbolKind::Namespace:
  case SymbolKind::Class:
  case SymbolKind::Function:
  case SymbolKind::LiteralOperator:
  case SymbolKind::Template:
    data_.string.~basic_string();
    break;
  default:
    break;
  }

  kind_ = SymbolKind::NotASymbol;
}

Name::Name(script::OperatorName op)
  : kind_(SymbolKind::Operator)
{
  data_.operation = op;
}


Name::Name(SymbolKind k, const std::string& str)
  : kind_(k)
{
  new (&data_.string) std::string{ str };
}

Name::Name(SymbolKind k, const Type & t)
  : kind_(k)
{
  data_.type = t;
}

bool Name::holdsString() const
{
  switch (kind_)
  {
  case SymbolKind::Namespace:
  case SymbolKind::Class:
  case SymbolKind::Function:
  case SymbolKind::LiteralOperator:
  case SymbolKind::Template:
    return true;
  default:
    return false;
  }
}

Name & Name::operator=(const Name& other)
{
  if (holdsString())
  {
    if (other.holdsString())
    {
      data_.string = other.data_.string;
    }
    else
    {
      data_.string.~basic_string();

      if (other.kind_ == SymbolKind::Operator)
        data_.operation = other.data_.operation;
      else
        data_.type = other.data_.type;
    }
  }
  else
  {
    if (other.holdsString())
    {
      new (&data_.string) std::string{ other.data_.string };
    }
    else
    {
      if (other.kind_ == SymbolKind::Operator)
        data_.operation = other.data_.operation;
      else
        data_.type = other.data_.type;
    }
  }

  kind_ = other.kind_;

  return (*this);
}

Name & Name::operator=(Name&& other) noexcept
{
  if (holdsString())
  {
    if (other.holdsString())
    {
      data_.string = std::move(other.data_.string);
      other.data_.string.~basic_string();
    }
    else
    {
      data_.string.~basic_string();

      if (other.kind_ == SymbolKind::Operator)
        data_.operation = other.data_.operation;
      else
        data_.type = other.data_.type;
    }
  }
  else
  {
    if (other.holdsString())
    {
      new (&data_.string) std::string{ std::move(other.data_.string) };
      other.data_.string.~basic_string();
    }
    else
    {
      if (other.kind_ == SymbolKind::Operator)
        data_.operation = other.data_.operation;
      else
        data_.type = other.data_.type;
    }
  }

  kind_ = other.kind_;
  other.kind_ = SymbolKind::NotASymbol;

  return (*this);
}

bool operator==(const Name & lhs, const Name & rhs)
{
  if (lhs.kind() != rhs.kind())
    return false;

  if (lhs.kind() == SymbolKind::NotASymbol)
    return true;

  if(lhs.holdsString())
    return lhs.data_.string == rhs.data_.string;
  else if (lhs.kind() == SymbolKind::Operator)
    return lhs.data_.operation == rhs.data_.operation;
  else 
    return lhs.data_.type == rhs.data_.type;
}


/*!
 * \endclass
 */

} // namespace script
