#pragma once
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include "Utilities.h"
#include "EngineLog.h"

template<typename Derived, typename T>
class ResourceManager
{
public:
    ResourceManager(const std::string& pathsFile)
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
        auto res = Find(id);
        if (res)
        {
            ++res->second;
            return true;
        }

        auto path = m_paths.find(id);
        if (path == m_paths.end())
            return false;

        // Using unique_ptr to handle safe and temporary allocation
        std::unique_ptr<T> resource = Load(path->second);
        if (!resource)
            return false;

        m_resources.emplace(id, std::make_pair(std::move(resource), 1));
        return true;
    }

    bool ReleaseResource(const std::string& id)
    {
        auto res = Find(id);
        if (!res) return false;

        --res->second;
        if (!res->second) Unload(id);

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

        std::string line;
        unsigned int lineNumber = 0;

        while (std::getline(paths, line))
        {
            ++lineNumber;

            // Skip empty lines and comment lines
            if (line.empty() || line[0] == '|') continue;

            std::stringstream keystream{ line };
            std::string pathName;
            std::string path;

            if (!(keystream >> pathName >> path))
            {
                EngineLog::Warn("Invalid path entry at line " + std::to_string(lineNumber) + " in " + pathFile);
                continue;
            }

            auto [it, inserted] = m_paths.emplace(pathName, path);
            if (!inserted)
            {
                EngineLog::Warn("Duplicate resource id '" + pathName + "' in " + pathFile + ". Overwriting previous value.");
                it->second = path;
            }
        }

        // paths.close(); Optional, not required (RAII already handles this)
    }


private:
    std::unordered_map<std::string, std::pair<std::unique_ptr<T>, unsigned int>> m_resources;
    std::unordered_map<std::string, std::string> m_paths;
};