#pragma once

#include <string>
#include <print>
#include <format>
#include <fstream>

namespace Context {
	inline std::ofstream m_stream{};
	inline bool m_streamOpen{};
	
	template<typename... Args>
	void Spew(std::string_view aFormatString, Args&&... aArgs) {
		auto str = std::vformat(aFormatString, std::make_format_args(aArgs...));

		std::println("- {}", str);

		if (m_streamOpen) {
			m_stream << "- " << str << "\n";
			m_stream.flush();
		}
	}

	template<typename... Args>
	void Error(std::string_view aFormatString, Args&&... aArgs) {
		auto str = std::vformat(aFormatString, std::make_format_args(aArgs...));

		std::println("- [Error] {}", str);

		if (m_streamOpen) {
			m_stream << "- [Error] " << str << "\n";
			m_stream.flush();
		}
	}
}