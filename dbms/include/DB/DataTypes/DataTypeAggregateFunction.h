#pragma once

#include <DB/AggregateFunctions/IAggregateFunction.h>
#include <DB/AggregateFunctions/AggregateFunctionFactory.h>

#include <DB/DataTypes/IDataType.h>


namespace DB
{

using Poco::SharedPtr;

/** Тип - состояние агрегатной функции.
  */
class DataTypeAggregateFunction : public IDataType
{
private:
	const AggregateFunctionFactoryPtr factory;
	
public:
	DataTypeAggregateFunction() : factory(new AggregateFunctionFactory) {}
	DataTypeAggregateFunction(const AggregateFunctionFactoryPtr & factory_) : factory(factory_) {}
	
	std::string getName() const { return "AggregateFunction"; }
	DataTypePtr clone() const { return new DataTypeAggregateFunction(factory); }

	void serializeBinary(const Field & field, WriteBuffer & ostr) const;
	void deserializeBinary(Field & field, ReadBuffer & istr) const;
	void serializeBinary(const IColumn & column, WriteBuffer & ostr) const;
	void deserializeBinary(IColumn & column, ReadBuffer & istr, size_t limit) const;
	void serializeText(const Field & field, WriteBuffer & ostr) const;
	void deserializeText(Field & field, ReadBuffer & istr) const;
	void serializeTextEscaped(const Field & field, WriteBuffer & ostr) const;
	void deserializeTextEscaped(Field & field, ReadBuffer & istr) const;
	void serializeTextQuoted(const Field & field, WriteBuffer & ostr, bool compatible = false) const;
	void deserializeTextQuoted(Field & field, ReadBuffer & istr, bool compatible = false) const;

	ColumnPtr createColumn() const;
	ColumnPtr createConstColumn(size_t size, const Field & field) const;
};


}

