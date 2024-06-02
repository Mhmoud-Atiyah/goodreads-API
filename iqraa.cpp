#include <iostream>
#include <fstream>
#include <curl/curl.h>
#include <cstdlib>
#include <string>
#include <cstdlib>
#include <ctime>
#include <chrono> //TODO: Just for time
#include "htmlParser.hpp"

#define RETURN_SUCCESS 0
#define RETURN_FAILURE 1

#ifdef WIN_32
#include <Windows.h>
const char *homeDir = std::getenv("USERPROFILE");
// Implementation of unix remove() in windows
int remove(const char *__filename)
{
    if (DeleteFile(__filename))
    {
        printf("File deleted successfully\n");
    }
    else
    {
        // Handle error if the file could not be deleted
        DWORD errorCode = GetLastError();
        printf("Error deleting file. Error code: %d\n", errorCode);
    }
}
#else
const char *homeDir = std::getenv("HOME");
#endif
std::string iqraaDir = (std::string)homeDir + "/.iqraa";
std::string cacheDir = iqraaDir + "/cache";
std::string temp_Dir = cacheDir + "/temp";
std::string ntoesDir = iqraaDir + "/notes";

#ifndef OUT_DIR
#define OUT_DIR iqraaDir.c_str()
#endif
#ifndef IQRAA_VERSION
#define IQRAA_VERSION "1.0"
#endif

class Iqraa
{
private:
    /* 0. Default Vars */
    const std::string searchLink = "https://www.goodreads.com/search?utf8=%E2%9C%93&q=";
    size_t iterNr = 5; // Default search results number
    /* 1. News's Data */
    const std::string sites_news[5][2] = {
        {"Youm7", "https://www.youm7.com/Section/%D8%AB%D9%82%D8%A7%D9%81%D8%A9/94/1"},
        {"skyarabia", "https://www.skynewsarabia.com/topic/774407-%D8%AB%D9%82%D8%A7%D9%81%D8%A9"},
        {"Jazeera", "link"},
        {"asharq.com", "link"},
        {"aawsat.com", "https://aawsat.com/%D8%AB%D9%82%D8%A7%D9%81%D8%A9-%D9%88%D9%81%D9%86%D9%88%D9%86/%D9%83%D8%AA%D8%A8"}};
    /* 2. Store's Data */
    const std::string sites_store[4][2] = {
        {"shorouk", "https://www.shoroukbookstores.com/books/arabic/bestseller?order=latest"},
        {"aseeralkotb", "https://www.aseeralkotb.com/ar/bestseller-this-month"},
        {"jarir", "https://www.jarir.com/jarir-publications-books/jarir-publications-books.html?producttag=الأفضل%20مبيعا"},
        {"asfar", "https://asfar.io/"}};


    /* Extracts the first number from a given string. */
    /**
     *
     * @brief Extracts the first number from a given string.
     *
     * This function takes a string as input and extracts the first number it encounters.
     * It iterates over each character in the string and appends any digit characters to
     * a separate string. Once a non-digit character is encountered and the number string
     * is not empty, the function stops and returns the extracted number.
     *
     * @param input The input string from which to extract the number, passed as a constant reference to a std::string.
     * @return The extracted number as a std::string.
     */
    static std::string extractNumber(const std::string &input)
    {
        std::string number;
        for (char c : input)
        {
            if (std::isdigit(c))
            {
                number += c;
            }
            else if (!number.empty())
            {
                break;
            }
        }
        return number;
    }
    /* Set Global Iterator Number */
    void setIterNr(size_t no)
    {
        this->iterNr = no;
    };
    /* Extracts the first number from a given string. */
    /**
     * @note ||| AI Generated |||
     *
     * @brief Removes query parameters and the "rank" parameter from a URL.
     *
     * This function takes a URL as input and removes any query parameters (starting from '?')
     * and the "rank" parameter (starting with "rank="). The cleaned URL is then returned.
     * @param url The input URL as a constant reference to a std::string.
     * @return The cleaned URL as a std::string.
     */
    static std::string removeQueryParamsAndRank(const std::string &url)
    {
        std::string result = url;

        // Find the position of '?' and remove everything after it
        size_t queryPos = result.find('?');
        if (queryPos != std::string::npos)
        {
            result = result.substr(0, queryPos);
        }

        // Find the position of "rank=" and remove it along with the number
        size_t rankPos = result.find("rank=");
        if (rankPos != std::string::npos)
        {
            size_t rankEndPos = result.find('&', rankPos);
            if (rankEndPos != std::string::npos)
            {
                result.erase(rankPos, rankEndPos - rankPos + 1);
            }
        }

        return result;
    }
    /* Replaces all occurrences of a substring in a string with another substring */
    /**
     * @brief Replaces all occurrences of a substring in a string with another substring.
     *
     * @param str The string to modify.
     * @param search The substring to search for.
     * @param replace The substring to replace occurrences of `search` with.
     */
    static std::string clearStr(const std::string &str, const std::string &search, const std::string &replace)
    {
        std::string modifiedStr = str;
        std::string searchStr = search;
        const std::string &replaceStr = replace;

        for (char escape : searchStr)
        {
            size_t pos = 0;
            while ((pos = modifiedStr.find(escape, pos)) != std::string::npos)
            {
                modifiedStr.replace(pos, 1, replaceStr);
                pos += replaceStr.length();
            }
        }

        return modifiedStr;
    }
    /* Replaces two different substrings in a string with their corresponding replacements */
    /**
     * @brief Replaces two different substrings in a string with their corresponding replacements.
     *
     * @param str The string to modify.
     * @param search1 The first substring to search for.
     * @param replace1 The replacement for the first substring.
     * @param search2 The second substring to search for.
     * @param replace2 The replacement for the second substring.
     * @return The modified string after replacing the substrings.
     */
    static std::string clearStr(const std::string &str, const std::string &search1, const std::string &replace1,
                                const std::string &search2, const std::string &replace2)
    {
        std::string modifiedStr = str;

        size_t pos = 0;
        while ((pos = modifiedStr.find(search1, pos)) != std::string::npos)
        {
            modifiedStr.replace(pos, search1.length(), replace1);
            pos += replace1.length();
        }

        pos = 0;
        while ((pos = modifiedStr.find(search2, pos)) != std::string::npos)
        {
            modifiedStr.replace(pos, search2.length(), replace2);
            pos += replace2.length();
        }

        return modifiedStr;
    }
    /* Write data */
    static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
    {
        size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
        return written;
    }
    /**
     * @brief Download the web page from internet and save it local
     *
     * @param url the https path to page
     * @param fileName the output name of page
     */
    static void retrievePage(const std::string &url, const std::string &fileName)
    {
        CURL *curl;
        CURLcode res;
        FILE *pagefile;

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();

        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

            #ifdef SKIP_PEER_VERIFICATION
                        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            #endif
            #ifdef SKIP_HOSTNAME_VERIFICATION
                        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            #endif

            /* cache the CA cert bundle in memory for a week */
            curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 604800L);
            /* send all data to this function  */
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

            /* Perform the request, res will get the return code */
            res = curl_easy_perform(curl);
            /* Check for errors */
            if (res != CURLE_OK)
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
            /* open the file */
            pagefile = fopen(fileName.c_str(), "wb");
            if (pagefile)
            {
                /* write the page body to this file handle */
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);

                /* get it! */
                curl_easy_perform(curl);

                /* close the header file */
                fclose(pagefile);
            }

            /* always cleanup */
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
    }
public:
    /* Downlaod Book Data Routine */
    int open(const std::string &seed)
    {
        /* 0. Generate ID of Book */
        #ifdef WIN32 // Windows
        std::random_device rd;
        std::mt19937 gen(rd());

        // Define the range of the random number
        int lowerBound = 1;
        int upperBound = 100;

        // Create a uniform distribution for the range
        std::uniform_int_distribution<int> dis(lowerBound, upperBound);

        // Generate a random number within the range
        int randomNumber = dis(gen);

        #else // Linux
        
        // Seed the random number generator with the current time
        std::srand(std::time(nullptr));
        int randomNumber = std::rand();
        #endif
        /* 1. Cache the web page local temporary */
        const std::string filePathHTML = "open.html";//TODO: Change path for multi threads
        Iqraa::retrievePage(seed, filePathHTML);
        std::ifstream ifs(filePathHTML);
        if (!ifs.is_open())
        {
            std::cout << "Error Open html file: " << filePathHTML << "\n";
            return 0;
        }
        /* 2. Open HTML Parser*/
        std::string page((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        html::parser p;
        html::node_ptr n = p.parse(page);
        html::node_ptr bookData = n->select("main")->select("div[class='BookPage__gridContainer']");
        html::node_ptr tags = bookData->select("span[class='BookPageMetadataSection__genreButton']");
        /* 3. Open JSON Handler */
        const std::string filePathJSON = std::to_string(randomNumber) + ".json";//TODO: Change Path
        std::fstream outJson;
        outJson.open(filePathJSON, std::ios_base::out);
        // outJson << n->select("main")->to_html();

        if (!outJson.is_open())
        {
            std::cout << "Unable to open the file.\n";
            return 0;
        }
        outJson << "{";
        outJson << "\"id\": "
                << "\"" << randomNumber << "\"," << std::endl; // Book ID For Data Manipulation
        outJson << "\"coverSrc\": "
                << "\"" << bookData->select("div[class='BookCover__image']:first")->select("div")->select("img")->at(0)->get_attr("src") << "\"," << std::endl; // Book Cover Path
        outJson << "\"title\": "
                << "\"" << Iqraa::clearStr((bookData->select("h1[class='Text Text__title1']")->to_text()), "\"", "'") << "\"," << std::endl;
        outJson << "\"pagesCount\": "
                << "\"" << Iqraa::extractNumber(bookData->select("p[data-testid='pagesFormat']")->to_text()) << "\"," << std::endl;
        outJson << "\"pubDate\": "
                << "\"" << bookData->select("p[data-testid='publicationInfo']")->to_text() << "\"," << std::endl;
        outJson << "\"rating\": "
                << "\"" << bookData->select("div[class='RatingStatistics__rating']")->at(0)->to_text() << "\"," << std::endl; //
        outJson << "\"author\": {" //TODO: Complete it with My DB ID
                << "\"name\":\"" << bookData->select("a[class='ContributorLink']")->at(1)->select("span")->to_text() << "\","
                << "\"birth\":\""
                << "TODO"
                << "\","
                << "\"info\":\"" << Iqraa::clearStr((bookData->select("div[class='DetailsLayoutRightParagraph__widthConstrained']")->at(1)->select("span")->to_text()), "\"", "'", "\n", "") << "\","
                << "\"path\":\"" << bookData->select("a[class='ContributorLink']")->at(1)->get_attr("href") << "\"" << std::endl;
        outJson << "},";
        outJson << "\"about\": "
                << "\"" << Iqraa::clearStr((bookData->select("div[class='DetailsLayoutRightParagraph__widthConstrained']")->at(0)->select("span")->to_text()), "\"", "'", "\n", "") << "\"," << std::endl; //
        outJson << "\"tags\": [";
        for (size_t i = 0; i < tags->size(); i++)
        {
            if ((i + 1) == tags->size())
            {
                outJson << "\"" << tags->at(i)->select("a")->select("span")->to_text() << "\"" << std::endl;
                break;
            }
            outJson << "\"" << tags->at(i)->select("a")->select("span")->to_text() << "\"," << std::endl;
        };
        outJson << "]}";
        outJson.close();

        if (remove(filePathHTML.c_str()) != 0)
        {
            perror("Error deleting file");
        }
        else
        {
            puts("File successfully deleted");
        }
        return 0;
    };
    /* Search Routine */
    int search(const std::vector<std::string> &seed)
    {
        // constitute the query
        std::string query;
        for (size_t i = 2; i < seed.size(); i++)
        {
            query += seed.at(i).c_str();
            (i != seed.size() - 1) ? query += "+" : query += "";
        };
        std::string url = searchLink + query;

        const std::string filePathHTML = Iqraa::clearStr(query, "+", " ") + ".html";
        const std::string filePathJSON = "search: " + Iqraa::clearStr(query, "+", " ") + ".json";

        std::fstream outJson;
        outJson.open(filePathJSON, std::ios_base::out);
        if (!outJson.is_open())
        {
            std::cout << "Unable to open the file.\n";
            return 0;
        }
        outJson << "{";

        Iqraa::retrievePage(url, filePathHTML);
        std::ifstream ifs(filePathHTML);
        if (!ifs.is_open())
        {
            std::cout << "Error Open html file: " << filePathHTML << "\n";
            return 0;
        }
        std::string page((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        size_t _iterNr = 0;
        html::parser p;
        html::node_ptr n = p.parse(page);
        html::node_ptr results = n->select("tr[itemtype='http://schema.org/Book']");
        results->size() >= (iterNr - 1) ? _iterNr = (iterNr - 1) : _iterNr = results->size();
        outJson << "\"querys\":[";
        for (size_t i = 0; i < _iterNr; i++)
        {
            outJson << "{";
            outJson << "\"ImageSrc\": "
                    << "\"" << (results->select("a")->select("img")->at(i))->get_attr("src") << "\"," << std::endl; // 	img source
            outJson << "\"Title\": "
                    << "\"" << (results->select("td:first")->select("a")->at(i))->get_attr("title") << "\"," << std::endl; // title
            outJson << "\"Author\": "
                    << "\"" << (results->select("td::eq(1)")->select("span")->select("div")->select("a")->select("span")->at(i))->to_text() << "\"," << std::endl; // img source
            outJson << "\"src\": "
                    << "\"https://www.goodreads.com" << Iqraa::removeQueryParamsAndRank((results->select("td:first")->select("a")->at(i))->get_attr("href")) << "\"" << std::endl; // img source

            if (i == _iterNr - 1)
            {
                outJson << "}";
            }
            else
            {
                outJson << "},";
            }
        }
        outJson << "]}";
        if (remove(filePathHTML.c_str()) != 0)
        {
            perror("Error deleting file");
        }
        else
        {
            puts("File successfully deleted");
        }
        outJson.close();
        return 0;
    };
    /* Author's Data Routine */
    int author(const std::string &seed)
    {

        const std::string filePathHTML = temp_Dir + "/" + "author.html";
        Iqraa::retrievePage(seed, filePathHTML);
        std::ifstream ifs(filePathHTML);
        if (!ifs.is_open())
        {
            std::cout << "Error Open html file: " << filePathHTML << "\n";
            return 0;
        }
        std::string page((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        ifs.close();
        html::parser p;
        html::node_ptr n = p.parse(page);
        html::node_ptr authorData = n->select("d");
        const std::string filePathJSON = temp_Dir + "/" + Iqraa::clearStr((n->select("h1[class='authorName']")->to_text()), "\"", "'", "\n", "") + ".json";
        std::fstream outJson;
        outJson.open(filePathJSON, std::ios_base::out);
        if (!outJson.is_open())
        {
            std::cout << "Unable to open the file.\n";
            return 0;
        }
        outJson << "{";
        outJson << "\"name\": "
                << "\""
                << Iqraa::clearStr((n->select("h1[class='authorName']")->to_text()), "\"", "'", "\n", "")
                << "\"," << std::endl;
        outJson << "\"profile\": "
                << "\""
                << Iqraa::clearStr((n->select("div[class='leftContainer authorLeftContainer']")->select("a")->select("img")->get_attr("src")), "\"", "'", "\n", "")
                << "\"," << std::endl;
        outJson << "\"about\": "
                << "\""
                << "about"
                << "\"," << std::endl;
        outJson << "\"books\": [";
        for (size_t i = 0; i < iterNr; i++)
        {
            outJson << "{";
            outJson << "\"title\": "
                    << "\""
                    << "name of book"
                    << "\"," << std::endl;
            outJson << "\"cover\": "
                    << "\""
                    << "cover of book"
                    << "\"," << std::endl;
            outJson << "\"path\": "
                    << "\""
                    << "path of book"
                    << "\"" << std::endl;
            (i != iterNr - 1) ? outJson << "}," : outJson << "}";
        }
        outJson << "]}";
        outJson.close();
        if (remove(filePathHTML.c_str()) != 0)
        {
            perror("Error deleting file");
        }
        else
        {
            puts("File successfully deleted");
        }
        return 0;
    };
    //TODO: Future !
    /* News Routine */
    int retrieveNEWS()
    {
        /**
        //TODO: Below Websites
        ==> skynewsarabia => content_container
        ==> Jazeera    => news-feed-container
        ==> asharq.com    => infinite-scroll-component
        */
        std::fstream outJson;
        outJson.open((cacheDir + "/" + "news.json"), std::ios_base::out);
        if (!outJson.is_open())
        {
            std::cout << "Unable to open the file.\n";
            return 0;
        }
        outJson << "{";
        for (size_t j = 0; j < 5; j++)
        {
            retrievePage(sites_news[j][1], (cacheDir + "/" + sites_news[j][0] + ".html"));
            std::ifstream ifs((cacheDir + "/" + sites_news[j][0] + ".html"));
            if (!ifs.is_open())
            {
                std::cout << "specify html file\n";
                std::cin.get();
                return 0;
            }
            std::string page((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            ifs.close();
            html::parser p;
            html::node_ptr n = p.parse(page);
            if (j == 0) // "Youm7"
            {
                html::node_ptr youm7 = n->select("div[id='paging'] div[class='col-xs-12 bigOneSec']:lt(5)");
                outJson << "\"youm-7\":[";
                for (size_t i = 0; i < youm7->size(); i++)
                {
                    outJson << "{";
                    outJson << "\"ImageSrc\": "
                            << "\"" << (youm7->select("a")->select("img")->at(i))->get_attr("src") << "\"," << std::endl; // 	img source
                    outJson << "\"Title\": "
                            << "\"" << (youm7->select("a")->select("img")->at(i))->get_attr("title") << "\"," << std::endl; // 	title
                    outJson << "\"link\": "
                            << "\""
                            << "https://youm7.com" << (youm7->select("a")->at(i))->get_attr("href") << "\"," << std::endl; // 	link
                    outJson << "\"Date\": "
                            << "\"" << (youm7->select("div")->select("span")->at(i))->to_text() << "\"," << std::endl; // 	date
                    outJson << "\"info\": "
                            << "\"" << (youm7->select("div")->select("p")->at(i))->to_text() << "\"" << std::endl; // 	info
                    if (i == (youm7->size() - 1))
                    {
                        outJson << "}";
                    }
                    else
                    {
                        outJson << "},";
                    }
                }
                outJson << "],";
            }
            else if (j == 1) // "skyarabia"
            {
                html::node_ptr skyarabia = n->select("div[class='comp_1_item']")->select("a");
                outJson << "\"skyarabia\":[";
                for (size_t i = 0; i < skyarabia->size(); i++)
                {
                    outJson << "{";
                    outJson << "\"ImageSrc\": "
                            << "\"" << (skyarabia->at(i))->select("div[class='aspect_content'] picture img")->get_attr("data-src") << "\"," << std::endl; // 	img source
                    outJson << "\"Title\": "
                            << "\"" << (skyarabia->at(i))->get_attr("title") << "\"," << std::endl; // 	title
                    outJson << "\"link\": "
                            << "\"" << (skyarabia->at(i))->get_attr("href") << "\"," << std::endl; // 	link
                    outJson << "\"Date\": "
                            << "\"" << (skyarabia->at(i))->select("div[class='comp_1_item_date'] span")->to_text() << "\"," << std::endl; // 	date
                    outJson << "\"info\": "
                            << "\"" << (skyarabia->at(i))->get_attr("title") << "\"" << std::endl; // 	info
                    if (i == (skyarabia->size() - 1))
                    {
                        outJson << "}";
                    }
                    else
                    {
                        outJson << "},";
                    }
                }
                outJson << "],";
            }
            else if (j == 2) // "Jazeera"
            {
                html::node_ptr Jazeera = n->select("div[id='#']");
                outJson << "\"Jazeera\":[";
                for (size_t i = 0; i < Jazeera->size(); i++)
                {
                    outJson << "{";
                    outJson << "\"ImageSrc\": "
                            << "\""
                            << "ImageSrc"
                            << "\"," << std::endl; // 	img source
                    outJson << "\"Title\": "
                            << "\""
                            << "Title"
                            << "\"," << std::endl; // 	title
                    outJson << "\"link\": "
                            << "\""
                            << "link"
                            << "\"," << std::endl; // 	link
                    outJson << "\"Date\": "
                            << "\""
                            << "Date"
                            << "\"," << std::endl; // 	date
                    outJson << "\"info\": "
                            << "\""
                            << "info"
                            << "\"" << std::endl; // 	info
                    if (i == (Jazeera->size() - 1))
                    {
                        outJson << "}";
                    }
                    else
                    {
                        outJson << "},";
                    }
                }
                outJson << "],";
            }
            else if (j == 3) // "asharq.com"
            {
                html::node_ptr asharq = n->select("div[id='#']");
                outJson << "\"asharq\":[";
                for (size_t i = 0; i < asharq->size(); i++)
                {
                    outJson << "{";
                    outJson << "\"ImageSrc\": "
                            << "\""
                            << "ImageSrc"
                            << "\"," << std::endl; // 	img source
                    outJson << "\"Title\": "
                            << "\""
                            << "Title"
                            << "\"," << std::endl; // 	title
                    outJson << "\"link\": "
                            << "\""
                            << "link"
                            << "\"," << std::endl; // 	link
                    outJson << "\"Date\": "
                            << "\""
                            << "Date"
                            << "\"," << std::endl; // 	date
                    outJson << "\"info\": "
                            << "\""
                            << "info"
                            << "\"" << std::endl; // 	info
                    if (i == (asharq->size() - 1))
                    {
                        outJson << "}";
                    }
                    else
                    {
                        outJson << "},";
                    }
                }
                outJson << "],";
            }
            else if (j == 4) // "aawsat.com"
            {
                html::node_ptr _aawsat = n->select("div[class='article-item-img video-item node--type-article node--view-mode-teaser']")->select("a[class='']");
                html::node_ptr __aawsat = n->select("div[class='article-item-info']");
                outJson << "\"aawsat\":[";
                for (size_t i = 0; i < 5; i++)
                {
                    outJson << "{";
                    outJson << "\"ImageSrc\": "
                            << "\""
                            << _aawsat->select("img[class='lazy']")->at(i)->get_attr("data-src")
                            << "\"," << std::endl; // 	img source
                    outJson << "\"Title\": "
                            << "\""
                            << clearStr(__aawsat->select("div[class='article-item-title']")->at(i)->to_text(), "\n", "")
                            << "\"," << std::endl; // 	title
                    outJson << "\"link\": "
                            << "\""
                            << _aawsat->at(i)->get_attr("href")
                            << "\"," << std::endl; // 	link
                    outJson << "\"Date\": "
                            << "\""
                            << __aawsat->select("div[class='article-item-meta']")->select("time")->at(i)->to_text()
                            << "\"," << std::endl; // 	date
                    outJson << "\"info\": "
                            << "\""
                            << clearStr(__aawsat->select("div[class='article-item-content']")->at(i)->to_text(), "\n", " ")
                            << "\"" << std::endl; // 	info
                    if (i == 4)
                    {
                        outJson << "}";
                    }
                    else
                    {
                        outJson << "},";
                    }
                }
                outJson << "]";
                // linkToNew = _aawsat->at(i)->get_attr("href");
                // imgOfNew = _aawsat->select("img[class='lazy']")->at(i)->get_attr("data-src");
                // titleOfNew = __aawsat->select("div[class='article-item-title']")->at(i)->to_text();
                // aboutOfNew = __aawsat->select("div[class='article-item-content']")->at(i)->to_text();
                // dateOfNew = __aawsat->select("div[class='article-item-meta']")->select("time")->at(i)->to_text();
            };
            // clean caching before exit loop
            if (remove((cacheDir + "/" + sites_news[j][0] + ".html").c_str()) != 0)
            {
                perror("Error deleting file");
            }
            else
            {
                puts("File successfully deleted");
            }
        }
        outJson << "}";
        outJson.close();
        return 0;
    };
    //TODO: Future !
    /* Store Routine */
    int retrieveStore()
    {
        std::fstream outJson;
        outJson.open((cacheDir + "/" + "store.json"), std::ios_base::out);
        if (!outJson.is_open())
        {
            std::cout << "Unable to open the file.\n";
            return 0;
        }
        outJson << "{";
        for (size_t j = 0; j < 5; j++)
        {
            retrievePage(sites_store[j][1], (cacheDir + "/" + sites_store[j][0] + ".html"));
            std::ifstream ifs((cacheDir + "/" + sites_store[j][0] + ".html"));
            if (!ifs.is_open())
            {
                std::cout << "specify html file\n";
                std::cin.get();
                return 0;
            }
            std::string page((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            ifs.close();
            html::parser p;
            html::node_ptr n = p.parse(page);
            if (j == 0) // "shorouk"
            {
                html::node_ptr shorouk = n->select("div[class='bookListBlock'] ul[class='row margin-zero'] li");
                //-------------------------------- the main Title ----------------------------------------------
                outJson << "\"main\":{";
                outJson << "\"ImageSrc\": "
                        << "\""
                        << shorouk->select("div[class='bookItem']")->at(0)->select("div [class='image']")->select("a")->select("img")->at(0)->get_attr("src")
                        << "\"," << std::endl; //
                outJson << "\"purchaseLink\": "
                        << "\"https://www.shoroukbookstores.com"
                        << shorouk->select("div[class='bookItem']")->at(0)->select("div [class='image']")->select("a")->at(0)->get_attr("href")
                        << "\"," << std::endl; //
                outJson << "\"author\": "
                        << "\""
                        << clearStr((shorouk->select("div[class='bookItem']")->at(0)->select("div [class='bookData']")->select("span")->to_text()), "\n\r", " ")
                        << "\"," << std::endl; //
                outJson << "\"title\": "
                        << "\""
                        << clearStr((shorouk->select("div[class='bookItem']")->at(0)->select("div [class='bookData']")->select("strong")->to_text()), "\n\r", " ")
                        << "\"," << std::endl; //
                outJson << "\"price\": "
                        << "\""
                        << clearStr((shorouk->select("div[class='bookItem']")->at(0)->select("span[class='oriPrice']")->to_text()), "\n\r", " ")
                        << "\"," << std::endl; //
                outJson << "\"discount\": "
                        << "\""
                        << clearStr((shorouk->select("div[class='bookItem']")->at(0)->select("a[class='discount']")->to_text()), "\n\r", " ")
                        << "\"}," << std::endl; //
                //-------------------------------- the main Title ----------------------------------------------
                outJson << "\"shorouk\":[";
                for (size_t i = 1; i < 5; i++)
                {
                    outJson << "{";
                    outJson << "\"ImageSrc\": "
                            << "\""
                            << shorouk->select("div[class='bookItem']")->at(i)->select("div [class='image']")->select("a")->select("img")->at(0)->get_attr("src")
                            << "\"," << std::endl; //
                    outJson << "\"purchaseLink\": "
                            << "\"https://www.shoroukbookstores.com"
                            << shorouk->select("div[class='bookItem']")->at(i)->select("div [class='image']")->select("a")->at(0)->get_attr("href")
                            << "\"," << std::endl; //
                    outJson << "\"author\": "
                            << "\""
                            << clearStr((shorouk->select("div[class='bookItem']")->at(i)->select("div [class='bookData']")->select("span")->to_text()), "\n\r", " ")
                            << "\"," << std::endl; //
                    outJson << "\"title\": "
                            << "\""
                            << clearStr((shorouk->select("div[class='bookItem']")->at(i)->select("div [class='bookData'] strong a")->to_text()), "\n\r", " ")
                            << "\"," << std::endl; //
                    outJson << "\"price\": "
                            << "\""
                            << clearStr((shorouk->select("div[class='bookItem']")->at(i)->select("span[class='oriPrice']")->to_text()), "\n\r", " ")
                            << "\"," << std::endl; //
                    outJson << "\"discount\": "
                            << "\""
                            << clearStr((shorouk->select("div[class='bookItem']")->at(i)->select("a[class='discount']")->to_text()), "\n\r", " ")
                            << "\"" << std::endl; //
                    if (i == 4)
                    {
                        outJson << "}";
                    }
                    else
                    {
                        outJson << "},";
                    }
                }
                outJson << "],";
            }
            else if (j == 1) // "aseeralkotb"
            {
                html::node_ptr aseeralkotb = n->select("");
                outJson << "\"aseeralkotb\":[";
                for (size_t i = 1; i < 5; i++)
                {
                    outJson << "{";
                    outJson << "\"ImageSrc\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"purchaseLink\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"author\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"title\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"price\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"discount\": "
                            << "\""
                            << "Soon"
                            << "\"" << std::endl; //
                    if (i == 4)
                    {
                        outJson << "}";
                    }
                    else
                    {
                        outJson << "},";
                    }
                }
                outJson << "],";
            }
            else if (j == 2) // "jarir"
            {
                html::node_ptr jarir = n->select("div[class='product-tile']");
                outJson << "\"jarir\":[";
                for (size_t i = 1; i < 5; i++)
                {
                    outJson << "{";
                    outJson << "\"ImageSrc\": "
                            << "\""
                            << ""
                            << "\"," << std::endl; //
                    outJson << "\"purchaseLink\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"author\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"title\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"price\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"discount\": "
                            << "\""
                            << "Soon"
                            << "\"" << std::endl; //
                    if (i == 4)
                    {
                        outJson << "}";
                    }
                    else
                    {
                        outJson << "},";
                    }
                }
                outJson << "],";
            }
            else if (j == 3) // "asfar"
            {
                html::node_ptr asfar = n->select("");
                outJson << "\"asfar\":[";
                for (size_t i = 1; i < 5; i++)
                {
                    outJson << "{";
                    outJson << "\"ImageSrc\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"purchaseLink\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"author\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"title\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"price\": "
                            << "\""
                            << "Soon"
                            << "\"," << std::endl; //
                    outJson << "\"discount\": "
                            << "\""
                            << "Soon"
                            << "\"" << std::endl; //
                    if (i == 4)
                    {
                        outJson << "}";
                    }
                    else
                    {
                        outJson << "},";
                    }
                }
                outJson << "]";
            };
            // clean caching before exit loop
            if (remove((cacheDir + "/" + sites_store[j][0] + ".html").c_str()) != 0)
            {
                perror("Error deleting file");
            }
            else
            {
                puts("File successfully deleted");
            }
        }
        outJson << "}";
        outJson.close();
        return 0;
    }
    /* Help Message */
    void print_help()
    {
        std::cout << "Usage: iqraa [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -open <link>                retrive book Data in link to cache Dir" << std::endl;
        std::cout << "  -search <keywords,...>      retrive Search querys to search: keywords.json to cache Dir" << std::endl;
        std::cout << "  -author <link>              retrive author in link to cache Dir" << std::endl;
        std::cout << "  -news                       Generate news Data to cache Dir" << std::endl;
        std::cout << "  -store                      Generate store Data to cache Dir" << std::endl;
        std::cout << "  -help                       Display this help message" << std::endl;
        std::cout << "  -version                    Display version information" << std::endl;
    };
};

int main(int argc, char *argv[])
{
    auto start = std::chrono::steady_clock::now(); // Record start time

    // INIT Iqraa
    Iqraa iqraa;
    if (argc < 2)
    {
        iqraa.print_help();
        return 1;
    };
    // Run For Specific Purpose
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg.substr(0, 1) == "-")
        {
            std::string option = arg.substr(1);
            if (option == "open")
            {
                if (argc <= 2)
                {
                    iqraa.print_help();
                    return 0;
                }
                std::string book = argv[2];
                int result = RETURN_SUCCESS;
                if ((result = iqraa.open(book)))
                {
                    std::cerr << "Error! With Book <link> parameter\n";
                }
            }            
            else if (option == "search")
            {
                std::vector<std::string> query;
                if (argc <= 2)
                {
                    iqraa.print_help();
                    return 0;
                }
                for (size_t j = 0; j < argc; j++)
                {
                    query.emplace_back(argv[j]);
                }
                int result = RETURN_SUCCESS;
                if ((result = iqraa.search(query)))
                {
                    std::cerr << "Sorry! Something went wrong\n";
                }
            }
            else if (option == "author")
            {
                if (argc <= 2)
                {
                    iqraa.print_help();
                    return 0;
                }
                std::string _author = argv[2];
                int result = RETURN_SUCCESS;
                if ((result = iqraa.author(_author)))
                {
                    std::cerr << "Sorry! Something went wrong\n";
                }
            }
            else if (option == "news")
            {
                int result = RETURN_SUCCESS;
                if ((result = iqraa.retrieveNEWS()))
                {
                    std::cerr << "Sorry! Something went wrong\n";
                }
            }
            else if (option == "store")
            {
                int result = RETURN_SUCCESS;
                if ((result = iqraa.retrieveStore()))
                {
                    std::cerr << "Sorry! Something went wrong\n";
                }
            }         
            else if (option == "help")
            {
                iqraa.print_help();
            }
            else if (option == "version")
            {
                std::cout << "Iqraa version: ("<< IQRAA_VERSION << ")  [https://github.com/Mhmoud-Atiyah/Iqraa]" << std::endl;
            }
        }
    }
    system("clear");
    auto end = std::chrono::steady_clock::now(); // Record end time
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Execution time: " << duration.count() << " milliseconds" << std::endl;

    return 0;
};