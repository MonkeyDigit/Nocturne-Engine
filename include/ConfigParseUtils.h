#pragma once

#include <sstream>
#include <string>

namespace ParseUtils
{
    template <typename... Args>
    bool TryReadExact(std::stringstream& stream, Args&... args)
    {
        if (!((stream >> args) && ...))
            return false;

        std::string trailing;
        return !(stream >> trailing); // Reject extra unexpected tokens
    }
}