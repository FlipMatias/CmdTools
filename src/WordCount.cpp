#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <chrono>
#include <string.h>
#include <assert.h>
using namespace std;
using WordMap = unordered_map<string, size_t>;


struct Word
{
    Word() {}
    Word(string_view w, size_t c) : word{w}, count{c} {}

    bool operator == (const Word& w) const { return (this->count == w.count); }
    bool operator <  (const Word& w) const { return (this->count >  w.count); }

    string word  = "";
    size_t count = 0;
};


namespace globals {
    string filename;
}


void parseFile(string_view filename);
void displayData(const WordMap& words);
void rearrangeData(const WordMap& words);
void saveTo(string_view filepath, const multiset<Word>& words);
auto getOutputFilename(string inputFilename) -> string;
auto split(const string& str, string_view delims = ",.\';?-[]:!() ") -> vector<string>;
auto join(vector<string>::iterator start, vector<string>::iterator end) -> string;
auto getline(char** line, size_t* len, FILE* fp) -> int64_t;
auto capitalize(char* str) -> char*;
auto isWord(const char* str) -> bool;

template <typename T> void showTimeStats(T timePoint);


int main(int argc, char** argv)
{
    if (argc < 2) {
        cout << "| Usage: wcount [filename]\n\n";
        return -1;
    }

    globals::filename = argv[1];
    parseFile(globals::filename);

    printf("\n\n");
    return 0;
}


void parseFile(string_view filename)
{
    using namespace std::chrono;

    size_t bytes = 4096;
    char* buffer = (char*)malloc(sizeof(char) * bytes);

    FILE* fp = fopen(filename.data(), "r");
    if (!fp) {
        cout << "Failed to open '" << filename << "'\n";
        free(buffer);
        exit(-1);
    }

    auto timeStart = high_resolution_clock::now();

    WordMap words;
    const char* delim = ",.\';?-[]:!() \n\t\r<>";

    while (getline(&buffer, &bytes, fp) != -1)
    {
        char* tok = strtok(buffer, delim);
        while (tok != nullptr) {
            if (isWord(capitalize(tok))) {
                words[tok]++;
            }

            tok = strtok(nullptr, delim);
        }
    }

    fclose(fp);
    free(buffer);

    rearrangeData(words);

    auto timeEnd = high_resolution_clock::now();
    showTimeStats(duration_cast<microseconds>(timeEnd - timeStart));
}


void rearrangeData(const WordMap& words)
{
    multiset<Word> sortedWords;

    for (auto&[word, count] : words) {
        sortedWords.emplace(word, count);
    }

    saveTo(getOutputFilename(globals::filename), sortedWords);
}


void saveTo(string_view filepath, const multiset<Word>& words)
{
    ofstream out;
    out.open(filepath.data());
    if (!out.is_open()) {
        cout << "Failed to create 1" << filepath << "'\n";
        exit(-1);
    }

    for (const Word& w : words) {
        out << w.word << " - " << w.count << '\n';
    }
}


auto getOutputFilename(string inputFilename) -> string
{
    for (char& c : inputFilename) {
        if (c == '\\') {
            c = '/';
        }
    }

    string fname = split(inputFilename, "/").back();
    auto fnToks = split(fname, ".");
    fnToks.pop_back();
    fname = join(fnToks.begin(), fnToks.end());
    return fname + ".wcount.txt";
}


void displayData(const WordMap& words)
{
    for (auto& p: words) {
        cout << p.first << " - " << p.second << '\n';
    }
}


auto split(const string& str, string_view delim) -> vector<string>
{
    stringstream stringStream(str);
    string line;
    vector<string> words;

    while (getline(stringStream, line))
    {
        size_t prev = 0, pos;
        while ((pos = line.find_first_of(delim, prev)) != string::npos)
        {
            if (pos > prev)
                words.push_back(line.substr(prev, pos-prev));
            prev = pos+1;
        }
        if (prev < line.length())
            words.push_back(line.substr(prev, string::npos));
    }

    return words;
}


auto join(vector<string>::iterator start, vector<string>::iterator end) -> string
{
    string str;
    while (start != end) {
        str += *start;
        start++;
    }

    return str;
}


auto getline(char** line, size_t* len, FILE* fp) -> int64_t
{
    if (!line || !len || !fp) {
        errno = EINVAL;
        return -1;
    }

    char chunk[128];

    if(*line == NULL || *len < sizeof(chunk)) {
         *len = sizeof(chunk);
         if((*line = malloc(*len)) == NULL) {
             errno = ENOMEM;
             return -1;
         }
     }

    (*line)[0] = '\0';

    while(fgets(chunk, sizeof(chunk), fp) != NULL)
    {
        size_t len_used = strlen(*line);
        size_t chunk_used = strlen(chunk);

        if(*len - len_used < chunk_used) {
            if(*len > SIZE_MAX / 2) {
                errno = EOVERFLOW;
                return -1;
            } else {
                *len *= 2;
            }

            if((*line = realloc(*line, *len)) == NULL) {
                errno = ENOMEM;
                return -1;
            }
        }

        memcpy(*line + len_used, chunk, chunk_used);
        len_used += chunk_used;
        (*line)[len_used] = '\0';

        if((*line)[len_used - 1] == '\n') {
            return (ssize_t)len_used;
        }
    }

    return -1;
}


auto capitalize(char* str) -> char*
{
    if (strlen(str) == 0)
        return str;

    char* c = str;
    for (; c != 0; ++c)
        *c = tolower(*c);

    str[0] = static_cast<char>(toupper(str[0]));
    return str;
}


auto isWord(const char* str) -> bool
{
    if (strlen(str) == 1) {
        if (str[0] != 'A' ||
            str[0] != 'E' ||
            str[0] != 'I' ||
            str[0] != 'O' ||
            str[0] != 'U' ||
            str[0] != 'a' ||
            str[0] != 'e' ||
            str[0] != 'i' ||
            str[0] != 'o' ||
            str[0] != 'u' )
            return false;
    }
    return true;
}


template <typename T>
void showTimeStats(T timePoint)
{
    auto millis = timePoint.count() * 0.001;
    cout << "Time ellapsed: "
         << timePoint.count()
         << " us | " << millis
         << " ms\n";
}

