#pragma once
#include <cstdint>

namespace EMS {

class AuthService {
public:
    AuthService() = default;
    // REMOVED 'virtual' - This fixes the alignment crash
    ~AuthService() = default;

    // Direct, non-virtual call
    bool isAuthorized(uint64_t user_id) const {
        return user_id != 0; 
    }
};

}