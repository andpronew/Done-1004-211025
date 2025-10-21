#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/result.h>
#include <arrow/csv/api.h>

#include <parquet/arrow/reader.h>
#include <parquet/exception.h>

using namespace std;

arrow::Status write_table_to_csv(const shared_ptr<arrow::Table>& table, const string& output_file)
{
    ARROW_ASSIGN_OR_RAISE(auto output, arrow::io::FileOutputStream::Open(output_file));

    arrow::csv::WriteOptions options = arrow::csv::WriteOptions::Defaults();
    options.include_header = true;

    return arrow::csv::WriteCSV(*table, options, arrow::default_memory_pool(), output.get());
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " input.parquet output.csv" << endl;
        return 1;
    }

    string parquet_file = argv[1];
    string csv_file = argv[2];

    try
    {
        ARROW_ASSIGN_OR_RAISE(auto infile, arrow::io::ReadableFile::Open(parquet_file));

        PARQUET_ASSIGN_OR_THROW(
            unique_ptr<parquet::arrow::FileReader> reader,
            parquet::arrow::OpenFile(infile, arrow::default_memory_pool())
        );

        shared_ptr<arrow::Table> table;
        ARROW_RETURN_NOT_OK(reader->ReadTable(&table));

        arrow::Status status = write_table_to_csv(table, csv_file);
        if (!status.ok())
        {
            cerr << "Error writing CSV: " << status.ToString() << endl;
            return 1;
        }

        cout << "Converted " << parquet_file << " -> " << csv_file << endl;
    }
    catch (const parquet::ParquetException& e)
    {
        cerr << "ParquetException: " << e.what() << endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        cerr << "Exception: " << e.what() << endl;
        return 1;
    }

    return 0;
}

