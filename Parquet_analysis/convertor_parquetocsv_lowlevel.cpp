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

// Helper: INT96 -> hex (legacy timestamp). Proper conversion to datetime is optional.
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

int parquet_to_csv_lowlevel(const string &input_file, const string &output_file)
{
    try
    {
        // Open Parquet file (do not memory-map by default)
        unique_ptr<parquet::ParquetFileReader> reader =
            parquet::ParquetFileReader::OpenFile(input_file, /*memory_map=*/false);

        shared_ptr<parquet::FileMetaData> file_metadata = reader->metadata();
        int num_row_groups = file_metadata->num_row_groups();
        int num_columns = file_metadata->num_columns();

        // Open CSV output
        ofstream csv(output_file, ios::out | ios::trunc);
        if (!csv.is_open()) {
            cerr << "Error: cannot open output CSV file: " << output_file << endl;
            return 1;
        }

        // Write header: column names (use ColumnDescriptor->name())
        for (int c = 0; c < num_columns; ++c) {
            const parquet::ColumnDescriptor* col_descr = file_metadata->schema()->Column(c);
            csv << EscapeCsv(col_descr->name());
            if (c + 1 < num_columns) csv << ",";
        }
        csv << "\n";

        // Iterate row-groups
        for (int rg_idx = 0; rg_idx < num_row_groups; ++rg_idx) {
            shared_ptr<parquet::RowGroupReader> rg_reader = reader->RowGroup(rg_idx);
            int64_t rows_in_rg = rg_reader->metadata()->num_rows();

            // Prepare column readers
    vector<shared_ptr<parquet::ColumnReader>> col_readers;
            col_readers.reserve(num_columns);
            for (int c = 0; c < num_columns; ++c) {
                col_readers.push_back(rg_reader->Column(c));
            }

            // For each row in row-group, read one value per column (simple, robust)
            for (int64_t row = 0; row < rows_in_rg; ++row) {
                for (int c = 0; c < num_columns; ++c) {
                    const parquet::ColumnDescriptor* descr = file_metadata->schema()->Column(c);
                    parquet::Type::type phys = descr->physical_type();
                    int16_t max_def_level = descr->max_definition_level();

                    string cell_text;

                    switch (phys) {
                        case parquet::Type::INT32: {
                            parquet::Int32Reader* r = static_cast<parquet::Int32Reader*>(col_readers[c].get());
                            int32_t value = 0;
                            int16_t def_level = 0;
                            int16_t rep_level = 0;
                            int64_t values_read = 0;
                            r->ReadBatch(1, &def_level, &rep_level, &value, &values_read);
                            if (def_level < max_def_level) {
                                cell_text = "";
                            } else {
                                cell_text = to_string(value);
                            }
                            break;
                        }
                        case parquet::Type::INT64: {
                            parquet::Int64Reader* r = static_cast<parquet::Int64Reader*>(col_readers[c].get());
                            int64_t value = 0;
                            int16_t def_level = 0;
                            int16_t rep_level = 0;
                            int64_t values_read = 0;
                            r->ReadBatch(1, &def_level, &rep_level, &value, &values_read);
                            if (def_level < max_def_level) {
                                cell_text = "";
                            } else {
                                cell_text = to_string(value);
                            }
                            break;
                        }
                        case parquet::Type::FLOAT: {
                            parquet::FloatReader* r = static_cast<parquet::FloatReader*>(col_readers[c].get());
                            float value = 0.0f;
                            int16_t def_level = 0;
                            int16_t rep_level = 0;
                            int64_t values_read = 0;
                            r->ReadBatch(1, &def_level, &rep_level, &value, &values_read);
                            if (def_level < max_def_level) {
                                cell_text = "";
                            } else {
                                ostringstream oss; oss << value;
                                cell_text = oss.str();
                            }
                            break;
                        }
                        case parquet::Type::DOUBLE: {
                            parquet::DoubleReader* r = static_cast<parquet::DoubleReader*>(col_readers[c].get());
                            double value = 0.0;
                            int16_t def_level = 0;
                            int16_t rep_level = 0;
                            int64_t values_read = 0;
                            r->ReadBatch(1, &def_level, &rep_level, &value, &values_read);
                            if (def_level < max_def_level) {
                                cell_text = "";
                            } else {
                                ostringstream oss; oss << value;
                                cell_text = oss.str();
                            }
                            break;
                        }
                        case parquet::Type::BOOLEAN: {
                            parquet::BoolReader* r = static_cast<parquet::BoolReader*>(col_readers[c].get());
                            bool value = false;
                            int16_t def_level = 0;
                            int16_t rep_level = 0;
                            int64_t values_read = 0;
                            r->ReadBatch(1, &def_level, &rep_level, &value, &values_read);
                            if (def_level < max_def_level) {
                                cell_text = "";
                            } else {
                                cell_text = value ? "true" : "false";
                            }
                            break;
                        }
                        case parquet::Type::BYTE_ARRAY: {
                            parquet::ByteArrayReader* r = static_cast<parquet::ByteArrayReader*>(col_readers[c].get());
                            parquet::ByteArray ba;
                            int16_t def_level = 0;
                            int16_t rep_level = 0;
                            int64_t values_read = 0;
                            r->ReadBatch(1, &def_level, &rep_level, &ba, &values_read);
                            if (def_level < max_def_level) {
                                cell_text = "";
                            } else {
                                cell_text = BAtoString(ba);
                            }
                            break;
                        }
                        case parquet::Type::FIXED_LEN_BYTE_ARRAY: {
                            parquet::FixedLenByteArrayReader* r = static_cast<parquet::FixedLenByteArrayReader*>(col_readers[c].get());
                            parquet::FixedLenByteArray fl;
                            int16_t def_level = 0;
                            int16_t rep_level = 0;
                            int64_t values_read = 0;
                            r->ReadBatch(1, &def_level, &rep_level, &fl, &values_read);
                            if (def_level < max_def_level) {
                                cell_text = "";
                            } else {
                                int len = descr->type_length();
                                cell_text = FixedLenToHex(fl.ptr, len);
                            }
                            break;
                        }
                        case parquet::Type::INT96: {
                            parquet::Int96Reader* r = static_cast<parquet::Int96Reader*>(col_readers[c].get());
                            parquet::Int96 i96;
                            int16_t def_level = 0;
                            int16_t rep_level = 0;
                            int64_t values_read = 0;
                            r->ReadBatch(1, &def_level, &rep_level, &i96, &values_read);
                            if (def_level < max_def_level) {
                                cell_text = "";
                            } else {
                                cell_text = Int96ToHex(i96);
                            }
                            break;
                        }
                        default: {
                            // Unknown / unsupported physical type
                            cell_text = "<UNSUPPORTED>";
                            break;
                        }
                    } // switch

                    csv << EscapeCsv(cell_text);
                    if (c + 1 < num_columns) csv << ",";
                } // columns
                csv << "\n";
            } // rows in rg
        } // row-groups

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
    return parquet_to_csv_lowlevel(input, output);
}

