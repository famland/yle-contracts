#ifndef _YLE_UTILS_H_
#define _YLE_UTILS_H_

#include <string>
#include <vector>

#include "config.hpp"

std::vector<string> split(const string& str, const string& delim)
{
    std::vector<string> tokens;
    size_t prev = 0, pos = 0;
    do {
        pos = str.find(delim, prev);
        if (pos == string::npos)
            pos = str.length();
        string token = str.substr(prev, pos-prev);
        tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());

    return tokens;
}


int64_t yield_no_2_asset_id(const string& farmid)
{
    int64_t asset_id = atoll(farmid.c_str() + strlen(YieldConfig::yield_prefix));;
    return asset_id;
}

int64_t farm_no_2_asset_id(const string& farmid)
{
    int64_t asset_id = atoll(farmid.c_str() + strlen(YieldConfig::farm_prefix));;
    return asset_id;
}

int64_t ctree_no_2_asset_id(const string& ctree_id)
{
    int64_t asset_id = atoll(ctree_id.c_str() + strlen(YieldConfig::ctree_prefix));;
    return asset_id;
}

#endif // _YLE_UTILS_H_
