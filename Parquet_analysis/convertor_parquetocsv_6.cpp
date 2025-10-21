#include <parquet/column_reader.h>
#include <parquet/exception.h>
#include <parquet/types.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace parquet;

static string ByteArrayToString(const ByteArray& ba)
{
    return string(reinterpret_cast<const char*>(ba.ptr), ba.len);
}

static string FixedLenToHex(const void* data, int32_t len)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(data);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < len; ++i) {
        oss << setw(2) << static_cast<int>(p[i]);
    }
    return oss.str();
}

static string Int96ToHex(const Int96& int96)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(&int96);
    ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 12; ++i) {
        oss << setw(2) << static_cast<int>(p[i]);
    }
    return oss.str();
}

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
        unique_ptr<ParquetFileReader> reader = ParquetFileReader::OpenFile(input_file, false);

        shared_ptr<FileMetaData> file_metadata = reader->metadata();
        int num_row_groups = file_metadata->num_row_groups();
        int num_columns = file_metadata->num_columns();

        ofstream csv(output_file, ios::out | ios::trunc);
        if (!csv.is_open()) {
            cerr << "Error: cannot open output CSV file: " << output_file << endl;
            return 1;
        }

        for (int c = 0; c < num_columns; ++c) {
            const SchemaElement* col_schema = file_metadata->schema()->Column(c);
            csv << EscapeCsv(col_schema->name);
            if (c + 1 < num_columns) csv << ",";
        }
        csv << "\n";

        for (int rg_idx = 0; rg_idx < num_row_groups; ++rg_idx) {
            unique_ptr<RowGroupReader> rg_reader = reader->RowGroup(rg_idx);
            int64_t rows_in_rg = rg_reader->metadata()->num_rows();

            vector<shared_ptr<ColumnReader>> col_readers;
            col_readers.reserve(num_columns);
            for (int c = 0; c < num_columns; ++c) {
                col_readers.push_back(rg_reader->Column(c));
            }

            for (int64_t row = 0; row < rows_in_rg; ++row) {
                for (int c = 0; c < num_columns; ++c) {
                    const ColumnDescriptor* descr = file_metadata->schema()->Column(c);
                    Type::type phys = descr->physical_type();
                    int16_t max_def_level = descr->max_definition_level();

                    string cell_text;

                    switch (phys) {
                        case Type::INT32: {
                            Int32Reader* r = static_cast<Int32Reader*>(col_readers[c].get());
                            int32_t value = 0;
                            int16_t def_level = 0;
                            int16_t rep_level = 0;
                            int64_t values_read = 0;
                            r->ReadBatch(1, &def_level, &rep_level, &value, &values_read);
                            if (def_level < max_def_level) {
                                cell_text = ""; // NULL
                            } else {
                                cell_text = to_string(value);
                            }
                            break;
                        }

                        case Type::INT64: {
                            Int64Reader* r = static_cast<Int64Reader*>(col_readers[c].get());
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

                        case Type::FLOAT: {
                            FloatReader* r = static_cast<FloatReader*>(col_readers[c].get());
                            float value = 0.0f;
                            int16_t def_level = 0;
                            int16_t rep_level = 0;
                            int64_t values_read = 0;
                            r->ReadBatch(1, &def_level, &rep_level, &value, &values_read);
                            if (def_level < max_def_level) {
                                cell_text = "";
                            } else {
                                // Use ostringstream to avoid scientific notation surprises
                                ostringstream oss; oss << value;
                                cell_text = oss.str();
                            }
                            break;
                        }

                        case Type::DOUBLE: {
                            DoubleReader* r = static_cast<DoubleReader*>(col_readers[c].get());
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

                        case Type::BOOLEAN: {
                            BoolReader* r = static_cast<BoolReader*>(col_readers[c].get());
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

                        case Type::BYTE_ARRAY: {
                            ByteArrayReader* r = static_cast<ByteArrayReader*>(col_readers[c].get());
                            ByteArray ba;
                            int16_t def_level = 0;
                            int16_t rep_level = 0;
                            int64_t values_read = 0;
                            r->ReadBatch(1, &def_level, &rep_level, &ba, &values_read);
                            if (def_level < max_def_level) {
                                cell_text = "";
                            } else {
                                // Interpret as UTF-8 string; if not UTF-8, user sees bytes-as-string
                                cell_text = ByteArrayToString(ba);
                            }
                            break;
                        }

                        case Type::FIXED_LEN_BYTE_ARRAY: {
                            FixedLenByteArrayReader* r = static_cast<FixedLenByteArrayReader*>(col_readers[c].get());
                            FixedLenByteArray fl;
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

                        case Type::INT96: {
                            Int96Reader* r = static_cast<Int96Reader*>(col_readers[c].get());
                            Int96 val;
                            int16_t def_level = 0;
                            int16_t rep_level = 0;
                            int64_t values_read = 0;
                            r->ReadBatch(1, &def_level, &rep_level, &val, &values_read);
                            if (def_level < max_def_level) {
                                cell_text = "";
                            } else {
                                cell_text = Int96ToHex(val);
                            }
                            break;
                        }

                        default: {
                            cell_text = "<UNSUPPORTED>";
                            break;
                        }
                    } 


                    csv << EscapeCsv(cell_text);
                    if (c + 1 < num_columns) csv << ",";
                } 

                csv << "\n";
            } 
        } 

        csv.close();
    }
    catch (const ParquetException& e) {
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

