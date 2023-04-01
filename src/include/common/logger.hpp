#pragma once

#ifndef SPDLOG_TRACE_ON
#define SPDLOG_TRACE_ON
#endif

#ifndef SPDLOG_DEBUG_ON
#define SPDLOG_DEBUG_ON
#endif

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <cassert>
#include <memory>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/spdlog.h>

namespace TinyIM {
class Logger {
public:
  static Logger &getInstance() {
    static Logger m_instalce;
    return m_instalce;
  }

  auto getLogger() { return m_logger; }

private:
  Logger() {
    std::vector<spdlog::sink_ptr> sinkList;
    auto basicSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        "logs/basicSink.txt");
    basicSink->set_level(spdlog::level::debug);
    basicSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%5l%$] [thread %t] %v");
    sinkList.push_back(basicSink);
    m_logger = std::make_shared<spdlog::logger>("both", std::begin(sinkList),
                                                std::end(sinkList));
    spdlog::register_logger(m_logger);

#ifdef NDEBUG
    m_logger->set_level(spdlog::level::info);
#else
    m_logger->set_level(spdlog::level::trace);
#endif
    //设置 logger 格式[%^%L%$]
    // m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%5l]  %v");
    // m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%5l%$]  %v");
    m_logger->flush_on(spdlog::level::err);
    spdlog::flush_every(std::chrono::seconds(3));
  }

  ~Logger() { spdlog::drop_all(); }
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

private:
  std::shared_ptr<spdlog::logger> m_logger;
};

#define LDebug(...)                                                            \
  TinyIM::Logger::getInstance().getLogger()->debug(__VA_ARGS__)
#define LInfo(...) TinyIM::Logger::getInstance().getLogger()->info(__VA_ARGS__)
#define LWarn(...) TinyIM::Logger::getInstance().getLogger()->warn(__VA_ARGS__)
#define LError(...)                                                            \
  TinyIM::Logger::getInstance().getLogger()->error(__VA_ARGS__)
#define LCritical(...)                                                         \
  TinyIM::Logger::getInstance().getLogger()->critical(__VA_ARGS__)

} // namespace TinyIM