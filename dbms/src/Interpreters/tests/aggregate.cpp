#include <iostream>
#include <iomanip>

#include <Poco/Stopwatch.h>

#include <DB/DataTypes/DataTypesNumberFixed.h>
#include <DB/DataTypes/DataTypeString.h>

#include <DB/Columns/ColumnsNumber.h>
#include <DB/Columns/ColumnString.h>

#include <DB/DataStreams/IBlockInputStream.h>

#include <DB/Interpreters/Aggregate.h>

#include <DB/AggregateFunctions/AggregateFunctionFactory.h>


class OneBlockInputStream : public DB::IBlockInputStream
{
private:
	const DB::Block & block;
	bool has_been_read;
public:
	OneBlockInputStream(const DB::Block & block_) : block(block_), has_been_read(false) {}

	DB::Block read()
	{
		if (!has_been_read)
		{
			has_been_read = true;
			return block;
		}
		else
			return DB::Block();
	}

	DB::String getName() const { return "OneBlockInputStream"; }
};


int main(int argc, char ** argv)
{
	try
	{
		size_t n = argc == 2 ? atoi(argv[1]) : 10;

		DB::Block block;
		
		DB::ColumnWithNameAndType column_x;
		column_x.name = "x";
		column_x.type = new DB::DataTypeInt16;
		DB::ColumnInt16 * x = new DB::ColumnInt16;
		column_x.column = x;
		std::vector<Int16> & vec_x = x->getData();

		vec_x.resize(n);
		for (size_t i = 0; i < n; ++i)
			vec_x[i] = i % 9;

		block.insert(column_x);

		const char * strings[] = {"abc", "def", "abcd", "defg", "ac"};

		DB::ColumnWithNameAndType column_s1;
		column_s1.name = "s1";
		column_s1.type = new DB::DataTypeString;
		column_s1.column = new DB::ColumnString;

		for (size_t i = 0; i < n; ++i)
			column_s1.column->insert(strings[i % 5]);

		block.insert(column_s1);

		DB::ColumnWithNameAndType column_s2;
		column_s2.name = "s2";
		column_s2.type = new DB::DataTypeString;
		column_s2.column = new DB::ColumnString;

		for (size_t i = 0; i < n; ++i)
			column_s2.column->insert(strings[i % 3]);

		block.insert(column_s2);

		DB::BlockInputStreamPtr stream = new OneBlockInputStream(block);
		DB::AggregatedData aggregated_data;

		DB::ColumnNumbers key_column_numbers;
		key_column_numbers.push_back(0);
		key_column_numbers.push_back(1);

		DB::AggregateFunctionFactory factory;

		DB::AggregateDescriptions aggregate_descriptions(1);
		aggregate_descriptions[0].function = factory.get("count");

		DB::Aggregate aggregator(key_column_numbers, aggregate_descriptions);
		
		{
			Poco::Stopwatch stopwatch;
			stopwatch.start();

			aggregated_data = aggregator.execute(stream);

			stopwatch.stop();
			std::cout << std::fixed << std::setprecision(2)
				<< "Elapsed " << stopwatch.elapsed() / 1000000.0 << " sec."
				<< ", " << n * 1000000 / stopwatch.elapsed() << " rows/sec."
				<< std::endl;
		}

		for (DB::AggregatedData::const_iterator it = aggregated_data.begin(); it != aggregated_data.end(); ++it)
		{
			for (DB::Row::const_iterator jt = it->first.begin(); jt != it->first.end(); ++jt)
				std::cout << boost::apply_visitor(DB::FieldVisitorToString(), *jt) << '\t';

			for (DB::AggregateFunctions::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt)
			{
				DB::Field result = (*jt)->getResult();
				std::cout << boost::apply_visitor(DB::FieldVisitorToString(), result) << '\t';
			}

			std::cout << '\n';
		}
	}
	catch (const DB::Exception & e)
	{
		std::cerr << e.message() << std::endl;
	}

	return 0;
}
