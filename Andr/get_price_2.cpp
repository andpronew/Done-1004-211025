#include <iostream>
#include <string>
#include <curl/curl.h>

using namespace std;

// Функция обратного вызова для записи данных в std::string
size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output)
{
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

int main()
{
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        cerr << "Failed to initialize curl" << endl;
        return 1;
    }

    string symbol = "TONUSDT"; // нужная пара
    string url = "https://api.binance.com/api/v3/ticker/price?symbol=" + symbol;

    string readBuffer;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(curl);
        return 1;
    }

    curl_easy_cleanup(curl);

    cout << "Response: " << readBuffer << endl;

    return 0;
}

