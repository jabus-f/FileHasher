#pragma once


#include "FileHasher.h"

#include <boost/uuid/detail/md5.hpp>



class CRC32Hasher: public FileHasher::Hasher {
public:
    CRC32Hasher(size_t block_size): block_size(block_size) {}

    virtual size_t input_size() const override {
        return block_size;
    }
    virtual size_t output_size() const override {
        return 4;
    }

    virtual void operator()(std::byte* out, std::byte* in) const override {
        for (size_t i = 0; i < block_size; i++) {
            out[i % 4] = std::byte(uint8_t(out[i % 4]) ^ (uint8_t(in[i]) + uint8_t(i % 256)));
        }
    }


public:
    const size_t block_size;

};


class MD5Hasher: public FileHasher::Hasher {
public:
    MD5Hasher(size_t block_size): block_size(block_size) {}

    virtual size_t input_size() const override {
        return block_size;
    }
    virtual size_t output_size() const override {
        return sizeof(boost::uuids::detail::md5::digest_type);
    }

    virtual void operator()(std::byte* out, std::byte* in) const override {
        boost::uuids::detail::md5 hash;
        boost::uuids::detail::md5::digest_type digest;
        hash.process_bytes(in, block_size);
        hash.get_digest(digest);
        std::copy(reinterpret_cast<const std::byte*>(&digest), reinterpret_cast<const std::byte*>(&digest + 1), out);
    }


public:
    const size_t block_size;

};

class NoneHasher: public FileHasher::Hasher {
public:
    NoneHasher(size_t block_size): block_size(block_size), data("NONENONENONENONE") {}

    virtual size_t input_size() const override {
        return block_size;
    }
    virtual size_t output_size() const override {
        return 16;
    }

    virtual void operator()(std::byte* out, std::byte* in) const override {
        std::copy(reinterpret_cast<const std::byte*>(data), reinterpret_cast<const std::byte*>(data + 16), out);
    }


public:
    const char* data;
    const size_t block_size;

};











