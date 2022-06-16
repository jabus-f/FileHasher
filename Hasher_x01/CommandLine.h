#pragma once


#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <string>




class CommandLine {
public:
	CommandLine(int argc, char** argv) {
		boost::program_options::command_line_parser parser(argc, argv);
		options = new boost::program_options::parsed_options(parser.run());
	}

	template<typename T>
	T get_arg(int index, T const& def) const { // optional
		return index < options->options.size() ? m_get_arg<T>(index) : def;
	}

	template<typename T>
	T get_arg(int index) const { // required
		if (index < options->options.size())
			return m_get_arg<T>(index);
		else
			throw std::runtime_error("Expected argument " + std::to_string(index));
	}




public:
	template<typename T>
	T m_get_arg(int index) const {
		return boost::lexical_cast<T>(options->options[index].string_key);
	}
	template<>
	std::string m_get_arg<std::string>(int index) const {
		return options->options[index].string_key;
	}



private:
	boost::program_options::parsed_options* options;


};












