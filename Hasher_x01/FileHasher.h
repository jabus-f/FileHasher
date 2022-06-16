#pragma once


#include <iostream>
#include <filesystem>
#include <condition_variable>
#include <mutex>
#include <fstream>
#include <boost/thread/concurrent_queues/sync_queue.hpp>




class FileHasher {
public:
	using Path = std::filesystem::path;

	class Hasher {
	public:
		virtual size_t input_size() const = 0;
		virtual size_t output_size() const = 0;
		virtual void operator()(std::byte* out, std::byte* in) const = 0;
	};


public:
	FileHasher() : cache_line(256) {}
	~FileHasher() {}

	void set_hasher(Hasher* hasher);
	void set_memlimit(size_t limit);
	void open(Path input, Path output);
	size_t set_thread_count(size_t count);
	void do_hash();



private:
	struct Block { 
		Block() = default;
		Block(Block&&) noexcept = default;
		Hasher* hasher;
		std::byte* data;
		std::streampos position;
	};

	struct BlocksPack {
		BlocksPack() = default;
		BlocksPack(BlocksPack&&) noexcept = default;
		BlocksPack& operator=(BlocksPack&&) noexcept = default;
		std::vector<Block> blocks;
		std::unique_ptr<std::byte[]> raw;
	};

	void worker();


public:
	size_t cache_line;


private:
	boost::concurrent::sync_queue<BlocksPack> m_blocks;
	std::mutex m_console_mutex;
	std::mutex m_output_mutex;
	Hasher* m_hasher;
	Path m_input_path; 
	Path m_output_path;
	std::ifstream m_input; 
	std::ofstream m_output;
	size_t m_thread_count;
	size_t m_memlimit;
	std::atomic<size_t> m_memsize;


};





