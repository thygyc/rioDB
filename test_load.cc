#include "DMS.hpp"

int main()
{
    DMScore DMSmanage(100);
    std::string rvalue1, rvalue2, rvalue3, rvalue4;
    DMSmanage.Query("Cuk", rvalue1);
    DMSmanage.Query("Deo", rvalue2);
    DMSmanage.Query("Qcy", rvalue3);
    DMSmanage.Query("Amd", rvalue4);
    std::cout << rvalue1 << std::endl << rvalue2 << std::endl << rvalue3 << std::endl << rvalue4 << std::endl;
    
    return 0;
}