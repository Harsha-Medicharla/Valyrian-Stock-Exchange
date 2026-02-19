#include "auth/AuthService.h"

namespace EMS {

bool AuthService::isAuthorized(uint64_t user_id) const {
    // Reject user 0, authorize everyone else
    return user_id != 0;
}

}