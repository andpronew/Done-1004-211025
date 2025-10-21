#include <zstd.h>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

constexpr size_t kChunkSize = 65536; // 64 KB

int main() {
    const char* input_filename = "input_cm_big2.txt";
    const char* compressed_filename = "output_cm_big2.zst";
    const char* decompressed_filename = "output_decompressed_cm_big2.txt";

    // ==== СЖАТИЕ ====
    ifstream infile(input_filename, ios::binary);
    if (!infile) {
        cerr << "Не удалось открыть файл " << input_filename << "\n";
        return 1;
    }

    ofstream compressed_out(compressed_filename, ios::binary);
    if (!compressed_out) {
        cerr << "Не удалось создать файл " << compressed_filename << "\n";
        return 1;
    }

    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    vector<char> in_buf(kChunkSize);
    vector<char> out_buf(ZSTD_compressBound(kChunkSize));

    while (true) {
        infile.read(in_buf.data(), kChunkSize);
        size_t read_size = infile.gcount();
        bool last_chunk = infile.eof();

        ZSTD_inBuffer input = { in_buf.data(), read_size, 0 };
        ZSTD_EndDirective directive = last_chunk ? ZSTD_e_end : ZSTD_e_continue;

        while (input.pos < input.size || directive == ZSTD_e_end) {
            ZSTD_outBuffer output = { out_buf.data(), out_buf.size(), 0 };
            size_t ret = ZSTD_compressStream2(cctx, &output, &input, directive);
            if (ZSTD_isError(ret)) {
                cerr << "Ошибка при сжатии: " << ZSTD_getErrorName(ret) << "\n";
                return 1;
            }

            compressed_out.write(out_buf.data(), output.pos);
            if (!compressed_out) {
                cerr << "Ошибка при записи в файл " << compressed_filename << "\n";
                return 1;
            }

            if (ret == 0) break; // завершение потока
        }

        if (last_chunk) break;
    }

    ZSTD_freeCCtx(cctx);
    infile.close();
    compressed_out.close();
    cout << "Сжатие завершено.\n";

    // ==== РАЗЖАТИЕ ====
    ifstream compressed_in(compressed_filename, ios::binary);
    if (!compressed_in) {
        cerr << "Не удалось открыть файл " << compressed_filename << "\n";
        return 1;
    }

    ofstream decompressed_out(decompressed_filename, ios::binary);
    if (!decompressed_out) {
        cerr << "Не удалось создать файл " << decompressed_filename << "\n";
        return 1;
    }

    ZSTD_DCtx* dctx = ZSTD_createDCtx();
    vector<char> comp_buf(kChunkSize);
    vector<char> decomp_buf(kChunkSize);

    while (true) {
        compressed_in.read(comp_buf.data(), kChunkSize);
        size_t read_size = compressed_in.gcount();
        if (read_size == 0) break;

        ZSTD_inBuffer input = { comp_buf.data(), read_size, 0 };

        while (input.pos < input.size) {
            ZSTD_outBuffer output = { decomp_buf.data(), decomp_buf.size(), 0 };
            size_t ret = ZSTD_decompressStream(dctx, &output, &input);
            if (ZSTD_isError(ret)) {
                cerr << "Ошибка при разжатии: " << ZSTD_getErrorName(ret) << "\n";
                return 1;
            }

            decompressed_out.write(decomp_buf.data(), output.pos);
            if (!decompressed_out) {
                cerr << "Ошибка при записи в файл " << decompressed_filename << "\n";
                return 1;
            }
        }
    }

    ZSTD_freeDCtx(dctx);
    compressed_in.close();
    decompressed_out.close();
    cout << "Разжатие завершено. Результат записан в " << decompressed_filename << "\n";

    return 0;
}
                                                                                         
