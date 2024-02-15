#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp" // Make sure this points to the correct location of json.hpp in your project

// Use namespace for convenience
using json = nlohmann::json;

// Callback function writes received data to a std::string
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s)
{
    size_t newLength = size * nmemb;
    try
    {
        s->append((char *)contents, newLength);
        return newLength;
    }
    catch (std::bad_alloc &e)
    {
        // Handle memory problem
        return 0;
    }
}

int main()
{
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:5001/api/data");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        else
        {
            // Parse JSON and save it in a json object
            json jsonResponse;
            try
            {
                jsonResponse = json::parse(readBuffer);
                // Now you can do something with jsonResponse
                // Example: Accessing an element (adjust according to your actual JSON structure)
                // std::cout << "Example element: " << jsonResponse["key"] << std::endl;
            }
            catch (json::parse_error &e)
            {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
        }
    }

    return 0;
}
