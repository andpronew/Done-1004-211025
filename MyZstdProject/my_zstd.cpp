#include <zstd.h>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

int main() {
    const char* input_filename = "input.txt";
    const char* compressed_filename = "output.zst";
    const char* decompressed_filename = "output_decompressed.txt";

    // Чтение исходного файла
    ifstream infile(input_filename, ios::binary);
    if (!infile) {
        cerr << "Не удалось открыть файл " << input_filename << "\n";
        return 1;
    }
    vector<char> input_data((istreambuf_iterator<char>(infile)), istreambuf_iterator<char>());
    infile.close();

    // Сжатие
    ZSTD_CCtx* cctx = ZSTD_createCCtx();
    size_t max_compressed_size = ZSTD_compressBound(input_data.size());
    vector<char> compressed_data(max_compressed_size);

    ZSTD_inBuffer input = { input_data.data(), input_data.size(), 0 };
    ZSTD_outBuffer output = { compressed_data.data(), compressed_data.size(), 0 };

    size_t ret = ZSTD_compressStream2(cctx, &output, &input, ZSTD_e_end);
    if (ZSTD_isError(ret)) {
        cerr << "Ошибка при сжатии: " << ZSTD_getErrorName(ret) << "\n";
        return 1;
    }
    ZSTD_freeCCtx(cctx);

    // Сохраняем сжатый файл
    ofstream outfile(compressed_filename, ios::binary);
    outfile.write(compressed_data.data(), output.pos);
    outfile.close();

    cout << "Сжатие завершено. Размер: " << output.pos << " байт.\n";

    // Разжатие
    ifstream compressed_in(compressed_filename, ios::binary);
    vector<char> compressed_input((istreambuf_iterator<char>(compressed_in)), istreambuf_iterator<char>());
    compressed_in.close();

    ZSTD_DCtx* dctx = ZSTD_createDCtx();
    vector<char> decompressed_data(1024 * 1024);  // 1MB буфер (можно больше)

    ZSTD_inBuffer comp_in = { compressed_input.data(), compressed_input.size(), 0 };
    ZSTD_outBuffer decomp_out = { decompressed_data.data(), decompressed_data.size(), 0 };

    ret = ZSTD_decompressStream(dctx, &decomp_out, &comp_in);
    if (ZSTD_isError(ret)) {
        cerr << "Ошибка при разжатии: " << ZSTD_getErrorName(ret) << "\n";
        return 1;
    }
    ZSTD_freeDCtx(dctx);

    // Сохраняем разжатый файл
    ofstream decomp_outfile(decompressed_filename, ios::binary);
    decomp_outfile.write(decompressed_data.data(), decomp_out.pos);
    decomp_outfile.close();

    cout << "Разжатие завершено. Результат записан в " << decompressed_filename << "\n";

    return 0;
}
