#include <iostream>
#include <fstream>
#include <parquet/stream_reader.h>
#include <parquet/exception.h>
#include <parquet/file_reader.h>
#include <arrow/io/file.h>

using namespace std;

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " input.parquet output.csv" << endl;
        return 1;
    }

    string parquet_path = argv[1];
    string csv_path = argv[2];

    try
    {
        shared_ptr<arrow::io::ReadableFile> infile =
            arrow::io::ReadableFile::Open(parquet_path).ValueOrDie();

        auto parquet_reader = parquet::ParquetFileReader::Open(infile);

        auto file_metadata = parquet_reader->metadata();
        int num_columns = file_metadata->num_columns();
        int num_rows = file_metadata->num_rows();

        ofstream csv_out(csv_path);

        for (int i = 0; i < num_columns; i++)
        {
            csv_out << file_metadata->schema()->Column(i)->name();
            if (i != num_columns - 1)
                csv_out << ",";
        }
        csv_out << "\n";

        parquet::StreamReader stream_reader{ std::move(parquet_reader) };
        string value;

        for (int64_t row = 0; row < num_rows; row++)
        {
            for (int col = 0; col < num_columns; col++)
            {
                stream_reader >> value;
                csv_out << value;
                if (col != num_columns - 1)
                    csv_out << ",";
            }
            stream_reader >> parquet::EndRow;
            csv_out << "\n";
        }

        csv_out.close();
        cout << "Conversion completed: " << csv_path << endl;
    
    }
        catch (const std::exception& e)
    {
    cerr << "Error: " << e.what() << endl;
    return 1;
    }

    return 0;
}

