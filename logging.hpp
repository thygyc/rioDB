#ifndef RIOCCTAN_LOGGING_HPP
#define RIOCCTAN_LOGGING_HPP

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <string>
#include <fstream>

namespace logging = boost::log;
using namespace logging::trivial;

#define DEFAULT_LOG_CONFIG "log.cfg"  // log 默认文件名
#define DEFAULT_LOG_WD_CONFIG "./log/"  // log 文件所在默认目录

#define DB_LOG_SEV(scl, level, message)                                 \
    BOOST_LOG_SEV((scl), (level)) << __FILE__ << ":" << __LINE__ << " " \
                                  << __FUNCTION__ << "() " << (message)

#define DB_LOG(level, message)                                     \
    BOOST_LOG_TRIVIAL(level) << __FILE__ << ":" << __LINE__ << " " \
                             << __FUNCTION__ << "() " << (message)

// 条件 cond 为真，则记录
#define DB_CHECK(cond, level, message) \
    do {                               \
        if (cond) {                    \
            DB_LOG(level, message);    \
        }                              \
    } while (0)

// 条件 cond 为真则记录，并返回错误码 err_code
#define ErrCheck(cond, level, err_code, message) \
    do {                                         \
        if (cond) {                              \
            DB_LOG(level, message);              \
            return (err_code);                   \
        }                                        \
    } while (0)

bool init_log_environment(std::string cfg = DEFAULT_LOG_CONFIG, std::string log_wd = DEFAULT_LOG_WD_CONFIG);
#endif