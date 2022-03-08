#pragma once

#include <stdint.h>

#include <string>

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include "eosio.token/eosio.token.hpp"
#include "config.hpp"

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

using std::string;

class [[eosio::contract("yle.swap")]] yield_swap : public contract {
public:
    using contract::contract;

    [[eosio::action]]
    void init( const name& author);

    [[eosio::on_notify("eosio.token::transfer")]]
    void on_transfer(const name& from, const name& to,
                    const asset &quantity, string memo);

public:
    static asset get_ylt_price()
    {
        // price = USDT/YL
        auto usdt_balance = eosio::token::get_balance(
                            usdt_contract_account, liquidity_pool, YieldConfig::usdt_symbol.code());
        auto ylt_balance = eosio::token::get_balance(
                            ylt_contract_account, liquidity_pool, YieldConfig::yl_symbol.code());

        if (usdt_balance.amount == 0 || ylt_balance.amount == 0) {
            return USDT_ASSET(1);
        }

        auto yl_price_by_usdt = usdt_balance.amount * 1.0 / (ylt_balance.amount);
        return USDT_ASSET(yl_price_by_usdt);
    }

private:
    struct [[eosio::table]] core_s {
        name    exchanger;

        asset   price;

        asset   low;
        asset   high;

        asset   usdt_balance;
        asset   ylt_balance;

        uint64_t primary_key() const { return exchanger.value; }
    };

	typedef eosio::multi_index< "core"_n, core_s> core_t;
    core_t g_core_table = core_t(get_self(), get_self().value);

private:
    constexpr static name liquidity_pool = name("yle.liquid");
    constexpr static name usdt_contract_account = name("eosio.token");
    constexpr static name ylt_contract_account = name("eosio.token");

    // memo parse
    struct transfer_memo {
        enum transfer_action_type trans_type;
        string  hash;
    };

    struct pay_context {
        name    payer;
        asset   quantity;
        asset   slip;
    };

private:
    void exchange_usdt( const name& user, const asset& quantity, const transfer_memo& memo );
    void exchange_ylt( const name& user, const asset& quantity, const transfer_memo& memo);

    asset convert( const asset& quantity, const string& hash, pay_context& ctx, asset& slippage );

    int parse_memo(const string& memo, struct transfer_memo& trans_memo);
    int64_t hash_to_random(const string& hash);

	void notify_core( const name& user, const asset& quantity);
	bool transfer_token( const name& from, const name& to, const asset& quantity);
	void transfer_fee(const asset& usdt_quantity, const asset& ylt_quantity);
};

}
