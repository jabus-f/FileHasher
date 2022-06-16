

#include <iostream>
#include <sstream>
#include <chrono>

#include "FileHasher.h"
#include "Hashers.h"
#include "CommandLine.h"




std::string upper(std::string const& str) {
    std::string upper = str;
    for (auto& c : upper)
        c = std::toupper(c);
    return upper;
}

std::string lower(std::string const& str) {
    std::string lower = str;
    for (auto& c : lower)
        c = std::tolower(c);
    return lower;
}


int main(int argc, char** argv) {
    std::setlocale(LC_ALL, "rus"); // for fstream exceptions...

    if (argc < 3) {
        std::cerr << "Too few arguments\n";
        return 1;
    }

    FileHasher::Path in, out;
    std::string hasher_name = "none";
    size_t thread_count = 0;
    size_t block_size = 1024 * 1024;
    size_t max_memory = 1024 * 1024 * 200; // 200MB

    FileHasher::Hasher* hasher = nullptr;
    
    try {
        if (argc < 3)
            throw std::runtime_error("To few arguments");
        in = argv[1];
        out = argv[2];

        if (argc > 3)
            block_size = boost::lexical_cast<size_t>(argv[3]);
        if (argc > 4)
            hasher_name = argv[4];
        if (argc > 5)
            thread_count = boost::lexical_cast<size_t>(argv[5]);
        if (argc > 6)
            max_memory = boost::lexical_cast<size_t>(argv[6]) * 1024;

        if (upper(hasher_name) == "MD5")
            hasher = new MD5Hasher(block_size);
        else if (upper(hasher_name) == "CRC32")
            hasher = new CRC32Hasher(block_size);
        else if (upper(hasher_name) == "NONE")
            hasher = new NoneHasher(block_size);
        else 
            throw std::runtime_error("Unsupported hasher name");
    }
    catch (boost::bad_lexical_cast const& err) {
        std::cerr << err.what() << "\n";
        return 1;
    }
    catch (std::runtime_error const& err) {
        std::cerr << err.what() << "\n";
        return 1;
    }

    std::cout.sync_with_stdio(false);

    auto start = std::chrono::steady_clock::now();

    try {
        FileHasher file_hasher;
        file_hasher.open(in, out);
        file_hasher.set_memlimit(max_memory);
        file_hasher.set_hasher(hasher);
        std::cout << "thread will be used: " << file_hasher.set_thread_count(thread_count) << "\n";
        file_hasher.do_hash();
    }
    catch (std::runtime_error const& err) {
        std::cerr << "Runtime error: " << err.what() << "\n";
        return 1;
    }

    auto stop = std::chrono::steady_clock::now();

    std::cout << "Finished in " << std::chrono::nanoseconds(stop - start).count() / 1'000'000.0 << "ms\n";
    
    return 0;
}






