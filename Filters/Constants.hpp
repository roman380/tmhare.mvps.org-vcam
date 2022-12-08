#pragma once

#include <string>
#include <spdlog/spdlog.h>
#include <magic_enum.hpp>

static const std::string name = "ShareTheBoard Virtual Camera";
static const auto wname = std::wstring(name.begin(), name.end());
static const std::string logger_name = "ContentCamera";

#define TRY_LOG(expr)                   \
    try {                               \
        auto logger = spdlog::get(logger_name);\
        if (logger)                     \
        logger->expr;                         \
    }                                   \
    catch (const spdlog::spdlog_ex& ex){\
    }                                   \
    catch (...) {                       \
    }
