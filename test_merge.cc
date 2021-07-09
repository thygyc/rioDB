#include "DMS.hpp"

int main()
{
    DMScore DMSmanage(100);
    DMSmanage.init();
    DMSmanage.Insert(std::string("Tom"), std::string("1234"));
    DMSmanage.Insert(std::string("Deo"), std::string("4567"));
    DMSmanage.Insert(std::string("Cuk"), std::string("0123"));
    DMSmanage.Insert(std::string("Tom"), std::string("abcd"));
    DMSmanage.Insert(std::string("Deo"), std::string("efgh"));
    DMSmanage.Insert(std::string("Cuk"), std::string("rstd"));
    DMSmanage.Delete(std::string("Tom"));
    DMSmanage.Insert(std::string("Qcy"), std::string("ghjk"));
    DMSmanage.Insert(std::string("Amd"), std::string("1234"));
    DMSmanage.Merge();
    return 0;
}