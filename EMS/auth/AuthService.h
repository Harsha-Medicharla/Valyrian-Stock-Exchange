#pragma once
#include <cstdint>

namespace EMS {

class AuthService {
public:
    bool isAuthorized(uint64_t) const;
};

}
