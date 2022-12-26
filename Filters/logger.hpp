#pragma once

#include <spdlog/spdlog.h>
#include <magic_enum.hpp>

namespace content_camera
{
	class logger
	{

#define TRY_LOG(expr)                   \
    try {                               \
        auto l = spdlog::get(logger_name);\
        if (l)                     \
        l->expr;                         \
    }                                   \
    catch (const spdlog::spdlog_ex& ex){\
    }                                   \
    catch (...) {                       \
    }
	public:
		logger();
		~logger();

		template<typename... Args>
		void trace(const std::string& msg, Args... args)
		{
			TRY_LOG(trace(msg, args...));
		}

		template<typename... Args>
		void debug(const std::string& msg, Args... args)
		{
			TRY_LOG(debug(msg, args...));
		}

		template<typename... Args>
		void info(const std::string& msg, Args... args)
		{
			TRY_LOG(info(msg, args...));
		}

		template<typename... Args>
		void warn(const std::string& msg, Args... args)
		{
			TRY_LOG(warn(msg, args...));
		}

		template<typename... Args>
		void error(const std::string& msg, Args... args)
		{
			TRY_LOG(error(msg, args...));
		}

	private:
		static const std::string logger_name;

		std::shared_ptr<spdlog::logger> m_logger;
	};
}
