#include "logging.hpp"
namespace logging = boost::log;
using namespace logging::trivial;

bool init_log_environment(std::string cfg, std::string log_wd) {
    if (!boost::filesystem::exists(log_wd)) {
        boost::filesystem::create_directory(log_wd);
    }
    logging::add_common_attributes();

    logging::register_simple_formatter_factory<severity_level, char>("Severity");
    logging::register_simple_filter_factory<severity_level, char>("Severity");

    std::ifstream file(cfg);
    try {
        logging::init_from_stream(file);
    } catch (const std::exception& e) {
        std::cerr << "init_logger is fail, read log config file fail. curse: "
                  << e.what() << std::endl;
        exit(-2);
    }
    return true;
}