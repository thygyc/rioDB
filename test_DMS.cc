#include "DMS.hpp"

int main()
{
    DMScore DMSmanage(100);
    DMSmanage.init();
    DMSmanage.Insert(std::string("Tmo"), std::string("1234"));
    DMSmanage.Insert(std::string("Dom"), std::string("4567"));
    DMSmanage.Insert(std::string("Nck"), std::string("0123"));
    DMSmanage.Insert(std::string("Tmo"), std::string("abcd"));
    std::string value1, value2;
    DMSmanage.Query(std::string("Tmo"), value1);
    DMSmanage.Delete(std::string("Nck"));
    DMSmanage.Query(std::string("Nck"), value2);
    printf("%s\n", value1.c_str());
    printf("%s\n", value2.c_str());
    return 0;
}