// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/conversions.h"

#include "script/engine.h"
#include "script/cast.h"

#include "script/program/expression.h"

namespace script
{

static StandardConversion same_type_conversion(const Type & src, const Type & dest, Engine *engine)
{
  if (dest.isReference())
  {
    if (dest.isConst())
    {
      if (!src.isConst())
        return StandardConversion::ReferenceInitialization().with(ConstQualification);
      else
        return StandardConversion::ReferenceInitialization();
    }

    /// TODO : is this correct ? I believe no
    //if (!(src.isReference() || src.isRefRef()))
    //  return StandardConversion::NotConvertible();
    //else if (src.isConst() && !dest.isConst())
    //  return StandardConversion::NotConvertible();
    /// should work better : a reference can be initialized from a non-reference 
    if (src.isConst() && !dest.isConst())
      return StandardConversion::NotConvertible();

    return StandardConversion::ReferenceInitialization();
  }
  else if (dest.isRefRef())
  {
    if (!src.isRefRef())
      return StandardConversion::NotConvertible();

    if (src.isConst() && !dest.isConst())
      return StandardConversion::NotConvertible();
    else if (dest.isConst() && !src.isConst())
      return StandardConversion::ReferenceInitialization().with(ConstQualification);

    return StandardConversion::ReferenceInitialization();
  }

  if (src.isFundamentalType() || src.isEnumType() || src.isClosureType() || src.isFunctionType())
    return StandardConversion{};
  else if (src.isObjectType())
  {
    Class c = engine->getClass(src);
    if (!c.isCopyConstructible())
      return StandardConversion::NotConvertible();
    return StandardConversion{};
  }

  return StandardConversion::NotConvertible();
}



static StandardConversion fundamental_conversion(const Type & srcType, const Type & destType, Engine *engine)
{
  static_assert(Type::Void == 1, "Fundamental types are expected to match a predefined numbering");
  static_assert(Type::Boolean == 2, "Fundamental types are expected to match a predefined numbering");
  static_assert(Type::Char == 3, "Fundamental types are expected to match a predefined numbering");
  static_assert(Type::Int == 4, "Fundamental types are expected to match a predefined numbering");
  static_assert(Type::Float == 5, "Fundamental types are expected to match a predefined numbering");
  static_assert(Type::Double == 6, "Fundamental types are expected to match a predefined numbering");

  const auto & ExactMatch = StandardConversion{};
  const auto & NotConvertible = StandardConversion::NotConvertible();

  // usage : ranks[src][dest]
  static const StandardConversion ranks[6][6] = {
    { ExactMatch, NotConvertible, NotConvertible, NotConvertible, NotConvertible, NotConvertible },
  { NotConvertible, ExactMatch, IntegralPromotion, IntegralPromotion, FloatingPointPromotion, FloatingPointPromotion },
  { NotConvertible, BooleanConversion, ExactMatch, IntegralPromotion, FloatingPointPromotion, FloatingPointPromotion },
  { NotConvertible, BooleanConversion, IntegralConversion, ExactMatch, FloatingPointPromotion, FloatingPointPromotion },
  { NotConvertible, BooleanConversion, IntegralConversion, IntegralConversion, ExactMatch, FloatingPointPromotion },
  { NotConvertible, BooleanConversion, IntegralConversion, IntegralConversion, FloatingPointConversion, ExactMatch },
  };

  assert(srcType.baseType() != destType.baseType()); // this case is handled before

  if (destType.isReference() && !destType.isConst()) // what about destType.isRefRef() ?
    return NotConvertible;

  const QualificationAdjustment constQual = destType.isConst() ? QualificationAdjustment::ConstQualification : QualificationAdjustment::NoQualificationAdjustment;
  auto ret = ranks[srcType.baseType().data() - 1][destType.baseType().data() - 1].with(constQual);
  if (destType.isConstRef())
    return StandardConversion::CopyRefInitialization(ret);
  return ret;
}

static ConversionSequence select_converting_constructor(const Type & src, const std::vector<Function> & ctors, const Type & dest, Engine *engine)
{
  // assert(!dest.isReference() || dest.isConst())

  // TODO : before returning, check if better candidates can be found ?
  // this would result in an ambiguous conversion sequence I believe
  for (const auto & c : ctors)
  {
    if (c.prototype().argc() != 1)
      continue;

    if (c.isExplicit())
      continue;

    StandardConversion first_conversion = StandardConversion::compute(src, c.prototype().argv(0), engine);
    if (first_conversion == StandardConversion::NotConvertible())
      continue;

    StandardConversion second_conversion = StandardConversion::compute(Type::rref(dest.baseType()), dest, engine);
    /// TODO : not sure this is correct
    /*if (second_conversion == StandardConversion::NotConvertible())
      continue;*/
    second_conversion = StandardConversion::None();

    return ConversionSequence{ first_conversion, c, second_conversion };
  }

  return ConversionSequence::NotConvertible();
}

static ConversionSequence select_cast(const Type & src, const std::vector<Cast> & casts, const Type & dest, Engine *engine)
{
  // TODO : before returning, check if better candidates can be found ?
  // this would result in an ambiguous conversion sequence I believe
  for (const auto & c : casts)
  {
    if (c.isExplicit())
      continue;

    StandardConversion first_conversion = StandardConversion::compute(src, c.sourceType(), engine);
    if (first_conversion == StandardConversion::NotConvertible())
      continue;

    StandardConversion second_conversion = StandardConversion::compute(c.destType(), dest, engine);
    if (second_conversion == StandardConversion::NotConvertible())
      continue;

    return ConversionSequence{ first_conversion, c, second_conversion };
  }

  return ConversionSequence::NotConvertible();
}


// The format used should allow easy comparison : 
// highest bit is set to 1 if no conversion is possible
// bit 0 is set to 1 for qualifcation adjustement
// bit 1 is used to indicate copy of one type to the same
// bit 2-3 are used to encode numeric promotion
// bit 4-6 are used to encode numeric conversion
// bit 8-16 are used to encode derived to base conversion

static int NumPromoMask = (3 << 1);
static int NumConvMask = (7 << 3);
static int DerivToBaseMask = (0xFF << DerivedToBaseConversion::Shift);

StandardConversion::StandardConversion()
  : d(static_cast<int>(CopyInitializationFlag))
{

}

StandardConversion::StandardConversion(NumericConversion conv, QualificationAdjustment qualadjust)
{
  d = static_cast<int>(CopyInitializationFlag) | static_cast<int>(conv) | static_cast<int>(qualadjust);
}

StandardConversion::StandardConversion(NumericPromotion promo, QualificationAdjustment qualadjust)
{
  d = static_cast<int>(CopyInitializationFlag) | static_cast<int>(promo) | static_cast<int>(qualadjust);
}

StandardConversion::StandardConversion(DerivedToBaseConversion dbconv, QualificationAdjustment qualadjust)
{
  d = static_cast<int>(CopyInitializationFlag) | static_cast<int>(dbconv) | static_cast<int>(qualadjust);

}

StandardConversion::StandardConversion(QualificationAdjustment qualadjust)
{
  d = static_cast<int>(qualadjust);
}


bool StandardConversion::isNone() const
{
  return d == 0;
}

StandardConversion StandardConversion::None()
{
  StandardConversion ret;
  ret.d = 0;
  return ret;
}

bool StandardConversion::isNarrowing() const
{
  return (d & NumConvMask) != 0;
}

StandardConversion::Rank StandardConversion::rank() const
{
  if ((d & NumConvMask) || (d & DerivToBaseMask))
    return Rank::Conversion;
  else if (d & NumPromoMask)
    return Rank::Promotion;
  else if (d & NotConvertibleFlag)
    return Rank::NotConvertible;
  return Rank::ExactMatch;
}

bool StandardConversion::isCopyInitialization() const
{
  return d & CopyInitializationFlag;
}

bool StandardConversion::isReferenceInitialization() const
{
  return d & ReferenceInitializationFlag;
}

bool StandardConversion::isNumericPromotion() const
{
  return d & NumPromoMask;
}

NumericPromotion StandardConversion::numericPromotion() const
{
  return static_cast<NumericPromotion>(d & NumPromoMask);
}


bool StandardConversion::isNumericConversion() const
{
  return d & NumConvMask;
}

NumericConversion StandardConversion::numericConversion() const
{
  return static_cast<NumericConversion>(d & NumConvMask);
}


bool StandardConversion::hasQualificationConversion() const
{
  return d & 1;
}

QualificationAdjustment StandardConversion::qualificationConversion()
{
  if (d & 1)
    return ConstQualification;
  return QualificationAdjustment::NoQualificationAdjustment;
}


bool StandardConversion::isDerivedToBaseConversion() const
{
  return (d & DerivToBaseMask);
}

int StandardConversion::derivedToBaseConversionDepth() const
{
  return (d & DerivToBaseMask) >> DerivedToBaseConversion::Shift;
}

StandardConversion StandardConversion::with(QualificationAdjustment adjust) const
{
  StandardConversion conv{ *this };
  conv.d |= static_cast<int>(adjust);
  return conv;
}


StandardConversion StandardConversion::NotConvertible()
{
  StandardConversion seq;
  seq.d = static_cast<int>(NotConvertibleFlag);
  return seq;
}

StandardConversion StandardConversion::ReferenceInitialization()
{
  StandardConversion seq;
  seq.d = static_cast<int>(ReferenceInitializationFlag);
  return seq;
}

StandardConversion StandardConversion::ReferenceInitialization(DerivedToBaseConversion dbconv, QualificationAdjustment qualadjust)
{
  StandardConversion seq;
  seq.d = static_cast<int>(ReferenceInitializationFlag) | static_cast<int>(dbconv) | static_cast<int>(qualadjust);
  return seq;
}

StandardConversion StandardConversion::ReferenceInitialization(QualificationAdjustment qualadjust)
{
  StandardConversion seq;
  seq.d = static_cast<int>(ReferenceInitializationFlag) | static_cast<int>(qualadjust);
  return seq;
}

StandardConversion StandardConversion::CopyRefInitialization(StandardConversion conv)
{
  if (conv.d & NotConvertibleFlag)
    return conv;
  conv.d |= static_cast<int>(CopyRefInitializationFlag);
  return conv;
}

StandardConversion StandardConversion::compute(const Type & src, const Type & dest, Engine *e)
{
  if (dest.isReference() && !dest.isConst() && src.isConst())
    return StandardConversion::NotConvertible();

  if (src.baseType() == dest.baseType())
    return same_type_conversion(src, dest, e);
  else if (src.isFundamentalType() && dest.isFundamentalType())
    return fundamental_conversion(src, dest, e);
  else if (src.isObjectType() && dest.isObjectType())
  {
    const Class src_class = e->getClass(src), dest_class = e->getClass(dest);
    const int inheritance_depth = src_class.inheritanceLevel(dest_class);
    if (inheritance_depth < 0)
      return StandardConversion::NotConvertible();

    const QualificationAdjustment adjust = (dest.isConst() && !src.isConst()) ? ConstQualification : NoQualificationAdjustment;

    if (dest.isReference())
      return StandardConversion::ReferenceInitialization(DerivedToBaseConversion{ inheritance_depth }, adjust);

    if (!dest_class.isCopyConstructible())
      return StandardConversion::NotConvertible();
    return StandardConversion{ DerivedToBaseConversion{inheritance_depth}, adjust };
  }
  else if (src.isEnumType() && dest.baseType() == Type::Int)
  {
    if (dest.isReference())
      return StandardConversion::NotConvertible();

    if (dest.isConstRef())
      return StandardConversion::CopyRefInitialization(StandardConversion{IntegralConversion});
    return StandardConversion{ IntegralConversion };
  }
  else if (src.isFunctionType() && dest.isFunctionType())
  {
    /// TODO : check if dest type is const-compatible with src type
    // eg. int(int, int) can be converted to int(const int, const int)
    return StandardConversion::NotConvertible();
  }

  return StandardConversion::NotConvertible();
}

bool StandardConversion::operator==(const StandardConversion & other) const
{
  return d == other.d;
}

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

  if (this->isCopyInitialization() && !other.isCopyInitialization())
    return false;
  else if (!this->isCopyInitialization() && other.isCopyInitialization())
    return true;

  return false;
}


ConversionSequence::ConversionSequence(const StandardConversion & c1, const Function & userdefinedConversion, const StandardConversion & c2)
  : conv1(c1)
  , function(userdefinedConversion)
  , conv3(c2)
{

}

ConversionSequence::ConversionSequence(const std::shared_ptr<ListInitializationSequence> & listinit)
  : listInitialization(listinit)
{

}

ConversionSequence::Rank ConversionSequence::rank() const
{
  if (conv1 == StandardConversion::NotConvertible())
    return Rank::NotConvertible;

  if (!function.isNull())
    return Rank::UserDefinedConversion;

  return static_cast<ConversionSequence::Rank>(conv1.rank());
}

ConversionSequence::Rank ConversionSequence::globalRank(const std::vector<ConversionSequence> & convs)
{
  if (convs.empty())
    return Rank::ExactMatch;

  Rank r = convs.front().rank();
  for (size_t i(1); i < convs.size(); ++i)
  {
    Rank r2 = convs.at(i).rank();
    r = r2 > r ? r2 : r;
  }
  return r;
}

bool ConversionSequence::isInvalid() const
{
  return conv1.rank() == StandardConversion::Rank::NotConvertible;
}

bool ConversionSequence::isNarrowing() const
{
  return conv1.isNarrowing() || conv3.isNarrowing();
}

bool ConversionSequence::isUserDefinedConversion() const
{
  return !this->function.isNull();
}

bool ConversionSequence::isListInitialization() const
{
  return this->listInitialization != nullptr;
}

ConversionSequence ConversionSequence::NotConvertible()
{
  return ConversionSequence{ StandardConversion::NotConvertible() };
}

ConversionSequence ConversionSequence::compute(const Type & src, const Type & dest, Engine *engine)
{
  if (dest.isReference() && !dest.isConst() && src.isConst())
    return ConversionSequence::NotConvertible();

  StandardConversion stdconv = StandardConversion::compute(src, dest, engine);
  if (stdconv != StandardConversion::NotConvertible())
    return stdconv;

  if (!src.isObjectType() && !dest.isObjectType())
    return ConversionSequence::NotConvertible();

  assert(src.isObjectType() || dest.isObjectType());

  if (dest.isObjectType())
  {
    const auto & ctors = engine->getClass(dest).constructors();
    ConversionSequence userdefconv = select_converting_constructor(src, ctors, dest, engine);
    if (userdefconv != ConversionSequence::NotConvertible())
      return userdefconv;
  }

  if (src.isObjectType())
  {
    const auto & casts = engine->getClass(src).casts();
    ConversionSequence userdefconv = select_cast(src, casts, dest, engine);
    if (userdefconv != ConversionSequence::NotConvertible())
      return userdefconv;
  }

  // TODO : implement other forms of conversion

  return ConversionSequence::NotConvertible();
}

ConversionSequence ConversionSequence::compute(const std::shared_ptr<program::Expression> & expr, const Type & dest, Engine *engine)
{
  if (expr->type() != Type::InitializerList)
    return compute(expr->type(), dest, engine);

  const program::InitializerList & init_list = dynamic_cast<const program::InitializerList &>(*expr);

  if (!dest.isObjectType())
    return ConversionSequence::NotConvertible();

  if (dest.isReference() && !dest.isConstRef())
    return ConversionSequence::NotConvertible();

  const Class dest_class = engine->getClass(dest);

  auto result = std::make_shared<ListInitializationSequence>();
  std::vector<ConversionSequence> & conversions = result->conversions();
  conversions.resize(init_list.elements.size());

  for (const auto & c : dest_class.constructors())
  {
    if (c.prototype().argc() != init_list.elements.size())
      continue;

    for (size_t i(0); i < init_list.elements.size(); ++i)
    {
      ConversionSequence conv = ConversionSequence::compute(init_list.elements.at(i), c.prototype().argv(i), engine);
      if (conv == ConversionSequence::NotConvertible())
        break;
    }

    if (conversions.back() != ConversionSequence::NotConvertible())
      return ConversionSequence{ result };
  }

  // TODO : implement aggregate and initializer_list initialization

  return ConversionSequence::NotConvertible();
}

int ConversionSequence::comp(const ConversionSequence & a, const ConversionSequence & b)
{
  if ((a.listInitialization == nullptr) != (b.listInitialization == nullptr))
    throw std::runtime_error{ "ConversionSequence::comp() : the two sequences are not comparable" };

  if (a.listInitialization != nullptr)
    return ListInitializationSequence::comp(*a.listInitialization, *b.listInitialization);

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

bool ConversionSequence::operator==(const ConversionSequence & other) const
{
  // TODO : add comparison when it is a brace initialization
  return conv1 == other.conv1 && conv3 == other.conv3 && other.function == other.function;
}

std::vector<ConversionSequence> & ListInitializationSequence::conversions()
{
  return mConversions;
}

const std::vector<ConversionSequence> & ListInitializationSequence::conversions() const
{
  return mConversions;
}

int ListInitializationSequence::comp(const ListInitializationSequence & a, const ListInitializationSequence & b)
{
  if (a.mKind == InitializerListInitialization && b.mKind != InitializerListInitialization)
    return -1;
  else if (b.mKind == InitializerListInitialization && a.mKind != InitializerListInitialization)
    return 1;

  // TODO : we can do better, we should store in the ListInitializationSequence the worst ConversionSequence
  return 0;
}

} // namespace script