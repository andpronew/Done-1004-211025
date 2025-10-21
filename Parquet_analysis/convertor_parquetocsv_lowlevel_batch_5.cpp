#include <parquet/file_reader.h>
#include <parquet/column_reader.h>
#include <parquet/exception.h>
#include <parquet/types.h>
#include <parquet/schema.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

// Helper: convert parquet::ByteArray -> std::string (UTF-8 interpretation)
static string BAtoString(const parquet::ByteArray& ba)
{
    return string(reinterpret_cast<const char*>(ba.ptr), ba.len);
}

// Helper: hex representation of fixed length bytes
static string FixedLenToHex(const void* data, int32_t len)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(data);
    ostringstream oss;
    oss << hex << nouppercase << setfill('0');
    for (int i = 0; i < len; ++i) {
        oss << setw(2) << static_cast<int>(p[i]);
    }
    return oss.str();
}

// Helper: INT96 -> hex (legacy timestamp)
static string Int96ToHex(const parquet::Int96& int96)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(&int96);
    ostringstream oss;
    oss << hex << nouppercase << setfill('0');
    for (int i = 0; i < 12; ++i) {
        oss << setw(2) << static_cast<int>(p[i]);
    }
    return oss.str();
}

// CSV escaping: quote fields that contain comma, quote, newline; double internal quotes
static string EscapeCsv(const string& s)
{
    bool need_quote = false;
    for (char c : s) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') { need_quote = true; break; }
    }
    if (!need_quote) return s;
    string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\""; else out += c;
    }
    out += "\"";
    return out;
}

int parquet_to_csv_lowlevel_batch(const string& input_file, const string& output_file, int batch_size = 1048576)
{
    try
    {
        unique_ptr<parquet::ParquetFileReader> reader =
            parquet::ParquetFileReader::OpenFile(input_file, /*memory_map=*/false);

        shared_ptr<parquet::FileMetaData> file_metadata = reader->metadata();
        int num_row_groups = file_metadata->num_row_groups();
        int num_columns = file_metadata->num_columns();

        ofstream csv(output_file, ios::out | ios::trunc);
        if (!csv.is_open()) {
            cerr << "Error: cannot open output CSV file: " << output_file << endl;
            return 1;
        }

        // Write header
        for (int c = 0; c < num_columns; ++c) {
            const parquet::ColumnDescriptor* col_descr = file_metadata->schema()->Column(c);
            csv << EscapeCsv(col_descr->name());
            if (c + 1 < num_columns) csv << ",";
        }
        csv << "\n";

        for (int rg_idx = 0; rg_idx < num_row_groups; ++rg_idx) {
            shared_ptr<parquet::RowGroupReader> rg_reader = reader->RowGroup(rg_idx);
            int64_t rows_in_rg = rg_reader->metadata()->num_rows();

            vector<shared_ptr<parquet::ColumnReader>> col_readers;
            col_readers.reserve(num_columns);
            for (int c = 0; c < num_columns; ++c) {
                col_readers.push_back(rg_reader->Column(c));
            }

            // Prepare buffers for batch reading (except bool!)
            vector<vector<int32_t>> int32_buffers(num_columns);
            vector<vector<int64_t>> int64_buffers(num_columns);
            vector<vector<float>> float_buffers(num_columns);
            vector<vector<double>> double_buffers(num_columns);
            vector<vector<parquet::ByteArray>> ba_buffers(num_columns);
            vector<vector<parquet::FixedLenByteArray>> flba_buffers(num_columns);
            vector<vector<parquet::Int96>> int96_buffers(num_columns);

            vector<vector<int16_t>> def_levels(num_columns);
            vector<vector<int16_t>> rep_levels(num_columns);
            vector<int64_t> values_read(num_columns);

            for (int64_t row_offset = 0; row_offset < rows_in_rg; row_offset += batch_size) {
                int64_t rows_to_read = min(static_cast<int64_t>(batch_size), rows_in_rg - row_offset);

                // Resize buffers
                for (int c = 0; c < num_columns; ++c) {
                    const parquet::ColumnDescriptor* descr = file_metadata->schema()->Column(c);
                    parquet::Type::type phys = descr->physical_type();

                    def_levels[c].resize(rows_to_read);
                    rep_levels[c].resize(rows_to_read);
                    values_read[c] = 0;

                    switch (phys) {
                        case parquet::Type::INT32:
                            int32_buffers[c].resize(rows_to_read);
                            break;
                        case parquet::Type::INT64:
                            int64_buffers[c].resize(rows_to_read);
                            break;
                        case parquet::Type::FLOAT:
                            float_buffers[c].resize(rows_to_read);
                            break;
                        case parquet::Type::DOUBLE:
                            double_buffers[c].resize(rows_to_read);
                            break;
                        case parquet::Type::BYTE_ARRAY:
                            ba_buffers[c].resize(rows_to_read);
                            break;
                        case parquet::Type::FIXED_LEN_BYTE_ARRAY:
                            flba_buffers[c].resize(rows_to_read);
                            break;
                        case parquet::Type::INT96:
                            int96_buffers[c].resize(rows_to_read);
                            break;
                        default:
                            // unsupported types: skip buffer resize
                            break;
                    }
                }

                // Read batch per column
                for (int c = 0; c < num_columns; ++c) {
                    const parquet::ColumnDescriptor* descr = file_metadata->schema()->Column(c);
                    parquet::Type::type phys = descr->physical_type();

                    switch (phys) {
                        case parquet::Type::INT32: {
                            parquet::Int32Reader* r = static_cast<parquet::Int32Reader*>(col_readers[c].get());
                            values_read[c] = r->ReadBatch(rows_to_read,
                                def_levels[c].data(), rep_levels[c].data(), int32_buffers[c].data(), &values_read[c]);
                            break;
                        }
                        case parquet::Type::INT64: {
                            parquet::Int64Reader* r = static_cast<parquet::Int64Reader*>(col_readers[c].get());
                            values_read[c] = r->ReadBatch(rows_to_read,
                                def_levels[c].data(), rep_levels[c].data(), int64_buffers[c].data(), &values_read[c]);
                            break;
                        }
                        case parquet::Type::FLOAT: {
                            parquet::FloatReader* r = static_cast<parquet::FloatReader*>(col_readers[c].get());
                            values_read[c] = r->ReadBatch(rows_to_read,
                                def_levels[c].data(), rep_levels[c].data(), float_buffers[c].data(), &values_read[c]);
                            break;
                        }
                        case parquet::Type::DOUBLE: {
                            parquet::DoubleReader* r = static_cast<parquet::DoubleReader*>(col_readers[c].get());
                            values_read[c] = r->ReadBatch(rows_to_read,
                                def_levels[c].data(), rep_levels[c].data(), double_buffers[c].data(), &values_read[c]);
                            break;
                        }
                        case parquet::Type::BYTE_ARRAY: {
                            parquet::ByteArrayReader* r = static_cast<parquet::ByteArrayReader*>(col_readers[c].get());
                            values_read[c] = r->ReadBatch(rows_to_read,
                                def_levels[c].data(), rep_levels[c].data(), ba_buffers[c].data(), &values_read[c]);
                            break;
                        }
                        case parquet::Type::FIXED_LEN_BYTE_ARRAY: {
                            parquet::FixedLenByteArrayReader* r = static_cast<parquet::FixedLenByteArrayReader*>(col_readers[c].get());
                            values_read[c] = r->ReadBatch(rows_to_read,
                                def_levels[c].data(), rep_levels[c].data(), flba_buffers[c].data(), &values_read[c]);
                            break;
                        }
                        case parquet::Type::INT96: {
                            parquet::Int96Reader* r = static_cast<parquet::Int96Reader*>(col_readers[c].get());
                            values_read[c] = r->ReadBatch(rows_to_read,
                                def_levels[c].data(), rep_levels[c].data(), int96_buffers[c].data(), &values_read[c]);
                            break;
                        }
                        default:
                            // unsupported types: do nothing
                            break;
                    }
                }

                // Output rows
                for (int64_t i = 0; i < rows_to_read; ++i) {
                    for (int c = 0; c < num_columns; ++c) {
                        const parquet::ColumnDescriptor* descr = file_metadata->schema()->Column(c);
                        parquet::Type::type phys = descr->physical_type();
                        int16_t max_def_level = descr->max_definition_level();

                        string cell_text;

                        int16_t def_level = def_levels[c][i];
                        if (def_level < max_def_level) {
                            cell_text = ""; // NULL value
                        }
                        else {
                            switch (phys) {
                                case parquet::Type::INT32:
                                    cell_text = to_string(int32_buffers[c][i]);
                                    break;
                                case parquet::Type::INT64:
                                    cell_text = to_string(int64_buffers[c][i]);
                                    break;
                                case parquet::Type::FLOAT: {
                                    ostringstream oss; oss << float_buffers[c][i]; cell_text = oss.str();
                                    break;
                                }
                                case parquet::Type::DOUBLE: {
                                    ostringstream oss; oss << double_buffers[c][i]; cell_text = oss.str();
                                    break;
                                }
                                case parquet::Type::BYTE_ARRAY:
                                    cell_text = BAtoString(ba_buffers[c][i]);
                                    break;
                                case parquet::Type::FIXED_LEN_BYTE_ARRAY:
                                    cell_text = FixedLenToHex(flba_buffers[c][i].ptr, descr->type_length());
                                    break;
                                case parquet::Type::INT96:
                                    cell_text = Int96ToHex(int96_buffers[c][i]);
                                    break;
                                default:
                                    cell_text = "<UNSUPPORTED>";
                                    break;
                            }
                        }

                        csv << EscapeCsv(cell_text);
                        if (c + 1 < num_columns) csv << ",";
                    }
                    csv << "\n";
                }
            } // for rows
        } // for row groups

        csv.close();
    }
    catch (const parquet::ParquetException& e) {
        cerr << "ParquetException: " << e.what() << endl;
        return 1;
    }
    catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
        return 1;
    }

    cout << "Conversion finished: " << output_file << endl;
    return 0;
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " input.parquet output.csv" << endl;
        return 1;
    }

    string input = argv[1];
    string output = argv[2];
    return parquet_to_csv_lowlevel_batch(input, output);
}

