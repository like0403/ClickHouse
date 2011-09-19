#pragma once

#include <Poco/RegularExpression.h>

#include <DB/DataTypes/IDataType.h>


namespace DB
{

using Poco::SharedPtr;


/** Позволяет создать тип данных по его имени. 
  */
class DataTypeFactory
{
public:
	DataTypeFactory();
	DataTypePtr get(const String & name) const;

private:
	typedef std::map<String, DataTypePtr> NonParametricDataTypes;
	NonParametricDataTypes non_parametric_data_types;

	Poco::RegularExpression fixed_string_regexp;
};

typedef SharedPtr<DataTypeFactory> DataTypeFactoryPtr;


}
