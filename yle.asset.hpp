#ifndef YLE_CONTRACT_YLE_ASSET_H_
#define YLE_CONTRACT_YLE_ASSET_H_

#include <stdint.h>

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include "config.hpp"

asset make_ylt_asset(double amount)
{
	//check ( amount > 0, "[make ylt asset] asset range failed" );
	return asset((uint64_t)(amount * 10000), YieldConfig::yl_symbol);
}

asset make_lptoken_asset(double amount)
{
	//check ( amount > 0, "[make lptoken asset] asset range failed" );
	return asset((uint64_t)(amount * 10000), YieldConfig::lptoken_symbol);
}

asset make_usdt_asset(double amount)
{
	//check ( amount > 0, "[make usdt asset] asset range failed" );
	return asset((uint64_t)(amount * 10000), YieldConfig::usdt_symbol);
}

#endif // YLE_CONTRACT_YLE_ASSET_H_
