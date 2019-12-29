// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/diagnosticmessage.h"

#include "script/accessspecifier.h"
#include "script/engine.h"
#include "script/operator.h"
#include "script/typesystem.h"

#include "script/parser/parsererrors.h"
#include "script/compiler/compilererrors.h"

#include <cstring>
#include <initializer_list>

namespace script
{

namespace diagnostic
{

class MessageDatabase
{
public:
  std::vector<std::string> messages;
  std::vector<std::string> tokens;

public:
  MessageDatabase()
  {
    messages.resize(static_cast<int>(ErrorCode::LastCompilerError) + 1);
    tokens.resize(static_cast<int>(parser::Token::MultiLineComment) + 1);
  }

public:
  void setMessage(ErrorCode c, std::string && mssg);
  void setMessages(std::initializer_list<std::pair<ErrorCode, std::string>> && list);

  void setToken(parser::Token::Type, std::string && mssg);
  void setTokens(std::initializer_list<std::pair<parser::Token::Type, std::string>> && list);
};

void MessageDatabase::setMessage(ErrorCode c, std::string && mssg)
{
  const int offset = static_cast<int>(c);
  messages[offset] = std::move(mssg);
}

void MessageDatabase::setMessages(std::initializer_list<std::pair<ErrorCode, std::string>> && list)
{
  for (auto & p : list)
    setMessage(p.first, std::string{ p.second });
}

void MessageDatabase::setToken(parser::Token::Type tt, std::string && mssg)
{
  tokens[static_cast<int>(tt) & 0xFFFF] = std::move(mssg);
}

void MessageDatabase::setTokens(std::initializer_list<std::pair<parser::Token::Type, std::string>> && list)
{
  for (auto & p : list)
    setToken(p.first, std::string{ p.second });
}


static std::shared_ptr<MessageDatabase> build_message_database()
{
  auto ret = std::make_shared<MessageDatabase>();
  
  ret->setTokens({
    { parser::Token::LeftPar, "(" },
    { parser::Token::RightPar, ")" },
    { parser::Token::LeftBracket, "[" },
    { parser::Token::RightBracket, "]" },
    { parser::Token::LeftBrace, "{" },
    { parser::Token::RightBrace, "}" },
    { parser::Token::Semicolon, " }," },
    { parser::Token::Colon, ":" },
    { parser::Token::Dot, "." },
    { parser::Token::QuestionMark, "?" },
    { parser::Token::SlashSlash, "//" },
    { parser::Token::SlashStar, "/*" },
    { parser::Token::StarSlash, "*/" },
    // keywords 
    { parser::Token::Auto, "auto" },
    { parser::Token::Bool, "bool" },
    { parser::Token::Break, "break" },
    { parser::Token::Char, "char" },
    { parser::Token::Class, "class" },
    { parser::Token::Const, "const" },
    { parser::Token::Continue, "continue" },
    { parser::Token::Default, "default" },
    { parser::Token::Delete, "delete" },
    { parser::Token::Double, "double" },
    { parser::Token::Else, "else" },
    { parser::Token::Enum, "enum" },
    { parser::Token::Explicit, "explicit" },
    { parser::Token::Export, "export" },
    { parser::Token::False, "false" },
    { parser::Token::Float, "float" },
    { parser::Token::For, "for" },
    { parser::Token::Friend, "friend" },
    { parser::Token::If, "if" },
    { parser::Token::Import, "import" },
    { parser::Token::Int, "int" },
    { parser::Token::Mutable, "mutable" },
    { parser::Token::Namespace, "namespace" },
    { parser::Token::Operator, "operator" },
    { parser::Token::Private, "private" },
    { parser::Token::Protected, "protected" },
    { parser::Token::Public, "public" },
    { parser::Token::Return, "return" },
    { parser::Token::Static, "static" },
    { parser::Token::Struct, "struct" },
    { parser::Token::Template, "template" },
    { parser::Token::This, "this" },
    { parser::Token::True, "true" },
    { parser::Token::Typedef, "typedef" },
    { parser::Token::Typeid, "typeid" },
    { parser::Token::Typename, "typename" },
    { parser::Token::Using, "using" },
    { parser::Token::Virtual, "virtual" },
    { parser::Token::Void, "void" },
    { parser::Token::While, "while" },
    //Operators
    { parser::Token::ScopeResolution, "::" },
    { parser::Token::PlusPlus, "++" },
    { parser::Token::MinusMinus, "--" },
    { parser::Token::Plus, "+" },
    { parser::Token::Minus, "-" },
    { parser::Token::LogicalNot, "!" },
    { parser::Token::BitwiseNot, "~" },
    { parser::Token::Mul, "*" },
    { parser::Token::Div, "/" },
    { parser::Token::Remainder, "%" },
    { parser::Token::LeftShift, "<<" },
    { parser::Token::RightShift, ">>" },
    { parser::Token::Less, "<" },
    { parser::Token::GreaterThan, ">" },
    { parser::Token::LessEqual, "<=" },
    { parser::Token::GreaterThanEqual, ">=" },
    { parser::Token::EqEq, "==" },
    { parser::Token::Neq, "!=" },
    { parser::Token::BitwiseAnd, "&" },
    { parser::Token::BitwiseOr, "|" },
    { parser::Token::BitwiseXor, "^" },
    { parser::Token::LogicalAnd, "&&" },
    { parser::Token::LogicalOr, "||" },
    { parser::Token::Eq, "=" },
    { parser::Token::MulEq, "*=" },
    { parser::Token::DivEq, "/=" },
    { parser::Token::AddEq, "+=" },
    { parser::Token::SubEq, "-=" },
    { parser::Token::RemainderEq, "%=" },
    { parser::Token::LeftShiftEq, "<<=" },
    { parser::Token::RightShiftEq, ">>=" },
    { parser::Token::BitAndEq, "&=" },
    { parser::Token::BitOrEq, "|=" },
    { parser::Token::BitXorEq, "^=" },
    { parser::Token::Comma, "," },
    { parser::Token::LeftRightPar, "()" },
    { parser::Token::LeftRightBracket, "[]" },
    { parser::Token::Zero, "0" },
  });

  ret->setMessages({
    { ErrorCode::P_UnexpectedEndOfInput, "Unexpected end of input" },
    { ErrorCode::P_UnexpectedFragmentEnd, "Unexpected end of fragment" },
    { ErrorCode::P_UnexpectedToken, "expected %2 got %1||unexpected token %1" },
    { ErrorCode::P_InvalidEmptyBrackets, "Invalid empty brackets" },
    { ErrorCode::P_IllegalUseOfKeyword, "keyword not allowed in this context" },
    { ErrorCode::P_ExpectedIdentifier, "expected identifier got %1" },
    { ErrorCode::P_ExpectedUserDefinedName, "expected user-defined name got %1" },
    { ErrorCode::P_ExpectedLiteral, "expected literal name got %1" },
    { ErrorCode::P_ExpectedOperatorSymbol, "Expected 'operator' to be followed by operator symbol, got %1" },
    { ErrorCode::P_InvalidEmptyOperand, "invalid empty parentheses" },
    { ErrorCode::P_ExpectedDeclaration, "expected a declaration" },
    { ErrorCode::P_MissingConditionalColon, "Incomplete conditional expression (read '?' but could not find ':')" },
    { ErrorCode::P_CouldNotParseLambdaCapture, "Could not read lambda-capture" },
    { ErrorCode::P_ExpectedCurrentClassName, "expected current class name" },
    { ErrorCode::P_CouldNotReadType, "could not read type" },
  });

  ret->setMessages({
    { ErrorCode::RuntimeError, "RuntimeError:" },
    { ErrorCode::C_IllegalUseOfThis, "Illegal use of this" },
    { ErrorCode::C_ObjectHasNoDestructor, "Object has no destructor" },
    { ErrorCode::C_InvalidUseOfDelegatedConstructor, "No other member initializer may be present when using delegating constructors" },
    { ErrorCode::C_NotDataMember, "%1 does not name a data member" },
    { ErrorCode::C_InheritedDataMember, "cannot initialize inherited data member %1" },
    { ErrorCode::C_DataMemberAlreadyHasInitializer, "data member %1 already has an intializer" },
    { ErrorCode::C_NoDelegatingConstructorFound, "Could not find a delegate constructor" },
    { ErrorCode::C_CouldNotFindValidBaseConstructor, "Could not find valid base constructor" },
    { ErrorCode::C_InitializerListAsFirstArrayElement, "An initializer list cannot be used as the first element of an array" },
    { ErrorCode::C_ReturnStatementWithoutValue, "Cannot have return-statement without a value in function returning non-void" },
    { ErrorCode::C_ReturnStatementWithValue, "A function returning void cannot return a value" },
    { ErrorCode::C_ReferencesMustBeInitialized, "References must be initialized" },
    { ErrorCode::C_EnumerationsCannotBeDefaultConstructed, "Enumerations cannot be default constructed" },
    { ErrorCode::C_EnumerationsMustBeInitialized, "Variables of enumeration type must be initialized" },
    { ErrorCode::C_FunctionVariablesMustBeInitialized, "Variables of function-type must be initialized" },
    { ErrorCode::C_VariableCannotBeDefaultConstructed, "Class '%1' does not provide a default constructor" },
    { ErrorCode::C_ClassHasDeletedDefaultCtor, "Class '%1' has a deleted default constructor" },
    { ErrorCode::C_VariableCannotBeDestroyed, "Class '%1' does not provide a destructor" },
    { ErrorCode::C_CouldNotResolveOperatorName, "Could not resolve operator name based on parameter count and operator symbol." },
    { ErrorCode::C_InvalidParamCountInOperatorOverload, "Invalid parameter count found in operator overload, expected %1 got %2" },
    { ErrorCode::C_OpOverloadMustBeDeclaredAsMember, "This operator can only be overloaded as a member" },
    { ErrorCode::C_InvalidTypeName, "%1 does not name a type" },
    { ErrorCode::C_DeclarationProcessingError, "Some declarations could not be processed." },
    { ErrorCode::C_DataMemberCannotBeAuto, "Data members cannot be declared 'auto'." },
    { ErrorCode::C_MissingStaticInitialization, "A static variable must be initialized." },
    { ErrorCode::C_InvalidStaticInitialization, "Static variables can only be initialized through assignment." },
    { ErrorCode::C_FailedToInitializeStaticVariable, "Failed to initialized static variable." },
    { ErrorCode::C_InvalidBaseClass, "Invalid base class." },
    { ErrorCode::C_InvalidUseOfDefaultArgument, "Cannot have a parameter without a default value after one was provided." },
    { ErrorCode::C_ArrayElementNotConvertible, "Could not convert element to array's element type." },
    { ErrorCode::C_ArraySubscriptOnNonObject, "Cannot perform array subscript on non object type." },
    { ErrorCode::C_CouldNotFindValidSubscriptOperator, "Could not find valid subscript operator." },
    { ErrorCode::C_CannotCaptureThis, "'this' cannot be captured outside of a member function." },
    { ErrorCode::C_UnknownCaptureName, "Could not capture any local variable with given name." },
    { ErrorCode::C_CannotCaptureNonCopyable, "Cannot capture by value a non copyable type." },
    { ErrorCode::C_SomeLocalsCannotBeCaptured, "Some local variables cannot be captured by value." },
    { ErrorCode::C_CannotCaptureByValueAndByRef, "Cannot capture both everything by reference and by value." },
    { ErrorCode::C_LambdaMustBeCaptureless, "A lambda must be captureless within this context." },
    { ErrorCode::C_CouldNotFindValidConstructor, "Could not find valid constructor." },
    { ErrorCode::C_CouldNotFindValidMemberFunction, "Could not find valid member function for call." },
    { ErrorCode::C_CouldNotFindValidOperator, "Could not find valid operator overload." },
    { ErrorCode::C_CouldNotFindValidOverload, "Overload resolution failed." },
    { ErrorCode::C_CouldNotFindValidCallOperator, "Could not find valid operator() overload for call." },
    { ErrorCode::C_AmbiguousFunctionName, "Name does not refer to a single function" },
    { ErrorCode::C_TemplateNamesAreNotExpressions, "Name refers to a template and cannot be used inside an expression" },
    { ErrorCode::C_TypeNameInExpression, "Name refers to a type and cannot be used inside an expression" },
    { ErrorCode::C_NamespaceNameInExpression, "Name refers to a namespace and cannot be used inside an expression" },
    { ErrorCode::C_TooManyArgumentInVariableInitialization, "Too many arguments provided in variable initialization." },
    { ErrorCode::C_TooManyArgumentInInitialization, "Too many arguments provided in initialization." },
    { ErrorCode::C_TooManyArgumentInReferenceInitialization, "More than one argument provided in reference initialization." },
    { ErrorCode::C_TooManyArgumentsInMemberInitialization, "Too many arguments in member initialization." },
    { ErrorCode::C_CouldNotConvert, "Could not convert from %1 to %2" },
    { ErrorCode::C_CouldNotFindCommonType, "Could not find common type of %1 and %2 in conditionnal expression" },
    { ErrorCode::C_CannotAccessMemberOfNonObject, "Cannot access member of non object type." },
    { ErrorCode::C_NoSuchMember, "Object has no such member." },
    { ErrorCode::C_InvalidTemplateArgument, "Invalid template argument." },
    { ErrorCode::C_InvalidLiteralTemplateArgument, "Only integer and boolean literals can be used as template arguments." },
    { ErrorCode::C_NonConstExprTemplateArgument, "Template arguments must be constant expressions." },
    { ErrorCode::C_InvalidTemplateArgumentType, "This constant epression does not evaluate to an int or a bool." },
    { ErrorCode::C_MissingNonDefaultedTemplateParameter, "Missing non-defaulted template parameter." },
    { ErrorCode::C_CouldNotFindPrimaryClassTemplate, "Could not find primary class template (must be declared in the same namespace)." },
    { ErrorCode::C_CouldNotFindPrimaryFunctionTemplate, "Could not find primary function template (must be declared in the same namespace)." },
    { ErrorCode::C_InvalidUseOfConstKeyword, "Invalid use of const keyword." },
    { ErrorCode::C_InvalidUseOfExplicitKeyword, "Invalid use of 'explicit' keyword." },
    { ErrorCode::C_InvalidUseOfStaticKeyword, "Invalid use of static keyword." },
    { ErrorCode::C_InvalidUseOfVirtualKeyword, "Invalid use of virtual keyword." },
    { ErrorCode::C_AutoMustBeUsedWithAssignment, "'auto' can only be used with assignment initialization." },
    { ErrorCode::C_CannotDeduceLambdaReturnType, "Cannot deduce lambda return type" },
    { ErrorCode::C_CallToDeletedFunction, "Call to deleted function." },
    { ErrorCode::C_FunctionCannotBeDefaulted, "Function cannot be defaulted." },
    { ErrorCode::C_ParentHasNoDefaultConstructor, "Cannot generate defaulted default constructor because parent has no default constructor." },
    { ErrorCode::C_ParentHasDeletedDefaultConstructor, "Cannot generate defaulted default constructor because parent default constructor is deleted." },
    { ErrorCode::C_ParentHasNoCopyConstructor, "Cannot generate defaulted copy constructor because parent has no copy constructor." },
    { ErrorCode::C_ParentHasDeletedCopyConstructor, "Cannot generate defaulted copy constructor because parent copy constructor is deleted." },
    { ErrorCode::C_DataMemberIsNotCopyable, "Cannot generate defaulted copy constructor because at least one data member is not copyable." },
    { ErrorCode::C_ParentHasNoMoveConstructor, "Cannot generate defaulted move constructor because parent has no move constructor." },
    { ErrorCode::C_ParentHasDeletedMoveConstructor, "Cannot generate defaulted move constructor because parent move constructor is deleted." },
    { ErrorCode::C_DataMemberIsNotMovable, "Cannot generate defaulted move constructor because at least one data member is not movable." },
    { ErrorCode::C_ParentHasNoAssignmentOperator, "Cannot generate defaulted assignment operator because parent has no assignment operator." },
    { ErrorCode::C_ParentHasDeletedAssignmentOperator, "Cannot generate defaulted assignment operator because parent has a deleted assignment operator." },
    { ErrorCode::C_DataMemberHasNoAssignmentOperator, "Cannot generate defaulted assignment operator because at least one data member has no assignment operator." },
    { ErrorCode::C_DataMemberHasDeletedAssignmentOperator, "Cannot generate defaulted assignment operator because at least one data member has a deleted assignment operator." },
    { ErrorCode::C_DataMemberIsReferenceAndCannotBeAssigned, "Cannot generate defaulted assignment operator because at least one data member is a reference." },
    { ErrorCode::C_InvalidArgumentCountInDataMemberRefInit, "Only one value must be provided to initialize a data member of reference type." },
    { ErrorCode::C_CannotInitializeNonConstRefDataMemberWithConst, "Cannot initialize a data member of non-const reference type with a const value." },
    { ErrorCode::C_BadDataMemberRefInit, "Bad reference initialization of data member %1." },
    { ErrorCode::C_EnumMemberCannotBeDefaultConstructed, "Data member %1 of enumeration type %2 cannot be default constructed." },
    { ErrorCode::C_DataMemberHasNoDefaultConstructor, "Data member %1 of type %2 has no default constructor." },
    { ErrorCode::C_DataMemberHasDeletedDefaultConstructor, "Data member %1 of type %2 has a deleted default constructor." },
    { ErrorCode::C_InvalidCharacterLiteral, "A character literal must contain only one character." },
    { ErrorCode::C_CouldNotFindValidLiteralOperator, "Could not find valid literal operator." },
    { ErrorCode::C_UnknownTypeInBraceInitialization, "Unknown type %1 in brace initialization" },
    { ErrorCode::C_NarrowingConversionInBraceInitialization, "Narrowing conversion from %1 to %2 in brace initialization" },
    { ErrorCode::C_NamespaceDeclarationCannotAppearAtThisLevel, "Namespace declarations cannot appear at this level" },
    { ErrorCode::C_ExpectedDeclaration, "Expected a declaration." },
    { ErrorCode::C_GlobalVariablesCannotBeAuto, "Global variables cannot be declared with auto." },
    { ErrorCode::C_GlobalVariablesMustBeInitialized, "Global variables must have an initializer." },
    { ErrorCode::C_GlobalVariablesMustBeAssigned, "Global variables must be initialized through assignment." },
    { ErrorCode::C_InaccessibleMember, "%1 is %2 within this context" },
    { ErrorCode::C_FriendMustBeAClass, "Friend must be a class" },
    { ErrorCode::C_UnknownModuleName, "Unknown module '%1'" },
    { ErrorCode::C_UnknownSubModuleName, "'%1' is not a submodule of '%2'" },
    { ErrorCode::C_ModuleImportationError, "Failed to import module '%1'\n%2" },
    { ErrorCode::C_InvalidNameInUsingDirective, "%1 does not name a namespace" },
    { ErrorCode::C_NoSuchCallee, "Callee was not declared in this scope" },
    { ErrorCode::C_LiteralOperatorNotInNamespace, "Literal operators can only appear at namespace level" },
  });

  return ret;
}

static const std::shared_ptr<MessageDatabase> gMessageDatabase = build_message_database();


bool isCompilerError(ErrorCode code)
{
  return static_cast<int>(ErrorCode::FirstCompilerError) <= static_cast<int>(code)
    && static_cast<int>(code) <= static_cast<int>(ErrorCode::LastCompilerError);
}

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

Message::Message(Severity severity, ErrorCode code)
  : mLine(-1)
  , mColumn(-1)
  , mSeverity(severity)
  , mCode(code)
{

}

Message::Message(const std::string & str, Severity severity, ErrorCode code)
  : mLine(-1)
  , mColumn(-1)
  , mSeverity(severity)
  , mCode(code)
  , mContent(str)
{

}

Message::Message(std::string && str, Severity severity, ErrorCode code)
  : mLine(-1)
  , mColumn(-1)
  , mSeverity(severity)
  , mCode(code)
  , mContent(std::move(str))
{

}

std::string Message::message() const
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

  if (mCode != ErrorCode::NoError)
  {
    /// TODO: do something with that ?
  }


  if (mLine >= 0)
  {
    result += std::to_string(mLine);
    result += std::string{ ":" };
    if (mColumn >= 0)
      result += std::to_string(mColumn) + std::string{ ":" };
  }

  result += " ";

  result += mContent;

  return result;
}

const std::string & Message::content() const
{
  return mContent;
}

Message & Message::operator=(Message && other)
{
  this->mCode = other.mCode;
  other.mCode = ErrorCode::NoError;
  this->mSeverity = other.mSeverity;
  this->mLine = other.mLine;
  this->mColumn = other.mColumn;
  this->mContent = std::move(other.mContent);
  return *(this);
}

line_t line(int l)
{
  return line_t{ l };
}

pos_t pos(int l, int col)
{
  return pos_t{ l, col };
}

MessageBuilder::MessageBuilder(Severity s, Engine *e)
  : mEngine(e)
  , mSeverity(s)
  , mCode(ErrorCode::NoError)
  , mLine(-1)
  , mColumn(-1)
{

}

std::string MessageBuilder::repr(script::AccessSpecifier as)
{
  if (as == AccessSpecifier::Protected)
    return "protected";
  else if (as == AccessSpecifier::Private)
    return "private";
  return "public";
}

std::string MessageBuilder::repr(script::OperatorName op)
{
  return script::Operator::getFullName(op);
}

std::string MessageBuilder::repr(const Type & t) const
{
  if (mEngine == nullptr)
    return diagnostic::format("Type<%1>", repr(t.data()));

  std::string result = mEngine->typeSystem()->typeName(t);
  if (t.isConst())
    result = std::string{ "const " } +result;
  if (t.isReference())
    result = result + std::string{ " &" };
  else if (t.isRefRef())
    result = result + std::string{ " &&" };
  return result;
}

std::string MessageBuilder::repr(const parser::Token & tok) const
{
  if (tok.type != parser::Token::UserDefinedLiteral && tok.type != parser::Token::UserDefinedName)
    return repr(tok.type);

  return std::string{ "Not implemented (print token)" };
}

const std::string & MessageBuilder::repr(const parser::Token::Type & tok) const
{
  return gMessageDatabase->tokens.at(static_cast<int>(tok) & 0xFFFF);
}

MessageBuilder & MessageBuilder::operator<<(int n)
{
  mBuffer += std::to_string(n);
  return *(this);
}

MessageBuilder& MessageBuilder::operator<<(size_t n)
{
  mBuffer += std::to_string(n);
  return *(this);
}

MessageBuilder & MessageBuilder::operator<<(line_t l)
{
  mLine = l.line;
  return *(this);
}

MessageBuilder & MessageBuilder::operator<<(pos_t p)
{
  mLine = p.line;
  mColumn = p.column;
  return *(this);
}

MessageBuilder & MessageBuilder::operator<<(const std::string & str)
{
  mBuffer += str;
  return *(this);
}

MessageBuilder & MessageBuilder::operator<<(std::string && str)
{
  mBuffer += std::move(str);
  return *(this);
}

MessageBuilder & MessageBuilder::operator<<(const Exception & ex)
{
  if (ex.is<parser::ParserException>())
    return *(this) << ex.as<parser::ParserException>();
  else if (ex.is<compiler::CompilerException>())
    return *(this) << ex.as<compiler::CompilerException>();
  
  mCode = ex.code();
  mBuffer += "Not implemented (print exception)";

  return *(this);
}

MessageBuilder & MessageBuilder::operator<<(const parser::ParserException & ex)
{
  mCode = ex.code();
  mBuffer += "ParserError: ";

  std::string fmt = gMessageDatabase->messages[static_cast<int>(ex.code())];

  switch (ex.code())
  {
  case ErrorCode::P_UnexpectedToken:
  {
    const auto & unexpected_token = ex.as<parser::UnexpectedToken>();
    (*this) << diagnostic::pos_t{ unexpected_token.actual.line, unexpected_token.actual.column };
    if (unexpected_token.expected != parser::Token::Invalid)
      fmt = fmt.substr(0, fmt.find("||"));
    else
      fmt = fmt.substr(fmt.find("||") + 2);
    mBuffer += format(fmt, unexpected_token.actual, unexpected_token.expected);
    break;
  }
  default:
    mBuffer += fmt;
    break;
  }

  return *this;
}

#define CASE_1(Name, m1) case ErrorCode::C_##Name: \
  mBuffer += format(fmt, ex.as<compiler::Name>().m1); \
  break

#define CASE_2(Name, m1, m2) case ErrorCode::C_##Name: \
  mBuffer += format(fmt, ex.as<compiler::Name>().m1, ex.as<compiler::Name>().m2); \
  break

#define CASE_3(Name, m1, m2, m3) case ErrorCode::C_##Name: \
  mBuffer += format(fmt, ex.as<compiler::Name>().m1, ex.as<compiler::Name>().m2, ex.as<compiler::Name>().m3); \
  break

MessageBuilder & MessageBuilder::operator<<(const compiler::CompilerException & ex)
{
  (*this) << ex.pos;
  mCode = ex.code();

  const std::string & fmt = gMessageDatabase->messages[static_cast<int>(ex.code())];

  switch (ex.code())
  {
  CASE_1(BadDataMemberRefInit, name);
  CASE_1(ClassHasDeletedDefaultCtor, type);
  CASE_2(CouldNotConvert, source, destination);
  CASE_2(CouldNotFindCommonType, first, second);
  CASE_1(DataMemberAlreadyHasInitializer, name);
  CASE_2(DataMemberHasDeletedDefaultConstructor, type, name);
  CASE_2(DataMemberHasNoDefaultConstructor, type, name);
  CASE_2(EnumMemberCannotBeDefaultConstructed, type, name);
  CASE_2(InaccessibleMember, name, access);
  CASE_1(InheritedDataMember, name);
  CASE_1(InvalidNameInUsingDirective, name);
  CASE_2(InvalidParamCountInOperatorOverload, expected, actual);
  CASE_1(InvalidTypeName, name);
  CASE_2(ModuleImportationError, name, message);
  CASE_2(NarrowingConversionInBraceInitialization, source, destination);
  CASE_1(NotDataMember, name);
  CASE_1(OpOverloadMustBeDeclaredAsMember, name);
  CASE_1(UnknownModuleName, name);
  CASE_2(UnknownSubModuleName, name, moduleName);
  CASE_1(UnknownTypeInBraceInitialization, name);
  CASE_1(VariableCannotBeDefaultConstructed, type);
  CASE_1(VariableCannotBeDestroyed, type);
  default:
    mBuffer += fmt;
    break;
  }

  return *this;
}

#undef CASE_1
#undef CASE_2
#undef CASE_3

Message MessageBuilder::build() const
{
  Message result{ std::move(mBuffer), mSeverity, mCode };
  result.mLine = mLine;
  result.mColumn = mColumn;
  return result;
}

MessageBuilder::operator Message() const
{
  return build();
}


MessageBuilder info(Engine *e)
{
  return MessageBuilder{ Info, e };
}

MessageBuilder warning(Engine *e)
{
  return MessageBuilder{ Warning, e };
}

MessageBuilder error(Engine *e)
{
  return MessageBuilder{ Error, e };
}

} // namespace diagnostic

} // namespace script
