// Compressor and decompressor
#include <zstd.h>
#include <iostream>
#include <cstring>

using namespace std;

int main() {
    char input_text[256];
    cout << "What is to be compressed: ";
    cin.getline(input_text, sizeof(input_text));
    size_t input_size = strlen(input_text);

    // Сжатие
    ZSTD_CCtx* cctx = ZSTD_createCCtx();

    char compressed[256];
    ZSTD_inBuffer input = { input_text, input_size, 0 };
    ZSTD_outBuffer output = { compressed, sizeof(compressed), 0 };

    // Сжимаем весь текст, говорим что это конец (ZSTD_e_end)
    ZSTD_compressStream2(cctx, &output, &input, ZSTD_e_end);

    size_t compressed_size = output.pos;
    ZSTD_freeCCtx(cctx);
    cout << "Compressed (" << compressed_size << " bytes): ";
    for (size_t i = 0; i < compressed_size; ++i)
    cout << hex << ((unsigned char)compressed[i] & 0xFF) << " ";
    cout << dec << "\n";

    // Разжатие
    ZSTD_DCtx* dctx = ZSTD_createDCtx();

    char decompressed[256];
    ZSTD_inBuffer input2 = { compressed, compressed_size, 0 };
    ZSTD_outBuffer output2 = { decompressed, sizeof(decompressed), 0 };

    ZSTD_decompressStream(dctx, &output2, &input2);

    decompressed[output2.pos] = '\0';  // завершаем строку
    ZSTD_freeDCtx(dctx);

    std::cout << "Результат: " << decompressed << "\n";
    return 0;
}
