#include <zstd.h>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

constexpr size_t kChunkSize = 65536; // 64 kB

int main() {
    const char* input_filename = "input_cm_big.txt";
    const char* compressed_filename = "output_cm_big.zst";
    const char* decompressed_filename = "output_decompressed_cm_big.txt";

    // ==== Compression ====
    ifstream infile(input_filename, ios::binary);
    if (!infile) {
        cerr << "File can not be opened " << input_filename << "\n";
        return 1;
    }

    ofstream compressed_out(compressed_filename, ios::binary);
    if (!compressed_out) {
        cerr << "File can not be created " << compressed_filename << "\n";
        return 1;
    }

    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    vector<char> in_buf(kChunkSize);
    vector<char> out_buf(ZSTD_compressBound(kChunkSize));

    while (infile) {
        infile.read(in_buf.data(), kChunkSize);
        size_t read_size = infile.gcount();

        ZSTD_inBuffer input = { in_buf.data(), read_size, 0 };

        while (input.pos < input.size) {
            ZSTD_outBuffer output = { out_buf.data(), out_buf.size(), 0 };
            size_t ret = ZSTD_compressStream(cctx, &output, &input);
            if (ZSTD_isError(ret)) {
                cerr << "Mistake when compressing: " << ZSTD_getErrorName(ret) << "\n";
                return 1;
            }
            compressed_out.write(out_buf.data(), output.pos);
        }
    }

    // Stream finishing
    ZSTD_outBuffer output = { out_buf.data(), out_buf.size(), 0 };
    size_t ret = ZSTD_endStream(cctx, &output);
    if (ZSTD_isError(ret)) {
        cerr << "Mistake when compression is finishing: " << ZSTD_getErrorName(ret) << "\n";
        return 1;
    }
    compressed_out.write(out_buf.data(), output.pos);

    ZSTD_freeCCtx(cctx);
    infile.close();
    compressed_out.close();
    cout << "Compression has been finished.\n";

    // ==== Decompression ====
    ifstream compressed_in(compressed_filename, ios::binary);
    if (!compressed_in) {
        cerr << "File can not be opened " << compressed_filename << "\n";
        return 1;
    }

    ofstream decompressed_out(decompressed_filename, ios::binary);
    if (!decompressed_out) {
        cerr << "File can not be created " << decompressed_filename << "\n";
        return 1;
    }

    ZSTD_DCtx* dctx = ZSTD_createDCtx();
    vector<char> comp_buf(kChunkSize);
    vector<char> decomp_buf(kChunkSize);

    while (compressed_in) {
        compressed_in.read(comp_buf.data(), kChunkSize);
        size_t read_size = compressed_in.gcount();
        ZSTD_inBuffer input = { comp_buf.data(), read_size, 0 };

        while (input.pos < input.size) {
            ZSTD_outBuffer output = { decomp_buf.data(), decomp_buf.size(), 0 };
            size_t ret = ZSTD_decompressStream(dctx, &output, &input);
            if (ZSTD_isError(ret)) {
                cerr << "Mistake when compressed: " << ZSTD_getErrorName(ret) << "\n";
                return 1;
            }
            decompressed_out.write(decomp_buf.data(), output.pos);
        }
    }

    ZSTD_freeDCtx(dctx);
    compressed_in.close();
    decompressed_out.close();
    cout << "Decompression has been finished. Result is in " << decompressed_filename << "\n";

    return 0;
}
