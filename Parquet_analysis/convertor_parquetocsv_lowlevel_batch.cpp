#include <parquet/api/reader.h>
#include <parquet/api/schema.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>

using namespace std;

// Универсальная функция для преобразования значения в строку
string value_to_string(parquet::Type::type type, const uint8_t* ptr, int length)
{
    switch (type)
    {
        case parquet::Type::BOOLEAN:
            return (*(reinterpret_cast<const bool*>(ptr))) ? "true" : "false";
        case parquet::Type::INT32:
            return to_string(*reinterpret_cast<const int32_t*>(ptr));
        case parquet::Type::INT64:
            return to_string(*reinterpret_cast<const int64_t*>(ptr));
        case parquet::Type::FLOAT:
            return to_string(*reinterpret_cast<const float*>(ptr));
        case parquet::Type::DOUBLE:
            return to_string(*reinterpret_cast<const double*>(ptr));
        case parquet::Type::BYTE_ARRAY:
        case parquet::Type::FIXED_LEN_BYTE_ARRAY:
            return string(reinterpret_cast<const char*>(ptr), length);
        default:
            return "";
    }
}

int parquet_to_csv_lowlevel_batch(const string& parquet_file, const string& csv_file)
{
    try
    {
        unique_ptr<parquet::ParquetFileReader> reader = parquet::ParquetFileReader::OpenFile(parquet_file, false);
        shared_ptr<parquet::FileMetaData> file_metadata = reader->metadata();

        int num_row_groups = file_metadata->num_row_groups();
        int num_columns = file_metadata->num_columns();

        ofstream csv(csv_file);
        if (!csv.is_open())
        {
            cerr << "Error: cannot open CSV file for writing\n";
            return 1;
        }

        // Читаем каждый RowGroup
        for (int rg_idx = 0; rg_idx < num_row_groups; rg_idx++)
        {
            shared_ptr<parquet::RowGroupReader> rg_reader = reader->RowGroup(rg_idx);

            vector<shared_ptr<parquet::ColumnReader>> col_readers;
            col_readers.reserve(num_columns);

            for (int col_idx = 0; col_idx < num_columns; col_idx++)
            {
                col_readers.push_back(rg_reader->Column(col_idx));
            }

            int64_t rows_in_group = file_metadata->RowGroup(rg_idx)->num_rows();

            // Буфер чтения — batch по 1024 строк
            const int64_t batch_size = 1024;

            for (int64_t row_start = 0; row_start < rows_in_group; row_start += batch_size)
            {
                int64_t rows_to_read = min(batch_size, rows_in_group - row_start);

                // Для каждой строки из batch пишем CSV
                for (int64_t row_offset = 0; row_offset < rows_to_read; row_offset++)
                {
                    for (int col_idx = 0; col_idx < num_columns; col_idx++)
                    {
                        parquet::ColumnReader* col_reader = col_readers[col_idx].get();
                        parquet::Type::type col_type = file_metadata->schema()->Column(col_idx)->physical_type();

                        switch (col_type)
                        {
                            case parquet::Type::BOOLEAN:
                            {
                                auto* bool_reader = static_cast<parquet::BoolReader*>(col_reader);
                                bool value;
                                int64_t values_read = 0;
                                bool_reader->ReadBatch(1, nullptr, nullptr, &value, &values_read);
                                csv << (value ? "true" : "false");
                                break;
                            }
                            case parquet::Type::INT32:
                            {
                                auto* int32_reader = static_cast<parquet::Int32Reader*>(col_reader);
                                int32_t value;
                                int64_t values_read = 0;
                                int32_reader->ReadBatch(1, nullptr, nullptr, &value, &values_read);
                                csv << value;
                                break;
                            }
                            case parquet::Type::INT64:
                            {
                                auto* int64_reader = static_cast<parquet::Int64Reader*>(col_reader);
                                int64_t value;
                                int64_reader->ReadBatch(1, nullptr, nullptr, &value, nullptr);
                                csv << value;
                                break;
                            }
                            case parquet::Type::FLOAT:
                            {
                                auto* float_reader = static_cast<parquet::FloatReader*>(col_reader);
                                float value;
                                float_reader->ReadBatch(1, nullptr, nullptr, &value, nullptr);
                                csv << value;
                                break;
                            }
                            case parquet::Type::DOUBLE:
                            {
                                auto* double_reader = static_cast<parquet::DoubleReader*>(col_reader);
                                double value;
                                double_reader->ReadBatch(1, nullptr, nullptr, &value, nullptr);
                                csv << value;
                                break;
                            }
                            case parquet::Type::BYTE_ARRAY:
                            {
                                auto* ba_reader = static_cast<parquet::ByteArrayReader*>(col_reader);
                                parquet::ByteArray value;
                                ba_reader->ReadBatch(1, nullptr, nullptr, &value, nullptr);
                                csv << string(reinterpret_cast<const char*>(value.ptr), value.len);
                                break;
                            }
                            case parquet::Type::FIXED_LEN_BYTE_ARRAY:
                            {
                                auto* flba_reader = static_cast<parquet::FixedLenByteArrayReader*>(col_reader);
                                parquet::FixedLenByteArray value;
                                flba_reader->ReadBatch(1, nullptr, nullptr, &value, nullptr);
                                int length = file_metadata->schema()->Column(col_idx)->type_length();
                                csv << string(reinterpret_cast<const char*>(value.ptr), length);
                                break;
                            }
                            default:
                                csv << "";
                                break;
                        }

                        if (col_idx < num_columns - 1)
                            csv << ",";
                    }
                    csv << "\n";
                }
            }
        }

        csv.close();
        return 0;
    }
    catch (const exception& ex)
    {
        cerr << "Exception: " << ex.what() << "\n";
        return 1;
    }
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " input.parquet output.csv\n";
        return 1;
    }

    return parquet_to_csv_lowlevel_batch(argv[1], argv[2]);
}

