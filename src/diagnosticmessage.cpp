// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/diagnosticmessage.h"

#include "script/accessspecifier.h"
#include "script/engine.h"
#include "script/initialization.h"
#include "script/operator.h"
#include "script/overloadresolution.h"
#include "script/typesystem.h"

#include "script/parser/token.h"
#include "script/parser/parsererrors.h"
#include "script/compiler/compilererrors.h"

#include <cstring>
#include <initializer_list>
#include <sstream>
#include <unordered_map>

namespace script
{

namespace diagnostic
{

inline static bool is_digit(const char c)
{
  return c >= '0' && c <= '9';
}

inline static bool is_letter(const char c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline static bool is_alphanumeric(const char c)
{
  return is_letter(c) || is_digit(c);
}

struct placemarker_t
{
  size_t pos;
  size_t length;
  int value;
};

placemarker_t read_placemarker(const std::string & str, size_t pos)
{
  size_t num_start = pos + 1;
  size_t end = num_start;
  while (end < str.size() && is_digit(str[end]))
    ++end;

  if (end == num_start)
    return placemarker_t{ std::string::npos, 0, -1 };

  return placemarker_t{ pos, end - pos, std::stoi(str.substr(num_start, end - num_start)) };
}

placemarker_t find_lowest_placemarker(const std::string & str)
{
  std::size_t pos = str.find('%');
  if (pos == std::string::npos)
    return placemarker_t{ pos, 0, -1 };

  placemarker_t pm = read_placemarker(str, pos);
  pos = str.find('%', pos + 1);
  while (pos != std::string::npos)
  {
    placemarker_t new_pm = read_placemarker(str, pos);
    if (new_pm.value != -1 && new_pm.value < pm.value)
      pm = new_pm;
    pos = str.find('%', pos + 1);
  }

  return pm;
}

const std::string & format(const std::string & str)
{
  return str;
}

std::string format(std::string str, const std::string & a)
{
  placemarker_t pm = find_lowest_placemarker(str);
  if (pm.value == -1)
    return str;

  str.replace(str.begin() + pm.pos, str.begin() + pm.pos + pm.length, a);
  return str;
}

std::string format(std::string str, const std::string & arg1, const std::string & arg2)
{
  return format(format(str, arg1), arg2);
}

std::string format(std::string str, const std::string & arg1, const std::string & arg2, const std::string & arg3)
{
  return format(format(str, arg1, arg2), arg3);
}

std::string format(std::string str, const std::string & arg1, const std::string & arg2, const std::string & arg3, const std::string & arg4)
{
  return format(format(str, arg1, arg2, arg3), arg3, arg4);
}

DiagnosticMessage::DiagnosticMessage(Severity s)
  : mSeverity(s)
{

}

DiagnosticMessage::DiagnosticMessage(Severity s, std::error_code ec, SourceLocation loc, std::string text)
  : mSeverity(s),
    mCode(ec),
    mLocation(loc),
    mContent(std::move(text))
{

}

DiagnosticMessage::DiagnosticMessage(Severity s, std::error_code ec, std::string text)
  : mSeverity(s),
    mCode(ec),
    mContent(std::move(text))
{

}

void DiagnosticMessage::setSeverity(Severity sev)
{
  mSeverity = sev;
}

std::string DiagnosticMessage::message() const
{
  std::string result;

  switch (mSeverity)
  {
  case Info:
    result += "[info]";
    break;
  case Warning:
    result += "[warning]";
    break;
  case Error:
    result += "[error]";
    break;
  }

  if (!mCode)
  {
    /// TODO: do something with that ?
  }


  if (line() != std::numeric_limits<uint16_t>::max())
  {
    result += std::to_string(line());
    result += std::string{ ":" };
    if (column() != std::numeric_limits<uint16_t>::max())
      result += std::to_string(column()) + std::string{ ":" };
  }

  result += " ";

  result += mContent;

  return result;
}

const std::string & DiagnosticMessage::content() const
{
  return mContent;
}

void DiagnosticMessage::setContent(std::string str)
{
  mContent = std::move(str);
}

void DiagnosticMessage::setCode(std::error_code ec)
{
  mCode = ec;
}

void DiagnosticMessage::setLocation(const SourceLocation& loc)
{
  mLocation = loc;
}

MessageBuilder::MessageBuilder(Engine* e)
  : mEngine(e)
{

}

MessageBuilder::~MessageBuilder()
{

}

Verbosity MessageBuilder::verbosity() const
{
  return mVerbosity;
}

void MessageBuilder::setVerbosity(Verbosity ver)
{
  mVerbosity = ver;
}

void MessageBuilder::build(DiagnosticMessage& mssg, const parser::SyntaxError& ex)
{
  auto tok_to_string = [&ex](parser::Token tok) -> std::string
  {
    return std::string(ex.location.m_source.data() + tok.pos, ex.location.m_source.data() + tok.pos + tok.length);
  };

  mssg.setCode(ex.errorCode());
  mssg.setLocation(ex.location);

  if (verbosity() == Verbosity::Terse || !ex.data())
  {
    mssg.setContent(ex.errorCode().message());
  }
  else
  {
    switch (static_cast<ParserError>(ex.errorCode().value()))
    {
    case ParserError::UnexpectedToken:
    {
      auto& data = ex.data()->get<parser::errors::UnexpectedToken>();

      if (data.expected == parser::Token::Invalid)
      {
        std::string text = format("expected '%1' got '%2'", to_string(data.expected), tok_to_string(data.actual));
        mssg.setContent(std::move(text));
      }
      else
      {
        std::string text = format("unexpected token '%1'", tok_to_string(data.actual));
        mssg.setContent(std::move(text));
      }
    }
    break;
    case ParserError::ExpectedIdentifier:
    {
      auto& data = ex.data()->get<parser::errors::ActualToken>();
      std::string text = format("expected identifier got '%1'", tok_to_string(data.token));
      mssg.setContent(std::move(text));
    }
    break;
    case ParserError::ExpectedUserDefinedName:
    {
      auto& data = ex.data()->get<parser::errors::ActualToken>();
      std::string text = format("expected user-defined name got '%1'", tok_to_string(data.token));
      mssg.setContent(std::move(text));
    }
    break;
    case ParserError::ExpectedLiteral:
    {
      auto& data = ex.data()->get<parser::errors::ActualToken>();
      std::string text = format("expected literal name got '%1'", tok_to_string(data.token));
      mssg.setContent(std::move(text));
    }
    break;
    case ParserError::ExpectedOperatorSymbol:
    {
      auto& data = ex.data()->get<parser::errors::ActualToken>();
      std::string text = format("expected 'operator' to be followed by operator symbol, got '%1'", tok_to_string(data.token));
      mssg.setContent(std::move(text));
    }
    break;
    default:
      mssg.setContent(ex.errorCode().message());
      break;
    }
  }
}

static std::unordered_map<CompilerError, std::string> build_compiler_error_fmt()
{
  return std::unordered_map<CompilerError, std::string>{
    { CompilerError::SyntaxError, "Syntax error: %1" },
    { CompilerError::IllegalUseOfThis, "Illegal use of this" },
    { CompilerError::ObjectHasNoDestructor, "Object has no destructor" },
    { CompilerError::InvalidUseOfDelegatedConstructor, "No other member initializer may be present when using delegating constructors" },
    { CompilerError::NotDataMember, "%1 does not name a data member" },
    { CompilerError::InheritedDataMember, "cannot initialize inherited data member %1" },
    { CompilerError::DataMemberAlreadyHasInitializer, "data member %1 already has an intializer" },
    { CompilerError::NoDelegatingConstructorFound, "Could not find a delegate constructor" },
    { CompilerError::CouldNotFindValidBaseConstructor, "Could not find valid base constructor" },
    { CompilerError::InitializerListAsFirstArrayElement, "An initializer list cannot be used as the first element of an array" },
    { CompilerError::ReturnStatementWithoutValue, "Cannot have return-statement without a value in function returning non-void" },
    { CompilerError::ReturnStatementWithValue, "A function returning void cannot return a value" },
    { CompilerError::ReferencesMustBeInitialized, "References must be initialized" },
    { CompilerError::EnumerationsCannotBeDefaultConstructed, "Enumerations cannot be default constructed" },
    { CompilerError::EnumerationsMustBeInitialized, "Variables of enumeration type must be initialized" },
    { CompilerError::FunctionVariablesMustBeInitialized, "Variables of function-type must be initialized" },
    { CompilerError::VariableCannotBeDefaultConstructed, "Class '%1' does not provide a default constructor" },
    { CompilerError::ClassHasDeletedDefaultCtor, "Class '%1' has a deleted default constructor" },
    { CompilerError::CouldNotResolveOperatorName, "Could not resolve operator name based on parameter count and operator symbol." },
    { CompilerError::InvalidParamCountInOperatorOverload, "Invalid parameter count found in operator overload, expected %1 got %2" },
    { CompilerError::OpOverloadMustBeDeclaredAsMember, "This operator can only be overloaded as a member" },
    { CompilerError::InvalidTypeName, "%1 does not name a type" },
    { CompilerError::DataMemberCannotBeAuto, "Data members cannot be declared 'auto'." },
    { CompilerError::MissingStaticInitialization, "A static variable must be initialized." },
    { CompilerError::InvalidStaticInitialization, "Static variables can only be initialized through assignment." },
    { CompilerError::FailedToInitializeStaticVariable, "Failed to initialized static variable." },
    { CompilerError::InvalidBaseClass, "Invalid base class." },
    { CompilerError::InvalidUseOfDefaultArgument, "Cannot have a parameter without a default value after one was provided." },
    { CompilerError::ArrayElementNotConvertible, "Could not convert element to array's element type." },
    { CompilerError::ArraySubscriptOnNonObject, "Cannot perform array subscript on non object type." },
    { CompilerError::CouldNotFindValidSubscriptOperator, "Could not find valid subscript operator." },
    { CompilerError::CannotCaptureThis, "'this' cannot be captured outside of a member function." },
    { CompilerError::UnknownCaptureName, "Could not capture any local variable with given name." },
    { CompilerError::CannotCaptureNonCopyable, "Cannot capture by value a non copyable type." },
    { CompilerError::SomeLocalsCannotBeCaptured, "Some local variables cannot be captured by value." },
    { CompilerError::CannotCaptureByValueAndByRef, "Cannot capture both everything by reference and by value." },
    { CompilerError::LambdaMustBeCaptureless, "A lambda must be captureless within this context." },
    { CompilerError::CouldNotFindValidConstructor, "Could not find valid constructor." },
    { CompilerError::CouldNotFindValidMemberFunction, "Could not find valid member function for call." },
    { CompilerError::CouldNotFindValidOperator, "Could not find valid operator overload." },
    { CompilerError::CouldNotFindValidCallOperator, "Could not find valid operator() overload for call." },
    { CompilerError::AmbiguousFunctionName, "Name does not refer to a single function" },
    { CompilerError::TemplateNamesAreNotExpressions, "Name refers to a template and cannot be used inside an expression" },
    { CompilerError::TypeNameInExpression, "Name refers to a type and cannot be used inside an expression" },
    { CompilerError::NamespaceNameInExpression, "Name refers to a namespace and cannot be used inside an expression" },
    { CompilerError::TooManyArgumentInVariableInitialization, "Too many arguments provided in variable initialization." },
    { CompilerError::TooManyArgumentInInitialization, "Too many arguments provided in initialization." },
    { CompilerError::TooManyArgumentInReferenceInitialization, "More than one argument provided in reference initialization." },
    { CompilerError::CouldNotConvert, "Could not convert from %1 to %2" },
    { CompilerError::CouldNotFindCommonType, "Could not find common type of %1 and %2 in conditionnal expression" },
    { CompilerError::CannotAccessMemberOfNonObject, "Cannot access member of non object type." },
    { CompilerError::NoSuchMember, "Object has no such member." },
    { CompilerError::InvalidTemplateArgument, "Invalid template argument." },
    { CompilerError::InvalidLiteralTemplateArgument, "Only integer and boolean literals can be used as template arguments." },
    { CompilerError::MissingNonDefaultedTemplateParameter, "Missing non-defaulted template parameter." },
    { CompilerError::CouldNotFindPrimaryClassTemplate, "Could not find primary class template (must be declared in the same namespace)." },
    { CompilerError::CouldNotFindPrimaryFunctionTemplate, "Could not find primary function template (must be declared in the same namespace)." },
    { CompilerError::InvalidUseOfConstKeyword, "Invalid use of const keyword." },
    { CompilerError::InvalidUseOfExplicitKeyword, "Invalid use of 'explicit' keyword." },
    { CompilerError::InvalidUseOfStaticKeyword, "Invalid use of static keyword." },
    { CompilerError::InvalidUseOfVirtualKeyword, "Invalid use of virtual keyword." },
    { CompilerError::AutoMustBeUsedWithAssignment, "'auto' can only be used with assignment initialization." },
    { CompilerError::CannotDeduceLambdaReturnType, "Cannot deduce lambda return type" },
    { CompilerError::CallToDeletedFunction, "Call to deleted function." },
    { CompilerError::FunctionCannotBeDefaulted, "Function cannot be defaulted." },
    { CompilerError::ParentHasNoDefaultConstructor, "Cannot generate defaulted default constructor because parent has no default constructor." },
    { CompilerError::ParentHasDeletedDefaultConstructor, "Cannot generate defaulted default constructor because parent default constructor is deleted." },
    { CompilerError::ParentHasNoCopyConstructor, "Cannot generate defaulted copy constructor because parent has no copy constructor." },
    { CompilerError::ParentHasDeletedCopyConstructor, "Cannot generate defaulted copy constructor because parent copy constructor is deleted." },
    { CompilerError::DataMemberIsNotCopyable, "Cannot generate defaulted copy constructor because at least one data member is not copyable." },
    { CompilerError::ParentHasDeletedMoveConstructor, "Cannot generate defaulted move constructor because parent move constructor is deleted." },
    { CompilerError::DataMemberIsNotMovable, "Cannot generate defaulted move constructor because at least one data member is not movable." },
    { CompilerError::ParentHasNoAssignmentOperator, "Cannot generate defaulted assignment operator because parent has no assignment operator." },
    { CompilerError::ParentHasDeletedAssignmentOperator, "Cannot generate defaulted assignment operator because parent has a deleted assignment operator." },
    { CompilerError::DataMemberHasNoAssignmentOperator, "Cannot generate defaulted assignment operator because at least one data member has no assignment operator." },
    { CompilerError::DataMemberHasDeletedAssignmentOperator, "Cannot generate defaulted assignment operator because at least one data member has a deleted assignment operator." },
    { CompilerError::DataMemberIsReferenceAndCannotBeAssigned, "Cannot generate defaulted assignment operator because at least one data member is a reference." },
    { CompilerError::InvalidCharacterLiteral, "A character literal must contain only one character." },
    { CompilerError::CouldNotFindValidLiteralOperator, "Could not find valid literal operator." },
    { CompilerError::UnknownTypeInBraceInitialization, "Unknown type %1 in brace initialization" },
    { CompilerError::NarrowingConversionInBraceInitialization, "Narrowing conversion from %1 to %2 in brace initialization" },
    { CompilerError::NamespaceDeclarationCannotAppearAtThisLevel, "Namespace declarations cannot appear at this level" },
    { CompilerError::ExpectedDeclaration, "Expected a declaration." },
    { CompilerError::GlobalVariablesCannotBeAuto, "Global variables cannot be declared with auto." },
    { CompilerError::GlobalVariablesMustBeInitialized, "Global variables must have an initializer." },
    { CompilerError::GlobalVariablesMustBeAssigned, "Global variables must be initialized through assignment." },
    { CompilerError::InaccessibleMember, "%1 is %2 within this context" },
    { CompilerError::FriendMustBeAClass, "Friend must be a class" },
    { CompilerError::UnknownModuleName, "Unknown module '%1'" },
    { CompilerError::UnknownSubModuleName, "'%1' is not a submodule of '%2'" },
    { CompilerError::ModuleImportationFailed, "Failed to import module '%1'\n%2" },
    { CompilerError::InvalidNameInUsingDirective, "%1 does not name a namespace" },
    { CompilerError::NoSuchCallee, "Callee was not declared in this scope" },
    { CompilerError::LiteralOperatorNotInNamespace, "Literal operators can only appear at namespace level" },
  };
}

void MessageBuilder::build(DiagnosticMessage& mssg, const compiler::CompilationFailure& ex)
{
  static const std::unordered_map<CompilerError, std::string> format_strings = build_compiler_error_fmt();

  mssg.setCode(ex.errorCode());
  mssg.setLocation(ex.location);

  if (verbosity() == Verbosity::Terse || !ex.data())
  {
    mssg.setContent(ex.errorCode().message());
  }
  else
  {
    const std::string& fmt = format_strings.at(static_cast<CompilerError>(ex.errorCode().value()));

    switch (static_cast<CompilerError>(ex.errorCode().value()))
    {
    case CompilerError::SyntaxError:
    {
      auto& data = ex.data()->get<DiagnosticMessage>();
      std::string text = format(fmt, data.message());
      mssg.setContent(std::move(text));
    }
    break;
    case CompilerError::UnknownTypeInBraceInitialization:
    case CompilerError::UnknownModuleName:
    case CompilerError::UnknownSubModuleName:
    case CompilerError::InvalidNameInUsingDirective:
    case CompilerError::InvalidTypeName:
    {
      auto& data = ex.data()->get<compiler::errors::InvalidName>();
      std::string text = format(fmt, data.name);
      mssg.setContent(std::move(text));
    }
    break;
    case CompilerError::NarrowingConversionInBraceInitialization:
    {
      auto& data = ex.data()->get<compiler::errors::NarrowingConversion>();
      std::string text = format(fmt, engine()->toString(data.src), engine()->toString(data.dest));
      mssg.setContent(std::move(text));
    }
    break;
    case CompilerError::CouldNotConvert:
    {
      auto& data = ex.data()->get<compiler::errors::ConversionFailure>();
      std::string text = format(fmt, engine()->toString(data.src), engine()->toString(data.dest));
      mssg.setContent(std::move(text));
    }
    break;
    case CompilerError::InvalidParamCountInOperatorOverload:
    {
      auto& data = ex.data()->get<compiler::errors::ParameterCount>();
      std::string text = format(fmt, std::to_string(data.expected), std::to_string(data.actual));
      mssg.setContent(std::move(text));
    }
    break;
    case CompilerError::CouldNotFindCommonType:
    {
      auto& data = ex.data()->get<compiler::errors::NoCommonType>();
      std::string text = format(fmt, engine()->toString(data.first), engine()->toString(data.second));
      mssg.setContent(std::move(text));
    }
    break;
    case CompilerError::NotDataMember:
    case CompilerError::InheritedDataMember:
    case CompilerError::DataMemberAlreadyHasInitializer:
    case CompilerError::NoSuchMember:
    {
      auto& data = ex.data()->get<compiler::errors::DataMemberName>();
      std::string text = format(fmt, data.name);
      mssg.setContent(std::move(text));
    }
    break;
    case CompilerError::ClassHasDeletedDefaultCtor:
    {
      auto& data = ex.data()->get<compiler::errors::VariableType>();
      std::string text = format(fmt, engine()->toString(data.type));
      mssg.setContent(std::move(text));
    }
    break;
    case CompilerError::InaccessibleMember:
    {
      auto& data = ex.data()->get<compiler::errors::InaccessibleMember>();
      std::string text = format(fmt, data.name, to_string(data.access));
      mssg.setContent(std::move(text));
    }
    break;
    case CompilerError::ModuleImportationFailed:
    {
      auto& data = ex.data()->get<compiler::errors::ModuleImportationFailed>();
      std::string text = format(fmt, data.message);
      mssg.setContent(std::move(text));
    }
    break;
    default:
      mssg.setContent(ex.errorCode().message());
      break;
    }
  }
}

std::string MessageBuilder::produce(const OverloadResolution& resol) const
{
  if (resol.success())
    return "Overload resolution succeeded";

  std::stringstream ss;

  if (!resol.ambiguousOverload().isNull())
  {
    ss << "Overload resolution failed because at least two candidates are not comparable or indistinguishable \n";
    ss << engine()->toString(resol.selectedOverload()) << "\n";
    ss << engine()->toString(resol.ambiguousOverload()) << "\n";
    return ss.str();
  }

  ss << "Overload resolution failed because no viable overload could be found \n";
  std::vector<Initialization> paraminits;
  for (const auto& f : resol.candidates())
  {
    auto status = resol.getViabilityStatus(f, &paraminits);
    ss << engine()->toString(f);
    if (status == OverloadResolution::IncorrectParameterCount)
      ss << "\n" << "Incorrect argument count, expects " << f.prototype().count() << " but " << resol.inputSize() << " were provided";
    else if (status == OverloadResolution::CouldNotConvertArgument)
      ss << "\n" << "Could not convert argument " << paraminits.size();
    ss << "\n";
  }
  return ss.str();
}

} // namespace diagnostic

} // namespace script
