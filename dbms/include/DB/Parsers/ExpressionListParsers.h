#pragma once

#include <list>

#include <DB/Parsers/IParserBase.h>
#include <DB/Parsers/CommonParsers.h>


namespace DB
{

/** Идущие подряд пары строк: оператор и соответствующая ему функция. Например, "+" -> "plus".
  * Порядок парсинга операторов имеет значение.
  */
using Operators_t = const char **;


/** Список элементов, разделённых чем-либо. */
class ParserList : public IParserBase
{
public:
	ParserList(ParserPtr && elem_parser_, ParserPtr && separator_parser_, bool allow_empty_ = true)
		: elem_parser(std::move(elem_parser_)), separator_parser(std::move(separator_parser_)), allow_empty(allow_empty_)
	{
	}
protected:
	const char * getName() const { return "list of elements"; }
	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
private:
	ParserPtr elem_parser;
	ParserPtr separator_parser;
	bool allow_empty;
};


/** Выражение с инфиксным бинарным лево-ассоциативным оператором.
  * Например, a + b - c + d.
  */
class ParserLeftAssociativeBinaryOperatorList : public IParserBase
{
private:
	Operators_t operators;
	ParserPtr first_elem_parser;
	ParserPtr remaining_elem_parser;

public:
	/** operators_ - допустимые операторы и соответствующие им функции
	  */
	ParserLeftAssociativeBinaryOperatorList(Operators_t operators_, ParserPtr && first_elem_parser_)
		: operators(operators_), first_elem_parser(std::move(first_elem_parser_))
	{
	}

	ParserLeftAssociativeBinaryOperatorList(Operators_t operators_, ParserPtr && first_elem_parser_,
		ParserPtr && remaining_elem_parser_)
		: operators(operators_), first_elem_parser(std::move(first_elem_parser_)),
		  remaining_elem_parser(std::move(remaining_elem_parser_))
	{
	}

protected:
	const char * getName() const { return "list, delimited by binary operators"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
};


/** Выражение с инфиксным оператором произвольной арности.
  * Например, a AND b AND c AND d.
  */
class ParserVariableArityOperatorList : public IParserBase
{
private:
	ParserString infix_parser;
	const char * function_name;
	ParserPtr elem_parser;

public:
	ParserVariableArityOperatorList(const char * infix_, const char * function_, ParserPtr && elem_parser_)
		: infix_parser(infix_, true, true), function_name(function_), elem_parser(std::move(elem_parser_))
	{
	}

protected:
	const char * getName() const { return "list, delimited by operator of variable arity"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
};


/** Выражение с префиксным унарным оператором.
  * Например, NOT x.
  */
class ParserPrefixUnaryOperatorExpression : public IParserBase
{
private:
	Operators_t operators;
	ParserPtr elem_parser;

public:
	/** operators_ - допустимые операторы и соответствующие им функции
	  */
	ParserPrefixUnaryOperatorExpression(Operators_t operators_, ParserPtr && elem_parser_)
		: operators(operators_), elem_parser(std::move(elem_parser_))
	{
	}

protected:
	const char * getName() const { return "expression with prefix unary operator"; }
	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
};


class ParserTupleElementExpression : public IParserBase
{
private:
	static const char * operators[];

protected:
	const char * getName() const { return "tuple element expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
};


class ParserArrayElementExpression : public IParserBase
{
private:
	static const char * operators[];

protected:
	const char * getName() const { return "array element expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
};


class ParserUnaryMinusExpression : public IParserBase
{
private:
	static const char * operators[];
	ParserPrefixUnaryOperatorExpression operator_parser {operators, std::make_unique<ParserTupleElementExpression>()};

protected:
	const char * getName() const { return "unary minus expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
};


class ParserMultiplicativeExpression : public IParserBase
{
private:
	static const char * operators[];
	ParserLeftAssociativeBinaryOperatorList operator_parser {operators, std::make_unique<ParserUnaryMinusExpression>()};

protected:
	const char * getName() const { return "multiplicative expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected)
	{
		return operator_parser.parse(pos, end, node, max_parsed_pos, expected);
	}
};


class ParserAdditiveExpression : public IParserBase
{
private:
	static const char * operators[];
	ParserLeftAssociativeBinaryOperatorList operator_parser {operators, std::make_unique<ParserMultiplicativeExpression>()};

protected:
	const char * getName() const { return "additive expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected)
	{
		return operator_parser.parse(pos, end, node, max_parsed_pos, expected);
	}
};


class ParserConcatExpression : public IParserBase
{
	ParserVariableArityOperatorList operator_parser {"||", "concat", std::make_unique<ParserAdditiveExpression>()};

protected:
	const char * getName() const { return "string concatenation expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected)
	{
		return operator_parser.parse(pos, end, node, max_parsed_pos, expected);
	}
};


class ParserBetweenExpression : public IParserBase
{
private:
	ParserConcatExpression elem_parser;

protected:
	const char * getName() const { return "BETWEEN expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
};


class ParserComparisonExpression : public IParserBase
{
private:
	static const char * operators[];
	ParserLeftAssociativeBinaryOperatorList operator_parser {operators, std::make_unique<ParserBetweenExpression>()};

protected:
	const char * getName() const { return "comparison expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected)
	{
		return operator_parser.parse(pos, end, node, max_parsed_pos, expected);
	}
};


/** Parser for nullity checking with IS (NOT) NULL.
  */
class ParserNullityChecking : public IParserBase
{
protected:
	const char * getName() const override { return "nullity checking"; }
	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected) override;
};


class ParserLogicalNotExpression : public IParserBase
{
private:
	static const char * operators[];
	ParserPrefixUnaryOperatorExpression operator_parser {operators, std::make_unique<ParserNullityChecking>()};

protected:
	const char * getName() const { return "logical-NOT expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected)
	{
		return operator_parser.parse(pos, end, node, max_parsed_pos, expected);
	}
};


class ParserLogicalAndExpression : public IParserBase
{
private:
	ParserVariableArityOperatorList operator_parser {"AND", "and", std::make_unique<ParserLogicalNotExpression>()};

protected:
	const char * getName() const { return "logical-AND expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected)
	{
		return operator_parser.parse(pos, end, node, max_parsed_pos, expected);
	}
};


class ParserLogicalOrExpression : public IParserBase
{
private:
	ParserVariableArityOperatorList operator_parser {"OR", "or", std::make_unique<ParserLogicalAndExpression>()};

protected:
	const char * getName() const { return "logical-OR expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected)
	{
		return operator_parser.parse(pos, end, node, max_parsed_pos, expected);
	}
};


/** Выражение с тернарным оператором.
  * Например, a = 1 ? b + 1 : c * 2.
  */
class ParserTernaryOperatorExpression : public IParserBase
{
private:
	ParserLogicalOrExpression elem_parser;

protected:
	const char * getName() const { return "expression with ternary operator"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
};


class ParserLambdaExpression : public IParserBase
{
private:
	ParserTernaryOperatorExpression elem_parser;

protected:
	const char * getName() const { return "lambda expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
};


class ParserExpressionWithOptionalAlias : public IParserBase
{
public:
	ParserExpressionWithOptionalAlias(bool allow_alias_without_as_keyword);
protected:
	ParserPtr impl;

	const char * getName() const { return "expression with optional alias"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected)
	{
		return impl->parse(pos, end, node, max_parsed_pos, expected);
	}
};

class ParserExpressionInCastExpression : public IParserBase
{
public:
	ParserExpressionInCastExpression(bool allow_alias_without_as_keyword);
protected:
	ParserPtr impl;

	const char * getName() const { return "expression in CAST expression"; }

	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected)
	{
		return impl->parse(pos, end, node, max_parsed_pos, expected);
	}
};


/** Список выражений, разделённых запятыми, возможно пустой. */
class ParserExpressionList : public IParserBase
{
public:
	ParserExpressionList(bool allow_alias_without_as_keyword_)
		: allow_alias_without_as_keyword(allow_alias_without_as_keyword_) {}

protected:
	bool allow_alias_without_as_keyword;

	const char * getName() const { return "list of expressions"; }
	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
};


class ParserNotEmptyExpressionList : public IParserBase
{
public:
	ParserNotEmptyExpressionList(bool allow_alias_without_as_keyword)
		: nested_parser(allow_alias_without_as_keyword) {}
private:
	ParserExpressionList nested_parser;
protected:
	const char * getName() const { return "not empty list of expressions"; }
	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
};


class ParserOrderByExpressionList : public IParserBase
{
protected:
	const char * getName() const { return "order by expression"; }
	bool parseImpl(Pos & pos, Pos end, ASTPtr & node, Pos & max_parsed_pos, Expected & expected);
};


}
