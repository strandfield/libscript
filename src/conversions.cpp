// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/conversions.h"

#include "script/engine.h"
#include "script/enumerator.h"
#include "script/cast.h"
#include "script/class.h"
#include "script/typesystem.h"

#include "script/program/expression.h"

#include <stdexcept>

namespace script
{

/*!
 * \class StandardConversion
 * \brief The StandardConversion class represents a builtin conversion.
 *
 * Standard conversions are one of the building blocks of \t{Conversion}s.
 * 
 * A standard conversion can represent the following operations:
 * \begin{list}
 *   \li a copy
 *   \li conversions between fundamental types
 *   \li derived-to-base conversions
 *   \li enum to int conversions
 * \end{list}
 * 
 * Additionally, a standard conversion can hold two flags for:
 * \begin{list}
 *   \li conversion from non-\c{const} to \c{const}
 *   \li reference conversions
 * \end{list}
 * 
 */

static const int stdconv_table[] = {
               /* bool */ /* char */ /* int */ /* float */ /* double */
  /* bool */         0,         2,        3,          4,           5,
  /* char */         6,         0,        8,          9,          10,
  /* int */         11,        12,        0,         14,          15,
  /* float */       16,        17,       18,          0,          20,
  /* double */      21,        22,       23,         24,           0,
};

static const Type::BuiltInType stdconv_srctype_table[] = {
  Type::Auto,
  Type::Boolean, 
  Type::Boolean,
  Type::Boolean,
  Type::Boolean,
  Type::Boolean,
  Type::Char,
  Type::Char,
  Type::Char,
  Type::Char,
  Type::Char,
  Type::Int,
  Type::Int,
  Type::Int,
  Type::Int,
  Type::Int,
  Type::Float,
  Type::Float,
  Type::Float,
  Type::Float,
  Type::Float,
  Type::Double,
  Type::Double,
  Type::Double,
  Type::Double,
  Type::Double,
  Type::Auto, // enum to int
  Type::Auto, // derived to base
  Type::Null, // not convertible
};

static const Type::BuiltInType stdconv_desttype_table[] = {
  Type::Auto,
  Type::Boolean,
  Type::Char,
  Type::Int,
  Type::Float,
  Type::Double,
  Type::Boolean,
  Type::Char,
  Type::Int,
  Type::Float,
  Type::Double,
  Type::Boolean,
  Type::Char,
  Type::Int,
  Type::Float,
  Type::Double,
  Type::Boolean,
  Type::Char,
  Type::Int,
  Type::Float,
  Type::Double,
  Type::Boolean,
  Type::Char,
  Type::Int,
  Type::Float,
  Type::Double,
  Type::Int, // enum to int
  Type::Auto, // derived to base
  Type::Null, // not convertible
};

static const int conversion_categories[] = {
  0, // copy
                                     0,   NumericPromotion::IntegralPromotion,   NumericPromotion::IntegralPromotion,   NumericPromotion::FloatingPointPromotion, NumericPromotion::FloatingPointPromotion,
  NumericConversion::BooleanConversion,                                     0,   NumericPromotion::IntegralPromotion,   NumericPromotion::FloatingPointPromotion, NumericPromotion::FloatingPointPromotion,
  NumericConversion::BooleanConversion, NumericConversion::IntegralConversion,                                     0,   NumericPromotion::FloatingPointPromotion, NumericPromotion::FloatingPointPromotion,
  NumericConversion::BooleanConversion, NumericConversion::IntegralConversion, NumericConversion::IntegralConversion,                                          0, NumericPromotion::FloatingPointPromotion,
  NumericConversion::BooleanConversion, NumericConversion::IntegralConversion, NumericConversion::IntegralConversion, NumericConversion::FloatingPointConversion,                                        0,
  NumericConversion::IntegralConversion, // enum-to-int
  0, // derived-to-base
  0, // not-convertible
};

static const ConversionRank conversion_ranks[] = {
  ConversionRank::ExactMatch, // copy
  ConversionRank::ExactMatch,  ConversionRank::Promotion,  ConversionRank::Promotion,  ConversionRank::Promotion,  ConversionRank::Promotion,
  ConversionRank::Conversion, ConversionRank::ExactMatch,  ConversionRank::Promotion,  ConversionRank::Promotion,  ConversionRank::Promotion,
  ConversionRank::Conversion, ConversionRank::Conversion, ConversionRank::ExactMatch,  ConversionRank::Promotion,  ConversionRank::Promotion,
  ConversionRank::Conversion, ConversionRank::Conversion, ConversionRank::Conversion, ConversionRank::ExactMatch,  ConversionRank::Promotion,
  ConversionRank::Conversion, ConversionRank::Conversion, ConversionRank::Conversion, ConversionRank::Conversion, ConversionRank::ExactMatch,
  ConversionRank::Conversion, // enum-to-int
  ConversionRank::Conversion, // derived-to-base
  ConversionRank::NotConvertible, //not-convertible
};

// 5 bits to store conv between fundamental types and "not convertible"
// 1 bit to store ref-init, 1 bit to store qual adjust
static const int gEnumToIntConversion = 26;
static const int gDerivedToBaseConv = 27;
static const int gNotConvertibleStdConv = 28;
static const int gConstQualAdjustStdConv = (1 << 5);
static const int gRefConvStdConv = (1 << 6);
static const int gDerivedToBaseConvOffset = 8;
static const int gConvIdMask = (1 << 5) - 1;
static const int g8bitsMask = 255;


inline static bool checkNotConvertible(const Type & src, const Type & dest)
{
  return
    src == Type::Void ||
    dest == Type::Void ||
    (dest.isReference() && src.baseType() != dest.baseType()) ||
    (dest.isReference() && src.isConst() && !dest.isConst());
}

/*!
 * \fun StandardConversion()
 * 
 * Constructs a reference-conversion that, when applied to a \t Value, 
 * has no effects.
 */
StandardConversion::StandardConversion()
{
  d = gRefConvStdConv;
}

/*!
 * \fun StandardConversion(const Type & src, const Type & dest)
 * \param a fundamental 'source' type
 * \param a fundamental 'dest' type
 * 
 * Constructs a standard conversion from two fundamental types.
 * The input types can be references and can have different \c const
 * qualifications.
 */
StandardConversion::StandardConversion(const Type & src, const Type & dest)
{
  if (!src.isFundamentalType() || !dest.isFundamentalType())
    throw std::runtime_error{ "Types must be fundamental types" };

  if (checkNotConvertible(src, dest))
  {
    d = gNotConvertibleStdConv;
    return;
  }
  
  d = stdconv_table[(src.baseType().data() - 2)*5 + (dest.baseType().data() - 2)];
  if (dest.isReference())
    d |= gRefConvStdConv;
  if (dest.isConst() && !src.isConst())
    d |= gConstQualAdjustStdConv;
}

/*!
 * \fun StandardConversion(QualificationAdjustment qualadjust)
 * \param a \c const qualification adjustment
 *
 * Constructs a reference-conversion with the given qualification 
 * adjustment.
 */
StandardConversion::StandardConversion(QualificationAdjustment qualadjust)
{
  d = gRefConvStdConv | (gConstQualAdjustStdConv * qualadjust);
}

/*!
 * \fun bool isNone() const
 * 
 * Returns true if this conversion is a reference-conversion 
 * without qualification adjustment (as constructed by the default 
 * constructor).
 */
bool StandardConversion::isNone() const
{
  return d == gRefConvStdConv;
}

StandardConversion StandardConversion::None()
{
  StandardConversion ret;
  ret.d = gRefConvStdConv;
  return ret;
}

/*!
 * \fun bool isNarrowing() const
 *
 * Returns true if this is a narrowing conversion.
 * This is equivalent to \c{isNumericConversion()}.
 */
bool StandardConversion::isNarrowing() const
{
  return isNumericConversion();
}

/*!
 * \fun ConversionRank rank() const
 *
 * Returns the rank of this standard conversion.
 * Note that a standard conversion never has \c UserDefinedConversion
 * rank.
 */
ConversionRank StandardConversion::rank() const
{
  if (isDerivedToBaseConversion())
    return ConversionRank::Conversion;
  else if (d == gNotConvertibleStdConv)
    return ConversionRank::NotConvertible;

  return conversion_ranks[d & gConvIdMask];
}

/*!
 * \fun bool isCopy() const
 *
 * Returns true if this conversion is a copy (that may have a qualification 
 * adjustment).
 */
bool StandardConversion::isCopy() const
{
  return (d & gRefConvStdConv) == 0 && (d & gConvIdMask) == 0;
}

/*!
 * \fun bool isReferenceConversion() const
 *
 * Returns true if this conversion is a reference conversion (that may have 
 * a qualification adjustment).
 */
bool StandardConversion::isReferenceConversion() const
{
  return d & gRefConvStdConv;
}


/*!
 * \fun bool isNumericPromotion() const
 * 
 * Returns true if this is a conversion from a fundamental type to 
 * another fundamental type that is able to represent a wider range 
 * of values (e.g. \c bool to \c int).
 */
bool StandardConversion::isNumericPromotion() const
{
  return numericPromotion() != NumericPromotion::NoNumericPromotion;
}

/*!
 * \fun NumericPromotion numericPromotion() const
 *
 * Returns a description of this conversion as a \t NumericPromotion.
 */
NumericPromotion StandardConversion::numericPromotion() const
{
  return static_cast<NumericPromotion>(conversion_categories[d & gConvIdMask] & (NumericPromotion::IntegralPromotion | NumericPromotion::FloatingPointPromotion));
}

/*!
 * \fun bool isNumericConversion() const
 *
 * Returns true if this is a conversion from a fundamental type 
 * to a smaller fundamental types (i.e. a narrowing conversion).
 */
bool StandardConversion::isNumericConversion() const
{
  return numericConversion() != NumericConversion::NoNumericConversion;
}

/*!
* \fun NumericConversion numericConversion() const
*
* Returns a description of this conversion as a \t NumericConversion.
*/
NumericConversion StandardConversion::numericConversion() const
{
  return static_cast<NumericConversion>(conversion_categories[d & gConvIdMask] & (NumericConversion::IntegralConversion | NumericConversion::FloatingPointConversion | NumericConversion::BooleanConversion));
}

/*!
 * \fun bool hasQualificationAdjustment() const
 *
 * Returns true if this conversion contains a qualifcation adjustment, 
 * i.e. whether this conversion adds a \c const to the converted value.
 */
bool StandardConversion::hasQualificationAdjustment() const
{
  return d & gConstQualAdjustStdConv;
}

/*!
 * \fun bool isDerivedToBaseConversion() const
 *
 * Returns true if this conversion is a derived-to-base conversion.
 */
bool StandardConversion::isDerivedToBaseConversion() const
{
  return (d & gConvIdMask) == gDerivedToBaseConv;
  // Alternative:
  //return ((d >> gDerivedToBaseConvOffset) & g8bitsMask) != 0;
}

/*!
 * \fun int derivedToBaseConversionDepth() const
 *
 * If this is a derived-to-base conversion, returns the number of times 
 * the source type is derived from the base type.
 */
int StandardConversion::derivedToBaseConversionDepth() const
{
  return (d >> gDerivedToBaseConvOffset) & g8bitsMask;
}

/*!
 * \fun Type srcType() const
 *
 * Returns the source type of this conversion.
 * Note that if this conversion is a copy, a reference-conversion 
 * or a derived-to-base conversion, this will return \c{Type::Auto}.
 */
Type StandardConversion::srcType() const
{
  return stdconv_srctype_table[d & gConvIdMask];
}

/*!
* \fun Type destType() const
*
* Returns the dest type of this conversion.
* Note that if this conversion is a copy, a reference-conversion
* or a derived-to-base conversion, this will return \c{Type::Auto}.
*/
Type StandardConversion::destType() const
{
  /// TODO : should we add const-qual and ref-specifiers ?
  return stdconv_desttype_table[d & gConvIdMask];
}

/*!
 * \fun StandardConversion with(QualificationAdjustment adjust) const
 *
 * Returns a copy of this conversion with an additional qualification adjustment.
 * Note that this function cannot be used to remove an existing qualification adjustment.
 */
StandardConversion StandardConversion::with(QualificationAdjustment adjust) const
{
  StandardConversion conv{ *this };
  conv.d |= static_cast<int>(adjust) * gConstQualAdjustStdConv;
  return conv;
}

/*!
 * \fun static StandardConversion Copy()
 *
 * Returns a conversion representing a simple copy, without any 
 * qualification adjustment.
 */
StandardConversion StandardConversion::Copy()
{
  return StandardConversion{ 0 };
}

/*!
 * \fun static  StandardConversion EnumToInt()
 *
 * Returns a conversion representing an enum-to-int conversion.
 */
StandardConversion StandardConversion::EnumToInt()
{
  return StandardConversion{ gEnumToIntConversion };
}

/*!
 * \fun static StandardConversion DerivedToBaseConversion(int depth, bool is_ref_conv, QualificationAdjustment adjust)
 *
 * Returns a conversion representing a derived-to-base conversion with the given \c depth.
 */
StandardConversion StandardConversion::DerivedToBaseConversion(int depth, bool is_ref_conv, QualificationAdjustment adjust)
{
  return StandardConversion{ gDerivedToBaseConv |(depth << gDerivedToBaseConvOffset) | (is_ref_conv ? gRefConvStdConv : 0) | (adjust ? gConstQualAdjustStdConv : 0) };
}

/*!
 * \fun static StandardConversion NotConvertible()
 *
 * Constructs a standard conversion with rank \c{ConversionRank::NotConvertible}.
 */
StandardConversion StandardConversion::NotConvertible()
{
  StandardConversion seq;
  seq.d = gNotConvertibleStdConv;
  return seq;
}


/*!
 * \fun static StandardConversion compute(const Type & src, const Type & dest, Engine *e)
 *
 * Computes, if possible, the standard conversion between \c src and \c dest; or returns 
 * \c{NotConvertible()}.
 *
 * Note that unlike the constructor taking two fundamental \t Type, this function 
 * can take arbitrary types (hence the last \t Engine parameter).
 *
 * This function takes into account the \c{const}-ness and \c{\&}.
 */
StandardConversion StandardConversion::compute(const Type & src, const Type & dest, Engine *e)
{
  if (dest.isReference() && src.isConst() && !dest.isConst())
    return StandardConversion::NotConvertible();

  if (dest.isFundamentalType() && src.isFundamentalType())
    return StandardConversion{ src, dest };

  if (src.isObjectType() && dest.isObjectType())
  {
    const Class src_class = e->typeSystem()->getClass(src), dest_class = e->typeSystem()->getClass(dest);
    const int inheritance_depth = src_class.inheritanceLevel(dest_class);

    if (inheritance_depth < 0)
      return StandardConversion::NotConvertible();

    const QualificationAdjustment adjust = (dest.isConst() && !src.isConst()) ? ConstQualification : NoQualificationAdjustment;

    if (inheritance_depth == 0)
    {
      if (dest.isReference())
        return StandardConversion{}.with(adjust);

      if (!dest_class.isCopyConstructible())
        return StandardConversion::NotConvertible();
      return StandardConversion::Copy().with(adjust);
    }
    else
    {
      if (dest.isReference())
        return StandardConversion::DerivedToBaseConversion(inheritance_depth, dest.isReference(), adjust);

      if (!dest_class.isCopyConstructible())
        return StandardConversion::NotConvertible();
      return StandardConversion::DerivedToBaseConversion(inheritance_depth, dest.isReference(), adjust);
    }
  }
  else if (src.baseType() == dest.baseType())
  {
    const QualificationAdjustment adjust = (dest.isConst() && !src.isConst()) ? ConstQualification : NoQualificationAdjustment;

    if (dest.isReference())
      return StandardConversion{};

    if (dest.isEnumType() || dest.isClosureType() || dest.isFunctionType())
      return StandardConversion::Copy().with(adjust);
  }
  else if (src.isEnumType() && dest.baseType() == Type::Int)
  {
    const QualificationAdjustment adjust = (dest.isConst() && !src.isConst()) ? ConstQualification : NoQualificationAdjustment;

    if (dest.isReference())
      return StandardConversion::NotConvertible();

    return StandardConversion::EnumToInt().with(adjust);
  }

  return StandardConversion::NotConvertible();
}


template<typename T>
T fundamental_value_cast(const Value& v)
{
  switch (v.type().baseType().data())
  {
  case Type::Boolean:
    return static_cast<T>(v.toBool());
  case Type::Char:
    return static_cast<T>(v.toChar());
  case Type::Int:
    return static_cast<T>(v.toInt());
  case Type::Float:
    return static_cast<T>(v.toFloat());
  case Type::Double:
    return static_cast<T>(v.toDouble());
  default:
    break;
  }

  if (v.type().isEnumType())
    return (T)v.toEnumerator().value();

  throw std::runtime_error{ "fundamental_value_cast : Implementation error" };
}

Value fundamental_conversion(const Value& src, int destType, Engine* e)
{
  switch (destType)
  {
  case Type::Boolean:
    return e->newBool(fundamental_value_cast<bool>(src));
  case Type::Char:
    return e->newChar(fundamental_value_cast<char>(src));
  case Type::Int:
    return e->newInt(fundamental_value_cast<int>(src));
  case Type::Float:
    return e->newFloat(fundamental_value_cast<float>(src));
  case Type::Double:
    return e->newDouble(fundamental_value_cast<double>(src));
  default:
    break;
  }

  throw std::runtime_error{ "fundamental_conversion : Implementation error" };
}

Value StandardConversion::apply(const StandardConversion& conv, const Value& val)
{
  if (conv.isReferenceConversion())
    return val;

  if (conv.isCopy())
    return val.engine()->copy(val);

  if (conv.isDerivedToBaseConversion())
  {
    Class target = val.engine()->typeSystem()->getClass(val.type()).indirectBase(conv.derivedToBaseConversionDepth());
    return target.copyConstructor().invoke({ Value(), val });
  }

  return fundamental_conversion(val, conv.destType().baseType().data(), val.engine());
}

bool StandardConversion::operator==(const StandardConversion & other) const
{
  return d == other.d;
}

/*!
 * \fun bool operator<(const StandardConversion & other) const
 *
 * Compares two standard conversion. 
 * This comparison is primarily used to perform overload resolution.
 *
 * The function first compares the two conversion's rank (with \c ExactMatch being 
 * the smallest and \c NotConvertible the largest), than the \c{derivedToBaseConversionDepth()},
 * than if one is a reference conversion and the other is not (reference conversion are considered 
 * better), than the qualification adjustment (no adjustment is better than one).
 */
bool StandardConversion::operator<(const StandardConversion & other) const
{
  auto this_rank = rank();
  auto other_rank = other.rank();
  if (this_rank < other_rank)
    return true;
  else if (this_rank > other_rank)
    return false;

  if (other.isDerivedToBaseConversion() && this->isDerivedToBaseConversion())
  {
    if (this->derivedToBaseConversionDepth() < other.derivedToBaseConversionDepth())
      return true;
    else if (this->derivedToBaseConversionDepth() > other.derivedToBaseConversionDepth())
      return false;
  }

  if (!this->isReferenceConversion() && other.isReferenceConversion())
    return false;
  else if (this->isReferenceConversion() && !other.isReferenceConversion())
    return true;

  if (other.hasQualificationAdjustment() && !this->hasQualificationAdjustment())
    return true;

  return false;
}


static Conversion select_converting_constructor(const Type & src, const std::vector<Function> & ctors, const Type & dest, Engine *engine, Conversion::ConversionPolicy policy)
{
  if (dest.isReference() && !dest.isConst() && src.isConst())
    return Conversion::NotConvertible();

  // We store the two best conversion sequences to detect ambiguity.
  StandardConversion best_conv = StandardConversion::NotConvertible();
  Function best_ctor;
  StandardConversion ambiguous_conv = StandardConversion::NotConvertible();

  for (const auto & c : ctors)
  {
    if (c.prototype().count() != 2)
      continue;

    if (c.isExplicit() && policy == Conversion::NoExplicitConversions)
      continue;

    StandardConversion first_conversion = StandardConversion::compute(src, c.prototype().at(1), engine);
    if (first_conversion == StandardConversion::NotConvertible())
      continue;

    bool comp1 = first_conversion < best_conv;
    bool comp2 = best_conv < first_conversion;

    if (comp1 && !comp2)
    {
      best_conv = first_conversion;
      best_ctor = c;
      ambiguous_conv = StandardConversion::NotConvertible();
    }
    else if (comp2 && !comp1)
    {
      // nothing to do
    }
    else
    {
      ambiguous_conv = first_conversion;
    }
  }

  if (!(best_conv < ambiguous_conv))
    return Conversion::NotConvertible();

  /// TODO: correctly compute second conversion, which can be
  // - a const-qualification
  // - a derived to base conversion
  //StandardConversion second_conversion = StandardConversion::compute(Type::rref(dest.baseType()), dest, engine);
  /// TODO : not sure this is correct
  /*if (second_conversion == StandardConversion::NotConvertible())
  continue;*/
  //second_conversion = StandardConversion::None();

  return Conversion{ best_conv, best_ctor, StandardConversion::None() };
}

static Conversion select_cast(const Type & src, const std::vector<Cast> & casts, const Type & dest, Engine *engine, Conversion::ConversionPolicy policy)
{
  // TODO : before returning, check if better candidates can be found ?
  // this would result in an ambiguous conversion sequence I believe
  for (const auto & c : casts)
  {
    if (c.isExplicit() && policy == Conversion::NoExplicitConversions)
      continue;

    StandardConversion first_conversion = StandardConversion::compute(src, c.sourceType(), engine);
    if (first_conversion == StandardConversion::NotConvertible())
      continue;

    StandardConversion second_conversion = StandardConversion::compute(c.destType(), dest, engine);
    if (second_conversion == StandardConversion::NotConvertible())
      continue;
    // Avoid additonal useless copy
    if (second_conversion == StandardConversion::Copy())
      second_conversion = StandardConversion{};

    return Conversion{ first_conversion, c, second_conversion };
  }

  return Conversion::NotConvertible();
}


Conversion::Conversion(const StandardConversion & c1, const Function & userdefinedConversion, const StandardConversion & c2)
  : conv1(c1)
  , function(userdefinedConversion)
  , conv3(c2)
{

}

ConversionRank Conversion::rank() const
{
  if (conv1 == StandardConversion::NotConvertible())
    return ConversionRank::NotConvertible;

  if (!function.isNull())
    return ConversionRank::UserDefinedConversion;

  return conv1.rank();
}

bool Conversion::isInvalid() const
{
  return conv1.rank() == ConversionRank::NotConvertible;
}

bool Conversion::isNarrowing() const
{
  return conv1.isNarrowing() || conv3.isNarrowing();
}

bool Conversion::isUserDefinedConversion() const
{
  return !this->function.isNull();
}

Type Conversion::srcType() const
{
  if (function.isNull())
    return conv1.srcType();
  else if (function.isConstructor())
    return function.parameter(1).baseType();

  assert(function.isCast());
  if (!conv3.isNone())
    return conv3.destType();
  return function.parameter(0).baseType();
}

Type Conversion::destType() const
{
  if (function.isNull())
    return conv1.destType();
  else if (function.isConstructor())
    return function.memberOf().id();

  assert(function.isCast());
  return function.returnType().baseType();
}

Conversion Conversion::NotConvertible()
{
  return Conversion{ StandardConversion::NotConvertible() };
}

Conversion Conversion::compute(const Type & src, const Type & dest, Engine *engine, ConversionPolicy policy)
{
  StandardConversion stdconv = StandardConversion::compute(src, dest, engine);
  if (stdconv != StandardConversion::NotConvertible())
    return stdconv;

  if (!src.isObjectType() && !dest.isObjectType())
    return Conversion::NotConvertible();

  assert(src.isObjectType() || dest.isObjectType());

  if (dest.isObjectType())
  {
    const auto & ctors = engine->typeSystem()->getClass(dest).constructors();
    Conversion userdefconv = select_converting_constructor(src, ctors, dest, engine, policy);
    if (userdefconv != Conversion::NotConvertible())
      return userdefconv;
  }

  if (src.isObjectType())
  {
    const auto & casts = engine->typeSystem()->getClass(src).casts();
    Conversion userdefconv = select_cast(src, casts, dest, engine, policy);
    if (userdefconv != Conversion::NotConvertible())
      return userdefconv;
  }

  /// TODO : implement other forms of conversion

  return Conversion::NotConvertible();
}

Conversion Conversion::compute(const std::shared_ptr<program::Expression> & expr, const Type & dest, Engine *engine)
{
  if (expr->type() == Type::InitializerList)
    throw std::runtime_error{ "Conversion does not support brace-expressions" };

  return compute(expr->type(), dest, engine);
}

int Conversion::comp(const Conversion & a, const Conversion & b)
{
  // 1) A standard conversion sequence is always better than a user defined conversion sequence.
  if (a.function.isNull() && !b.function.isNull())
    return -1;
  else if (b.function.isNull() && !a.function.isNull())
    return 1;

  if (a.function.isNull() && b.function.isNull())
  {
    if (a.conv1 < b.conv1)
      return -1;
    else if (b.conv1 < a.conv1)
      return 1;
    return 0;
  }

  assert(a.function.isNull() == false && b.function.isNull() == false);

  if (a.conv3 < b.conv3)
    return -1;
  else if (b.conv3 < a.conv3)
    return 1;
  return 0;
}

Value Conversion::apply(const Conversion& conv, const Value& val)
{
  if (!conv.isUserDefinedConversion())
    return StandardConversion::apply(conv.firstStandardConversion(), val);

  Engine* e = val.engine();
  Value ret = StandardConversion::apply(conv.firstStandardConversion(), val);
  
  if (conv.userDefinedConversion().isCast())
  {
    ret = conv.userDefinedConversion().invoke({ ret });
  }
  else
  {
    ret = conv.userDefinedConversion().invoke({ Value(), ret });
  }

  return  StandardConversion::apply(conv.secondStandardConversion(), ret);
}

/*!
 * \fn void apply(const std::vector<Conversion>& conv, std::vector<Value>& values)
 * \param conversions to be applied
 * \param input values
 * \brief Applies conversions to a range of value.
 *
 */
void Conversion::apply(const std::vector<Conversion>& conv, std::vector<Value>& values)
{
  for (size_t i(0); i < conv.size(); ++i)
  {
    values[i] = Conversion::apply(conv.at(i), values.at(i));
  }
}

bool Conversion::operator==(const Conversion & other) const
{
  return conv1 == other.conv1 && conv3 == other.conv3 && other.function == other.function;
}


} // namespace script