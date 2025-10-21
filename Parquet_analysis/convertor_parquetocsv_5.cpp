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

    // Убираем arrow::default_memory_pool()
    return arrow::csv::WriteCSV(*table, options, output.get());
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

    // Открываем Parquet файл
    auto maybe_infile = arrow::io::ReadableFile::Open(parquet_file);
    if (!maybe_infile.ok())
    {
        cerr << "Error opening parquet file: " << maybe_infile.status().ToString() << endl;
        return 1;
    }
    auto infile = *maybe_infile;

    // Создаём Parquet reader
    auto maybe_reader = parquet::arrow::OpenFile(infile, arrow::default_memory_pool());
    if (!maybe_reader.ok())
    {
        cerr << "Error opening parquet reader: " << maybe_reader.status().ToString() << endl;
        return 1;
    }
    unique_ptr<parquet::arrow::FileReader> reader = std::move(maybe_reader).ValueOrDie();

    // Читаем всю таблицу
    shared_ptr<arrow::Table> table;
    arrow::Status st = reader->ReadTable(&table);
    if (!st.ok())
    {
        cerr << "Error reading parquet table: " << st.ToString() << endl;
        return 1;
    }

    // Записываем CSV
    st = write_table_to_csv(table, csv_file);
    if (!st.ok())
    {
        cerr << "Error writing CSV: " << st.ToString() << endl;
        return 1;
    }

    cout << "Converted " << parquet_file << " -> " << csv_file << endl;
    return 0;
}

