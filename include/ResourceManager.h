#pragma once
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include "Utilities.h"
#include "EngineLog.h"
#include "ConfigParseUtils.h"

template<typename Derived, typename T>
class ResourceManager
{
public:
    ResourceManager(const std::string& pathsFile)
        : m_pathsFile(pathsFile)
    {
        LoadPaths(pathsFile);
    }

    virtual ~ResourceManager() { PurgeResources(); }

    T* GetResource(const std::string& id)
    {
        auto res = Find(id);
        return (res ? res->first.get() : nullptr); // .get() extracts the raw pointer from unique_ptr
    }

    std::string GetPath(const std::string& id)
    {
        auto path = m_paths.find(id);
        return (path != m_paths.end() ? path->second : "");
    }

    bool RequireResource(const std::string& id)
    {
        if (id.empty())
        {
            EngineLog::WarnOnce(
                "resource.require.empty." + m_pathsFile,
                "RequireResource called with an empty id for '" + m_pathsFile + "'");
            return false;
        }

        auto res = Find(id);
        if (res)
        {
            ++res->second;
            return true;
        }

        auto path = m_paths.find(id);
        if (path == m_paths.end())
        {
            EngineLog::WarnOnce(
                "resource.require.unknown_id." + m_pathsFile + "." + id,
                "Unknown resource id '" + id + "' requested in '" + m_pathsFile + "'");
            return false;
        }

        std::unique_ptr<T> resource = Load(path->second);
        if (!resource)
        {
            EngineLog::ErrorOnce(
                "resource.require.load_failed." + m_pathsFile + "." + id,
                "Failed to load resource '" + id + "' from path '" + path->second + "'");
            return false;
        }

        m_resources.emplace(id, std::make_pair(std::move(resource), 1));
        return true;
    }

    bool ReleaseResource(const std::string& id)
    {
        if (id.empty())
        {
            EngineLog::WarnOnce(
                "resource.release.empty." + m_pathsFile,
                "ReleaseResource called with an empty id for '" + m_pathsFile + "'");
            return false;
        }

        auto res = Find(id);
        if (!res)
        {
            EngineLog::WarnOnce(
                "resource.release.unknown_id." + m_pathsFile + "." + id,
                "ReleaseResource called for unknown id '" + id + "' in '" + m_pathsFile + "'");
            return false;
        }

        // Defensive guard: reference count should never be zero here.
        if (res->second == 0)
        {
            EngineLog::WarnOnce(
                "resource.release.zero_ref." + m_pathsFile + "." + id,
                "Resource '" + id + "' has invalid zero ref-count before release. Unloading.");
            Unload(id);
            return false;
        }

        --res->second;
        if (res->second == 0)
            Unload(id);

        return true;
    }

    void PurgeResources()
    {
        // unique_ptr automatically destroys objects when the map clears
        m_resources.clear();
    }

    std::unique_ptr<T> Load(const std::string& path)
    {
        return static_cast<Derived*>(this)->Load(path);
    }

    std::pair<std::unique_ptr<T>, unsigned int>* Find(const std::string& id)
    {
        auto itr = m_resources.find(id);
        return (itr != m_resources.end() ? &itr->second : nullptr);
    }

    bool Unload(const std::string& id)
    {
        auto itr = m_resources.find(id);
        if (itr == m_resources.end()) return false;

        m_resources.erase(itr); // Memory cleanup is automatic

        return true;
    }

    void LoadPaths(const std::string& pathFile)
    {
        std::ifstream paths{ Utils::GetWorkingDirectory() + pathFile };
        if (!paths.is_open())
        {
            EngineLog::Error("Failed loading the path file: " + pathFile);
            return;
        }

        auto warnLine = [&](unsigned int lineNumber, const std::string& message)
            {
                EngineLog::Warn(pathFile + " line " + std::to_string(lineNumber) + ": " + message);
            };

        std::unordered_map<std::string, unsigned int> firstSeenLineById;
        std::string line;
        unsigned int lineNumber = 0;

        while (std::getline(paths, line))
        {
            ++lineNumber;

            // Shared config-line preprocessing:
            // strips inline comments, trims whitespace, skips empty/custom comment lines
            if (!ParseUtils::PrepareConfigLine(line)) continue;

            std::stringstream keystream{ line };
            std::string pathName;
            std::string path;

            // Exact read: requires exactly 2 tokens, rejects trailing garbage
            if (!ParseUtils::TryReadExact(keystream, pathName, path))
            {
                warnLine(lineNumber, "Invalid entry (expected: <ResourceId> <RelativePath>).");
                continue;
            }

            if (pathName.empty() || path.empty())
            {
                warnLine(lineNumber, "Resource id/path cannot be empty.");
                continue;
            }

            auto [seenIt, insertedSeen] = firstSeenLineById.emplace(pathName, lineNumber);
            (void)insertedSeen;

            auto [it, inserted] = m_paths.emplace(pathName, path);
            if (!inserted)
            {
                warnLine(
                    lineNumber,
                    "Duplicate resource id '" + pathName +
                    "' (first seen at line " + std::to_string(seenIt->second) +
                    "). Overwriting previous path.");
                it->second = path;
            }
        }
    }

private:
    std::unordered_map<std::string, std::pair<std::unique_ptr<T>, unsigned int>> m_resources;
    std::unordered_map<std::string, std::string> m_paths;
    std::string m_pathsFile;
};