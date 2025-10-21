#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/result.h>
#include <parquet/arrow/reader.h>
#include <arrow/csv/api.h>
#include <iostream>
#include <fstream>

using namespace std;
using arrow::Status;
using arrow::Table;

// Универсальная функция для записи любой таблицы в CSV
Status write_table_to_csv(const shared_ptr<Table>& table, const string& output_file)
{
    // Создаём файловый поток Arrow
    ARROW_ASSIGN_OR_RAISE(auto output,
        arrow::io::FileOutputStream::Open(output_file));

    // Настройки CSV
    auto write_options = arrow::csv::WriteOptions::Defaults();
    write_options.include_header = true; // заголовок с именами колонок

    // Запись в CSV
    ARROW_RETURN_NOT_OK(arrow::csv::WriteCSV(*table, write_options, arrow::default_memory_pool(), output.get()));
    return Status::OK();
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

    // Открываем Parquet
    ARROW_ASSIGN_OR_RAISE(auto infile,
        arrow::io::ReadableFile::Open(parquet_file));

    // Создаём Parquet reader
    std::unique_ptr<parquet::arrow::FileReader> reader;
    ARROW_RETURN_NOT_OK(parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader));

    // Читаем всю таблицу
    shared_ptr<Table> table;
    ARROW_RETURN_NOT_OK(reader->ReadTable(&table));

    // Пишем в CSV
    Status st = write_table_to_csv(table, csv_file);
    if (!st.ok())
    {
        cerr << "Error writing CSV: " << st.ToString() << endl;
        return 1;
    }

    cout << "Converted " << parquet_file << " -> " << csv_file << endl;
    return 0;
}

