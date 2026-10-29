// A C++ program that includes the installed header and calls the library.
// Exits 0 and prints one fixed line on success, 1 otherwise.
#include <schwaystation.h>
#include <cstdio>
#include <cstring>

int main()
{
    schway_versions v;
    std::memset(&v, 0, sizeof v);
    v.size = static_cast<uint32_t>(sizeof v);
    if (schway_get_versions(&v) != SCHWAY_STATUS_OK) {
        std::printf("schway_get_versions failed\n");
        return 1;
    }
    std::printf("consumer_cxx: api %u\n", static_cast<unsigned>(v.api));
    return 0;
}
