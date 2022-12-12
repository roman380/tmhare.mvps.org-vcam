#include "logger.hpp"

#include <windows.h>
#include <shlobj.h>

#include <spdlog/sinks/rotating_file_sink.h>

const std::string content_camera::logger::logger_name = "ContentCamera";

std::string GetMyDocumentsFolderPath()
{
    wchar_t folder[1024];
    HRESULT hr = SHGetFolderPathW(0, CSIDL_MYDOCUMENTS, 0, 0, folder);
    if (SUCCEEDED(hr))
    {
        char str[1024];
        wcstombs(str, folder, 1023);
        return str;
    }
    else return "";
}

content_camera::logger::logger()
{
    try
    {
        auto max_size = 1048576 * 5; // 5mb
        auto max_files = 10;
        auto document_path = GetMyDocumentsFolderPath();
        if (!document_path.empty())
        {
            m_logger = spdlog::rotating_logger_mt(logger_name, document_path + "/STBVirtualCamera/ContentCamera.log", max_size, max_files);
            spdlog::set_default_logger(m_logger);
            spdlog::set_level(spdlog::level::info);
            spdlog::set_pattern("[%H:%M:%S %z] [%n] [%l] [process %P] [thread %t] %v");
        }
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        // TODO
    }
}
