#pragma once

#include <string>
#include <vector>
#include <Common/Common.hpp>

class CSkinDatabase;

class SkinApiClient {
public:
    auto SetEndpoint(const std::string& url) -> void { m_Endpoint = url; }
    auto FetchAndUpdate(CSkinDatabase* db) -> bool;
    auto LoadCache(std::string& outJson) -> bool;
    auto SaveCache(const std::string& json) -> bool;

private:
    auto FetchJson(std::string& outJson) -> bool;
    auto ParseIntoDatabase(const std::string& json, CSkinDatabase* db) -> bool;

private:
    std::string m_Endpoint;
};