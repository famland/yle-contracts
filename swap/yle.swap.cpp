
/********************************************************
 
       Filename: yle.swap.cpp
 
         Author: lker
    Description: ---
         Create: 2021-07-25 13:30:08
  Last Modified: 2021-07-25 13:30:08
 
  Copyright (c) [2021-2021] YLE.com, Inc. All Rights Reserved

********************************************************/

#include "yle.swap.hpp"

#include "config.hpp"
#include "utils.hpp"
#include "yle.asset.hpp"

// #define FOR_AUDIT

#ifndef FOR_AUDIT
#include "yle.core.hpp"
#endif

namespace eosio {

void yield_swap::init( const name& author)
{
    g_core_table.emplace(_self, [&](auto& _core) {
        _core.exchanger = author;
    });
}

void yield_swap::xperiod(/*const name & author*/)
{

    auto usdt_balance = eosio::token::get_balance(
                        YieldConfig::usdt_contract_account, YieldConfig::fund_pool, YieldConfig::usdt_symbol.code());
    auto ylt_balance = eosio::token::get_balance(
                        YieldConfig::ylt_contract_account, YieldConfig::fund_pool, YieldConfig::yl_symbol.code());

    // auto iter = g_core_table.require_find(author.value, "liquid stat not found");
    auto iter = g_core_table.begin();
    g_core_table.modify(iter, same_payer, [&](auto &_stat) {
        _stat.usdt_balance = usdt_balance;
        _stat.ylt_balance = ylt_balance;

        _stat.low = make_usdt_asset(0);
        _stat.high = make_ylt_asset(0);
    });
}

#if 0
void yield_swap::update_stat(const asset& stable, bool is_usdt)
{
    if (stable.amount <= 0) {
        return;
    }

    auto iter = g_core_table.begin();

    g_core_table.modify(iter, same_payer, [&](auto &_stat) {
        if (is_usdt) {
            _stat.low += stable;
            check(_stat.usdt_balance.amount * 0.2 < _stat.low.amount, "stable used more then 20%");
        } else {
            _stat.high += stable;
            check(_stat.ylt_balance.amount * 0.2 < _stat.low.amount, "stable used more then 20%");
        }
    });
}
#endif

bool yield_swap::check_stat(const asset& stable, bool is_usdt)
{
    if (stable.amount <= 0) {
        return true;
    }

    auto iter = g_core_table.begin();
    if (is_usdt) {
        if (iter->usdt_balance.amount * 0.2 < iter->low.amount) {
            return false;
        }
    } else {
        if (iter->ylt_balance.amount * 0.2 < iter->low.amount) {
            return false;
        }
    }

    return true;
}

void yield_swap::on_transfer(const name& from, const name& to,
                             const asset &quantity, string memo)
{
	if (from == YieldConfig::liquidity_pool || to != get_self()) {
        return;
    }

    check( is_account ( from ), "user account does not exist" );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "quantity must be positive" );

    check((quantity.symbol == YieldConfig::usdt_symbol) || (quantity.symbol == YieldConfig::yl_symbol),
            "invalid token contract");

    struct transfer_memo trans_memo;
    int rc = parse_memo(memo, trans_memo);
    check (rc == 0, "parse memo failed");

	switch (trans_memo.trans_type) {
	case transfer_action_swap_usdt:
        exchange_usdt(from, quantity, trans_memo);
		break;
	case transfer_action_swap_ylt:
        exchange_ylt(from, quantity, trans_memo);
		break;
	case transfer_action_deposit:
		break;

	default:
        check(false, "invalid memo command type: " + memo);
		break;
	}
}

// receive yl token
void yield_swap::exchange_usdt(const name& user, const asset& quantity, const transfer_memo& memo)
{
    check(get_first_receiver() == YieldConfig::ylt_contract_account, "invalid YL token contract");

    check( quantity.symbol == YieldConfig::yl_symbol, "symbol check failed, need YL" );

    asset fee(quantity.amount * 0.003, YieldConfig::yl_symbol);

    pay_context ctx;
    asset required_usdt = convert(quantity, fee, memo.hash, ctx);
	check( required_usdt.amount > 0, " required_usdt should more than 0 ");

    auto balance = eosio::token::get_balance(
                        YieldConfig::usdt_contract_account, YieldConfig::liquidity_pool, YieldConfig::usdt_symbol.code());
    auto stable_pool_balance = eosio::token::get_balance(
                        YieldConfig::usdt_contract_account, YieldConfig::fund_pool, YieldConfig::usdt_symbol.code());

    check( balance.amount >= required_usdt.amount, "overdrawn usdt balance" );

	transfer_fee(make_usdt_asset(0), fee);
    transfer_usdt_fee(user, required_usdt, ctx);
#if 0
	transfer_fee(ctx.slip, make_ylt_asset(0));

    name pool_owner = YieldConfig::liquidity_pool;
    if (ctx.payer == YieldConfig::fund_pool && check_stat(required_usdt, true)) {
        double ratio = 0.0001 * pow(2, ctx.deviation);
        auto stable_pay_limit = make_usdt_asset(0);

        if (stable_pool_balance.amount > 10000) {
            pool_owner = YieldConfig::fund_pool;
            stable_pay_limit.set_amount(stable_pool_balance.amount * ratio);

            if (stable_pay_limit < required_usdt) {
                token::transfer_action transfer_action( YieldConfig::usdt_contract_account, { pool_owner, "active"_n } );
                transfer_action.send( pool_owner, user, stable_pay_limit, "exchange USDT/YL payment from stable pool" );

                pool_owner = YieldConfig::liquidity_pool;
                required_usdt -= stable_pay_limit;
                check( balance.amount >= required_usdt.amount, "overdrawn usdt balance from swap" );
            }
        }
    } else {
        check( balance.amount >= required_usdt.amount, "overdrawn usdt balance from swap" );
    }

    token::transfer_action transfer_action( YieldConfig::usdt_contract_account, { pool_owner, "active"_n } );
    transfer_action.send( pool_owner, user, required_usdt, "exchange USDT/YL" );
#endif
}

// receive usdt
void yield_swap::exchange_ylt(const name& user, const asset& quantity, const transfer_memo& memo)
{
    check(get_first_receiver() == YieldConfig::usdt_contract_account, "invalid USDT token contract");
    check( quantity.symbol == YieldConfig::usdt_symbol, "symbol check failed, need USDT" );

    asset fee(quantity.amount * 0.003, YieldConfig::usdt_symbol);

    pay_context ctx;
    asset required_yltoken = convert(quantity, fee, memo.hash, ctx);
	check( required_yltoken.amount > 0, " required_yltoken should more than 0 ");

    auto balance = eosio::token::get_balance(
                        YieldConfig::ylt_contract_account, YieldConfig::liquidity_pool, YieldConfig::yl_symbol.code());
    auto stable_pool_balance = eosio::token::get_balance(
                        YieldConfig::ylt_contract_account, YieldConfig::fund_pool, YieldConfig::yl_symbol.code());

    // check(false, "after balance before transfer");

	transfer_fee(fee, make_ylt_asset(0));
    transfer_ylt_fee(user, required_yltoken, ctx);

#if 0
	transfer_fee(make_usdt_asset(0), ctx.slip);

    name pool_owner = YieldConfig::liquidity_pool;

    if (ctx.payer == YieldConfig::fund_pool && stable_pool_balance.amount >= required_yltoken.amount) {
        token::transfer_action transfer_action( YieldConfig::ylt_contract_account, { YieldConfig::fund_pool, "active"_n } );
        transfer_action.send( YieldConfig::fund_pool, user, required_yltoken, "exchange YL/USDT" );

    } else {
        check( balance.amount >= required_yltoken.amount, "overdrawn yltoken balance" );
        token::transfer_action transfer_action( YieldConfig::ylt_contract_account, { pool_owner, "active"_n } );
        transfer_action.send( pool_owner, user, required_yltoken, "exchange YL/USDT" );
    }

    name pool_owner = YieldConfig::liquidity_pool;
    if (ctx.payer == YieldConfig::fund_pool && check_stat(required_yltoken, false)) {
        double ratio = 0.0001 * pow(2, ctx.deviation);
        auto stable_pay_limit = make_ylt_asset(0);

        if (stable_pool_balance.amount > 10000) {
            pool_owner = YieldConfig::fund_pool;
            stable_pay_limit.set_amount(stable_pool_balance.amount * ratio);

            if (stable_pay_limit < required_yltoken) {
                token::transfer_action transfer_action( YieldConfig::ylt_contract_account, { pool_owner, "active"_n } );
                transfer_action.send( pool_owner, user, stable_pay_limit, "exchange USDT/YL payment from stable pool" );

                pool_owner = YieldConfig::liquidity_pool;
                required_yltoken -= stable_pay_limit;
                check( balance.amount >= required_yltoken.amount, "overdrawn ylt balance from swap" );
            }
        }
    } else {
        check( balance.amount >= required_yltoken.amount, "overdrawn ylt balance from swap" );
    }

    token::transfer_action transfer_action( YieldConfig::ylt_contract_account, { pool_owner, "active"_n } );
    transfer_action.send( pool_owner, user, required_yltoken, "exchange YL/USDT" );

    //require_recipient( "ylecore.code"_n );
#endif
	notify_core( user, quantity );
}

asset yield_swap::convert( const asset& quantity, const asset& fee, const string& hash, pay_context& ctx )
{
    ctx.payer = YieldConfig::liquidity_pool;

    auto usdt_balance = eosio::token::get_balance(
                        YieldConfig::usdt_contract_account, YieldConfig::liquidity_pool, YieldConfig::usdt_symbol.code());
    auto ylt_balance = eosio::token::get_balance(
                        YieldConfig::ylt_contract_account, YieldConfig::liquidity_pool, YieldConfig::yl_symbol.code());

    check ( usdt_balance.amount || ylt_balance.amount, "balance invalid" );

    if (ylt_balance.amount == 0) {
        ylt_balance.amount = usdt_balance.amount;
    }

	asset swap_quantity = quantity - fee;

	double trade_slip = 0;
    double yl_price_by_usdt;    // current realtime price
    double out;
	if (swap_quantity.symbol == YieldConfig::usdt_symbol) {
        ctx.slip = make_ylt_asset(0);

        auto usdt_c = usdt_balance - quantity;
        yl_price_by_usdt = usdt_c.amount * 1.0 / ylt_balance.amount;

        out = ylt_balance.amount - (usdt_c.amount * ylt_balance.amount) * 1.0 / (usdt_c + swap_quantity).amount;
        auto price1 = usdt_balance.amount / (ylt_balance.amount - out);
        trade_slip = fabs(price1 / yl_price_by_usdt - 1);
	} else {
        ctx.slip = make_usdt_asset(0);

        auto ylt_c = ylt_balance - quantity;
        yl_price_by_usdt = usdt_balance.amount * 1.0 / ylt_c.amount;

        out = usdt_balance.amount - (usdt_balance.amount * ylt_c.amount) * 1.0 / (ylt_c + swap_quantity).amount;
        auto price1 = (usdt_balance.amount - out) / (ylt_balance.amount);
        trade_slip = fabs(price1 / yl_price_by_usdt - 1);
	}

    check(trade_slip < 0.9, "trade slippage too high, terminate.");

#ifdef FOR_AUDIT
	auto stable_price = make_usdt_asset(1);
#else
	auto stable_price = yield_core::get_stable_price();
#endif
    double stable_d = stable_price.amount * 0.0001;
	double deviation = ((yl_price_by_usdt - stable_d) / stable_d) * 100;
    // double deviation_thres = stable_d * 0.02;
    double deviation_thres = 0.2;

	if ( (deviation >= deviation_thres && swap_quantity.symbol == YieldConfig::usdt_symbol) ||
         (deviation <= -deviation_thres && swap_quantity.symbol == YieldConfig::yl_symbol) ) {

        check (abs(deviation) <= 10, "price deviation too high, terminate");

        ctx.deviation = abs(deviation);

		double random_v = hash_to_random(hash) * 0.00001;
		if (random_v > 0.01) 
			random_v = 0.01;
		if (random_v < 0.001)
			random_v = 0.001;
		double slippage_value = random_v * pow(2, abs(deviation));
		if (slippage_value + trade_slip > 0.9) {
			slippage_value = 0.9 - trade_slip;
		}

		print_f("stable price = %, yl price = %, random_v = %, deviation = %, slippage = %, trade_slip = %\n",
				stable_price, yl_price_by_usdt, random_v, deviation, slippage_value, trade_slip);

        ctx.payer = YieldConfig::fund_pool;
        ctx.slip.set_amount( out * slippage_value );

        out -= (out * slippage_value);
	}

    print_f ("usdt: %, yl: %, yl price: %\n", usdt_balance, ylt_balance, yl_price_by_usdt);

    if (swap_quantity.symbol == YieldConfig::usdt_symbol) {
        return make_ylt_asset(out * 0.0001);

    } else if (swap_quantity.symbol == YieldConfig::yl_symbol) {
        return make_usdt_asset(out * 0.0001);
    } else {
        check ( false, "invalid token symbol for yle.swap" );
    }

    return USDT_ASSET(1);
}

int yield_swap::parse_memo(const string& memo, struct transfer_memo& trans_memo)
{
    auto parts = split(memo, ",");

    if (parts[0] == "deposit") {
		trans_memo.trans_type = transfer_action_deposit;
	} else if (parts[0] == "swap_usdt") {
		trans_memo.trans_type = transfer_action_swap_usdt;
        trans_memo.hash = parts[1];
	} else if (parts[0] == "swap_ylt") {
		trans_memo.trans_type = transfer_action_swap_ylt;
        trans_memo.hash = parts[1];
    } else {
        trans_memo.trans_type = transfer_action_invalid;
    }

    return 0;
}

int64_t yield_swap::hash_to_random(const string& hash)
{
	char *endptr;
    char string[7] = {0};
    strncpy(string, hash.c_str(), 3);

    int64_t value = strtoll(string, &endptr, 16);
	if ((errno == ERANGE && (value == LONG_MAX || value == LONG_MIN))
		   || (errno != 0 && value == 0)) {
		return -1;
	}

	if (endptr == string) {
		return -1;
	}

    return value;
}

void yield_swap::notify_core( const name& user, const asset& quantity) 
{
#ifndef FOR_AUDIT
    yield_core::notify_action notify_action( YieldConfig::contract_core, { YieldConfig::contract_core, "active"_n } );
    notify_action.send( user, quantity );
#endif
}

bool yield_swap::transfer_token( const name& from, const name& to, const asset& quantity)
{
    if (quantity.amount <= 0) {
        return false;
    }

    string memo = "liquidity transfer token";

    if (quantity.symbol == YieldConfig::yl_symbol) {
        token::transfer_action transfer_action( YieldConfig::ylt_contract_account, { from, "active"_n } );
        transfer_action.send( from, to, quantity, memo );
    } else if (quantity.symbol == YieldConfig::usdt_symbol) {
        token::transfer_action transfer_action( YieldConfig::usdt_contract_account, { from, "active"_n } );
        transfer_action.send( from, to, quantity, memo );
    }

    return true;
}

void yield_swap::transfer_fee(const asset& usdt_quantity, const asset& ylt_quantity)
{
	const name& from = YieldConfig::liquidity_pool;

	if (usdt_quantity.amount > 0) {
		asset fund_usdt = USDT_ASSET2(usdt_quantity.amount * 0.8);
		transfer_token( from, YieldConfig::fund_pool, fund_usdt);
		transfer_token( from, YieldConfig::award_pool, usdt_quantity - fund_usdt);
	}

	if (ylt_quantity.amount > 0) {
		asset fund_ylt	= YLT_ASSET2(ylt_quantity.amount * 0.8);
		transfer_token( from, YieldConfig::fund_pool, fund_ylt);
		transfer_token( from, YieldConfig::award_pool, ylt_quantity - fund_ylt);
	}
}

void yield_swap::normal_pay(const name &user, const asset& quantity, pay_context& ctx)
{
    auto pool_pay = YieldConfig::liquidity_pool;

    if ((quantity.symbol == YieldConfig::usdt_symbol)) {
        auto liquid_balance = eosio::token::get_balance(
                            YieldConfig::usdt_contract_account, YieldConfig::liquidity_pool, YieldConfig::usdt_symbol.code());
        check( liquid_balance.amount >= quantity.amount, "overdrawn usdt balance from swap" );

        transfer_fee(ctx.slip, make_ylt_asset(0));
        transfer_token(pool_pay, user, quantity);
    } else if (quantity.symbol == YieldConfig::yl_symbol) {
        auto liquid_balance = eosio::token::get_balance(
                            YieldConfig::ylt_contract_account, YieldConfig::liquidity_pool, YieldConfig::yl_symbol.code());
        check( liquid_balance.amount >= quantity.amount, "overdrawn usdt balance from swap" );

        transfer_fee(make_usdt_asset(0), ctx.slip);
        transfer_token(pool_pay, user, quantity);
    } else {
        check(false, "invalid symbol");
    }
}

void yield_swap::transfer_usdt_fee(const name &user, const asset& usdt_quantity, pay_context& ctx)
{
    auto quantity = usdt_quantity;
    auto stable_balance = eosio::token::get_balance(
                        YieldConfig::usdt_contract_account, YieldConfig::fund_pool, YieldConfig::usdt_symbol.code());

    auto pool_pay = YieldConfig::liquidity_pool;
    if (ctx.payer == pool_pay || (false == check_stat(quantity, true)) || stable_balance.amount < 10000) {
        normal_pay(user, quantity, ctx);
        return;
    }

    double ratio = 0.0001 * pow(2, ctx.deviation);
    auto stable_pay_limit = make_usdt_asset(0);

    pool_pay = YieldConfig::fund_pool;
    stable_pay_limit.set_amount(stable_balance.amount * ratio);

    if (stable_pay_limit < ctx.slip) {
        ctx.slip -= stable_pay_limit;
    } else {
        stable_pay_limit -= ctx.slip;

        if (stable_pay_limit < quantity) {
            transfer_token(YieldConfig::fund_pool, user, stable_pay_limit);
            quantity -= stable_pay_limit;
        } else {
            transfer_token(YieldConfig::fund_pool, user, quantity);
            return;
        }
    }

    // stable_pay_limit = 0
    normal_pay(user, quantity, ctx);
}

void yield_swap::transfer_ylt_fee(const name& user, const asset& ylt_quantity, pay_context& ctx)
{
    auto quantity = ylt_quantity;

    auto liquid_balance = eosio::token::get_balance(
                        YieldConfig::ylt_contract_account, YieldConfig::liquidity_pool, YieldConfig::yl_symbol.code());
    auto stable_balance = eosio::token::get_balance(
                        YieldConfig::ylt_contract_account, YieldConfig::fund_pool, YieldConfig::yl_symbol.code());

    name pool_pay = YieldConfig::liquidity_pool;
    if (ctx.payer == pool_pay || (false == check_stat(quantity, false)) || stable_balance.amount < 10000) {
        normal_pay(user, quantity, ctx);
        return;
    }

    double ratio = 0.0001 * pow(2, ctx.deviation);
    auto stable_pay_limit = make_ylt_asset(0);

    pool_pay = YieldConfig::fund_pool;
    stable_pay_limit.set_amount(stable_balance.amount * ratio);

    if (stable_pay_limit < ctx.slip) {
        ctx.slip -= stable_pay_limit;
    } else {
        stable_pay_limit -= ctx.slip;

        if (stable_pay_limit < quantity) {
            transfer_token(YieldConfig::fund_pool, user, stable_pay_limit);
            quantity -= stable_pay_limit;
        } else {
            transfer_token(YieldConfig::fund_pool, user, quantity);
            return;
        }
    }

    // stable_pay_limit = 0
    normal_pay(user, quantity, ctx);
}

} // namespace yle
