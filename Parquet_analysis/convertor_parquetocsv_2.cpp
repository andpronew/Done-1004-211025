#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

using namespace std;

void write_csv(const string& filename, shared_ptr<arrow::Table> table)
{
    ofstream out(filename);
    if (!out.is_open())
    {
        throw runtime_error("Cannot open output CSV file");
    }

    // Write header
    for (int i = 0; i < table->num_columns(); i++)
    {
        out << table->schema()->field(i)->name();
        if (i < table->num_columns() - 1)
            out << ",";
    }
    out << "\n";

    int64_t num_rows = table->num_rows();
    int num_cols = table->num_columns();

    for (int64_t row = 0; row < num_rows; row++)
    {
        for (int col = 0; col < num_cols; col++)
        {
            shared_ptr<arrow::ChunkedArray> column = table->column(col);
            int64_t chunk_index = 0;
            int64_t row_in_chunk = row;
            while (chunk_index < column->num_chunks() &&
                   row_in_chunk >= column->chunk(chunk_index)->length())
            {
                row_in_chunk -= column->chunk(chunk_index)->length();
                chunk_index++;
            }

            auto arr = column->chunk(chunk_index);
            string cell;

            switch (arr->type()->id())
            {
                case arrow::Type::INT64:
                    cell = to_string(static_pointer_cast<arrow::Int64Array>(arr)->Value(row_in_chunk));
                    break;
                case arrow::Type::DOUBLE:
                    cell = to_string(static_pointer_cast<arrow::DoubleArray>(arr)->Value(row_in_chunk));
                    break;
                case arrow::Type::FLOAT:
                    cell = to_string(static_pointer_cast<arrow::FloatArray>(arr)->Value(row_in_chunk));
                    break;
                case arrow::Type::STRING:
                    cell = static_pointer_cast<arrow::StringArray>(arr)->GetString(row_in_chunk);
                    break;
                case arrow::Type::BINARY:
                    cell = "<BINARY>";
                    break;
                case arrow::Type::BOOL:
                    cell = static_pointer_cast<arrow::BooleanArray>(arr)->Value(row_in_chunk) ? "true" : "false";
                    break;
                default:
                    cell = "<UNSUPPORTED>";
            }

            out << cell;
            if (col < num_cols - 1)
                out << ",";
        }
        out << "\n";
    }
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " <input.parquet> <output.csv>" << endl;
        return 1;
    }

    string input_file = argv[1];
    string output_file = argv[2];

    try
    {
        shared_ptr<arrow::io::ReadableFile> infile;
        PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open(input_file));

        auto maybe_reader = parquet::arrow::OpenFile(infile, arrow::default_memory_pool());
        PARQUET_ASSIGN_OR_THROW(auto reader, maybe_reader);


        shared_ptr<arrow::Table> table;
        PARQUET_THROW_NOT_OK(reader->ReadTable(&table));

        write_csv(output_file, table);
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

