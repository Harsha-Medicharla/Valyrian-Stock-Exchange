#include <cstdlib>
#include <iostream>

int main()
{
    std::cout << "api_server: Drogon was not found at configure time.\n"
                 "Install Drogon (e.g. `brew install drogon`) and re-run CMake, or set Drogon_DIR.\n";
    return EXIT_SUCCESS;
}
