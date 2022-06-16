#include "FileHasher.h"

#include <fstream>
#include <boost/math/common_factor.hpp>
#include <numeric>




void FileHasher::set_hasher(Hasher* hasher) {
    if (hasher != nullptr)
        m_hasher = hasher;
    else
        throw std::logic_error("Hasher == nullptr");
}

void FileHasher::set_memlimit(size_t limit) {
    m_memlimit = limit;
}

void FileHasher::open(Path input, Path output) {
    try {
        m_input_path = input;
        m_output_path = output;

        m_input.exceptions(std::ios::badbit);
        m_input.open(input);

        m_output.exceptions(std::ios::badbit);
        m_output.open(output);
    }
    catch (...) {
        m_input.close();
        m_output.close();
        throw;
    }
}


size_t get_min_optimal_block_count(size_t block_size, size_t cache_line) {
    size_t clmod = block_size % cache_line;
    size_t gcd = boost::math::gcd(clmod, cache_line);
    return std::max<size_t>(64 * 1024 * 1024 / block_size, cache_line / gcd);
}

size_t FileHasher::set_thread_count(size_t count) {
    size_t block_size = m_hasher->input_size();
    size_t file_size = std::filesystem::file_size(m_input_path);
    size_t min_opt_block_count = std::min<size_t>(get_min_optimal_block_count(block_size, cache_line), 1);

    if (count == 0) {
        count = std::max<size_t>(std::thread::hardware_concurrency(), 1);
    }

    size_t max_worker_count = file_size / block_size / min_opt_block_count;
    if (max_worker_count < count) {
        count = std::max<size_t>(max_worker_count, 1);
    }

    return m_thread_count = count;
}


void FileHasher::do_hash() {
    size_t block_size = m_hasher->input_size();

    size_t file_size = std::filesystem::file_size(m_input_path);
    size_t size_without_last = file_size - file_size % block_size;
    size_t min_opt_block_count = std::max<size_t>(get_min_optimal_block_count(block_size, cache_line), 1);

    std::vector<std::thread> threads(m_thread_count);
    for (auto& thr : threads)
        thr = std::thread([&] { worker(); });

    size_t block_count = size_without_last / block_size + (file_size - size_without_last ? 1 : 0);
    std::streampos position = 0;
    for (size_t i = 0; i <= block_count;) {
        while (m_memsize > m_memlimit)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        size_t count = std::min<size_t>(std::max<size_t>(block_count - i, 1), min_opt_block_count);
        std::unique_ptr<std::byte[]> data(new std::byte[block_size * count]);

        if (i == block_count) {
            memset(data.get(), 0, block_size);
        }

        m_input.read(reinterpret_cast<char*>(data.get()), block_size * count);

        std::vector<Block> blocks;
        for (size_t j = 0; j < count; j++) {
            Block block;
            block.hasher = m_hasher;
            block.position = position + std::streamoff(j * block.hasher->output_size());
            block.data = data.get() + j * block_size;
            blocks.push_back(std::move(block));
        }

        m_blocks.push(BlocksPack{std::move(blocks), std::move(data)});
        m_memsize += block_size * count;

        i += count;
        position += count * m_hasher->output_size();
    }

    for (auto& thr : threads)
        m_blocks.push({}); // eof

    for (auto& thr : threads)
        thr.join();
}

void FileHasher::worker() {
    static int worker_index = 0;
    int index = worker_index++;
    std::unique_ptr<std::byte[]> out;
    size_t out_size = 0;
    size_t sum_size = 0;

    while (true) {
        sum_size = 0;
        auto s = std::chrono::steady_clock::now();
        BlocksPack blocks_pack;
        m_blocks.pull(blocks_pack);
        std::vector<Block>& blocks = blocks_pack.blocks;
        auto poll_size = m_blocks.size();
        if (blocks.empty()) // eof
            return;

        for (auto& block : blocks) {
            if (out_size < block.hasher->output_size()) {
                out = std::unique_ptr<std::byte[]>(new std::byte[block.hasher->output_size()]);
                out_size = block.hasher->output_size();
            }
            std::lock_guard lg_(m_output_mutex);
            (*block.hasher)(out.get(), block.data);
            if (block.position != m_output.tellp())
                m_output.seekp(block.position);
            m_output.write(reinterpret_cast<const char*>(out.get()), block.hasher->output_size());
            sum_size += block.hasher->input_size();
            m_memsize -= block.hasher->input_size();
        }

        {
            std::lock_guard lg_(m_console_mutex);
            auto e = std::chrono::steady_clock::now();
            std::cout << "Worker " << index << ": "
                << "Blocks [" << blocks[0].position << "-" << blocks.back().position << "] processed "
                << "in " << std::chrono::nanoseconds(e - s).count() / 1000'000.0 << "ms, "
                << "poll size : " << poll_size
                << "\n";
        }
    }
}
