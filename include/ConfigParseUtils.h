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

    // Normalizes a config line:
    // 1) strips inline comments, 2) trims spaces, 3) skips empty/full-line comments
    inline bool PrepareConfigLine(
        std::string& line,
        char inlineCommentMarker = '#',
        char fullLineCommentMarker = '|')
    {
        const size_t commentPos = line.find(inlineCommentMarker);
        if (commentPos != std::string::npos)
            line.erase(commentPos);

        const size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return false;
        if (line[first] == fullLineCommentMarker) return false;

        const size_t last = line.find_last_not_of(" \t\r\n");
        line = line.substr(first, last - first + 1);

        return !line.empty();
    }
}