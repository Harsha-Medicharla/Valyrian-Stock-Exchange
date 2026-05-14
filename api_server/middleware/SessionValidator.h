#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

#include <drogon/HttpFilter.h>

class SessionValidator : public drogon::HttpFilter<SessionValidator>
{
public:
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback &&fcb,
                  drogon::FilterChainCallback &&fccb) override;

    static std::optional<uint64_t> userId(const drogon::HttpRequestPtr &req);
    static std::optional<std::string_view> bearerToken(std::string_view header);
};
