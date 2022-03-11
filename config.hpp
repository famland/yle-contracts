#ifndef YLE_CONTRACT_CONFIG_H_
#define YLE_CONTRACT_CONFIG_H_

#include <stdint.h>
#include <string>

#include <eosio/eosio.hpp> 
#include <eosio/asset.hpp>
#include <eosio/name.hpp>


using namespace std;
using namespace eosio;

struct YieldConfig {
	constexpr static symbol yl_symbol   = symbol("YL", 4);
	constexpr static symbol yls_symbol  = symbol("YLS", 4);
	constexpr static symbol usdt_symbol = symbol("USDT", 4);
	constexpr static symbol lptoken_symbol = symbol("YLULP", 4);

    constexpr static name yl_issuer     = name("yle.core");     // YL Token 发行账户
	constexpr static name fund_pool     = name("yle.stable1");
	constexpr static name award_pool    = name("yle.stable2");
	constexpr static name retain_pool   = name("yle.retain");	// 蓄水池
	constexpr static name liquidity_pool= name("yle.liquid");	// 流动池
	constexpr static name dividend_pool = name("yle.dividend");	// 分发池
    constexpr static name present_pool  = name("yle.present");  // 赠送资金池

    // bonus pool
    constexpr static name regular_bonus_pool    = name("regbonus.yle");
    constexpr static name random_bonus_pool     = name("rndbonus.yle");
    constexpr static name share_bonus_pool      = name("srebonus.yle");

	constexpr static name contract_farm = name("yle.shared");
	constexpr static name contract_token= name("eosio.token");
	constexpr static name contract_core = name("ylecore.code");

    constexpr static name usdt_contract_account = name("eosio.token");
    constexpr static name ylt_contract_account  = name("eosio.token");
    constexpr static name yls_contract_account  = name("eosio.token");
    constexpr static name lptoken_contract_account = name("eosio.token");


    constexpr static bool raw_liquidity_switch = true;

    constexpr static uint64_t ylt_initial_issue = 1500000;
    constexpr static uint64_t raw_initial_quota = 500;
    constexpr static uint64_t wild_initial_quota = 10000;
    constexpr static uint64_t yls_max_issue     = 1000000;

    constexpr static double issue_once  = 693.5;

    constexpr static double trade_fee   = 0.003;

    constexpr static char* yield_prefix = "YieldFarm#";
    constexpr static char* farm_prefix = "FamLand#";
    constexpr static char* ctree_prefix = "CryptoTree#";
};

enum transfer_action_type {
    transfer_action_deposit = 0,        // 做市

    transfer_action_swap,
    transfer_action_swap_ylt,           // swap ylt by usdt
    transfer_action_swap_usdt,          // swap usdt by ylt

    transfer_action_put_farm_order,     // 农田出售挂单(支付手续费)
    transfer_action_buy_farm,           // 购买农田(交易价+手续费）
    transfer_action_put_ctree_order,    // 加密树出售挂单（支付手续费）
    transfer_action_buy_ctree,          // 购买加密树（交易价格+手续费）

    transfer_action_acution,

    transfer_action_create_yield_farm,  // 创建共享农场
    transfer_action_batch_create,       // 批量创建农田(自动加入共享农场)
    transfer_action_invite,             // 共享农场邀约
    transfer_action_join_by_private_ad, // 通过私人邀约加入共享农场
    transfer_action_exit_yield_farm,

    transfer_action_invalid = -1,
};

struct action_param_create_yield_farm {
};

struct action_param_batch_create {
    uint32_t    count;
    uint64_t    yield_no;
};

struct action_param_invite {
    uint64_t    yield_no;
    uint32_t    ad_time;
};

struct action_param_exit {
    uint64_t    yield_no;
    uint64_t    land_id;
};

struct action_param_private_ad_join {
    uint64_t    land_id;
    uint64_t    yield_no;
    char        private_ad[16];
    const char* lands[16];
    int         lands_cnt;
};

// memo parse
struct transfer_memo {
    enum    transfer_action_type trans_type;
    string  farm_id;
    string  ctree_id;
    string  hash;
    int     land_type;
    double  price;

    union {
        struct action_param_batch_create batch_create;
        struct action_param_invite invite;
        struct action_param_private_ad_join private_ad_join;
        struct action_param_exit exit;
    } u;

    string  opaque;
};

#define USDT_ASSET(x)   asset((x * 10000), YieldConfig::usdt_symbol)
#define YLT_ASSET(x)    asset((x * 10000), YieldConfig::yl_symbol)
#define YLS_ASSET(x)    asset((x * 10000), YieldConfig::yls_symbol)

#define USDT_ASSET2(x)   asset((x) , YieldConfig::usdt_symbol)
#define YLT_ASSET2(x)    asset((x) , YieldConfig::yl_symbol)
#define YLS_ASSET2(x)    asset((x) , YieldConfig::yls_symbol)

#define EPOCH1_DURATION 3  // hours
#define EPOCH2_DURATION 24 // hours
#define SECS_PER_YEAR   (365*24*60*60) 

#endif // YLE_CONTRACT_CONFIG_H_
