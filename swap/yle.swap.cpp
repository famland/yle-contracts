
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
#include "yle.core.hpp"

namespace eosio {

void yield_swap::init( const name& author)
{
    g_core_table.emplace(_self, [&](auto& _core) {
        _core.exchanger = author;
#if 0
        _core.usdt_balance = eosio::token::get_balance(
                    usdt_contract_account, liquidity_pool, YieldConfig::usdt_symbol.code());
        _core.ylt_balance = eosio::token::get_balance(
                    ylt_contract_account, liquidity_pool, YieldConfig::yl_symbol.code());
        _core.price = asset( _core.usdt_balance.amount / _core.ylt_balance.amount, YieldConfig::usdt_symbol);
#endif
    });
}

void yield_swap::on_transfer(const name& from, const name& to,
                             const asset &quantity, string memo)
{
    check( is_account ( from ), "user account does not exist" );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "quantity must be positive" );

    if ( from == liquidity_pool || to != liquidity_pool ) {
        return;
    }

    // TODO: check ( to == get_self(), "receiver invalid");
    //
    struct transfer_memo trans_memo;
    int rc = parse_memo(memo, trans_memo);
    check (rc == 0, "memo invalid");

	switch (trans_memo.trans_type) {
	case transfer_action_swap_usdt:
        exchange_usdt(from, quantity, trans_memo);
		break;
	case transfer_action_swap_ylt:
        exchange_ylt(from, quantity, trans_memo);
		break;

	default:
		break;
	}
}

// receive yl token
void yield_swap::exchange_usdt(const name& user, const asset& quantity, const transfer_memo& memo)
{
    check( quantity.symbol == YieldConfig::yl_symbol, "symbol check failed, need YL" );

    asset fee(quantity.amount * 0.003, YieldConfig::yl_symbol);
	transfer_fee(make_usdt_asset(0), fee);

	asset slippage = fee;
    pay_context ctx;
    asset required_usdt = convert(quantity - fee, memo.hash, ctx, slippage);
	check( required_usdt.amount > 0, " required_usdt should more than 0 ");

	transfer_fee(make_usdt_asset(0), slippage);

    auto balance = eosio::token::get_balance(
                        usdt_contract_account, liquidity_pool, YieldConfig::usdt_symbol.code());
    auto stable_pool_balance = eosio::token::get_balance(
                        usdt_contract_account, YieldConfig::fund_pool, YieldConfig::usdt_symbol.code());

    check( balance.amount >= required_usdt.amount, "overdrawn usdt balance" );

    name pool_owner = liquidity_pool;

    if (ctx.payer == YieldConfig::fund_pool && stable_pool_balance.amount >= required_usdt.amount) {
        pool_owner = YieldConfig::fund_pool;
    } else {
        check( balance.amount >= required_usdt.amount, "overdrawn usdt balance" );
    }

    token::transfer_action transfer_action( usdt_contract_account, { pool_owner, "active"_n } );
    transfer_action.send( pool_owner, user, required_usdt, "exchange USDT/YL" );

    //require_recipient( "ylecore.code"_n );
	// TODO: usdt quantity should be caculated from original quantity
	notify_core( user, required_usdt );
}

// receive usdt
void yield_swap::exchange_ylt(const name& user, const asset& quantity, const transfer_memo& memo)
{
    check( quantity.symbol == YieldConfig::usdt_symbol, "symbol check failed, need USDT" );

    asset fee(quantity.amount * 0.003, YieldConfig::usdt_symbol);
	transfer_fee(fee, make_ylt_asset(0));

	asset slippage = fee;
    pay_context ctx;
    asset required_yltoken = convert(quantity - fee, memo.hash, ctx, slippage);
	check( required_yltoken.amount > 0, " required_yltoken should more than 0 ");

	transfer_fee(slippage, make_ylt_asset(0));

    auto balance = eosio::token::get_balance(
                        ylt_contract_account, liquidity_pool, YieldConfig::yl_symbol.code());
    auto stable_pool_balance = eosio::token::get_balance(
                        ylt_contract_account, YieldConfig::fund_pool, YieldConfig::yl_symbol.code());

    name pool_owner = liquidity_pool;

    if (ctx.payer == YieldConfig::fund_pool && stable_pool_balance.amount >= required_yltoken.amount) {
        token::transfer_action transfer_action( ylt_contract_account, { YieldConfig::fund_pool, "active"_n } );
        transfer_action.send( YieldConfig::fund_pool, user, required_yltoken, "exchange YL/USDT" );

    } else {
        check( balance.amount >= required_yltoken.amount, "overdrawn yltoken balance" );
        token::transfer_action transfer_action( ylt_contract_account, { pool_owner, "active"_n } );
        transfer_action.send( pool_owner, user, required_yltoken, "exchange YL/USDT" );
    }

    //require_recipient( "ylecore.code"_n );
	notify_core( user, quantity );
}

asset yield_swap::convert( const asset& quantity, const string& hash, pay_context& ctx, asset& slippage )
{
    ctx.payer = liquidity_pool;

    auto usdt_balance = eosio::token::get_balance(
                        ylt_contract_account, liquidity_pool, YieldConfig::usdt_symbol.code());
    auto ylt_balance = eosio::token::get_balance(
                        ylt_contract_account, liquidity_pool, YieldConfig::yl_symbol.code());

    check ( usdt_balance.amount || ylt_balance.amount, "balance invalid" );

    if (ylt_balance.amount == 0) {
        ylt_balance.amount = usdt_balance.amount;
    }

	asset swap_quantity = quantity;

	double trade_slip = 0;
	if (swap_quantity.symbol == YieldConfig::usdt_symbol) {
		trade_slip = quantity.amount * 1.0 / (quantity.amount + usdt_balance.amount);
	} else {
		trade_slip = quantity.amount * 1.0 / (quantity.amount + ylt_balance.amount);
	}

	auto stable_price = yield_core::get_stable_price();
    auto yl_price_by_usdt = usdt_balance.amount * 1.0 / ylt_balance.amount;
	int deviation = (yl_price_by_usdt - 0.0001 * stable_price.amount) * 100;
	if ( (deviation >= 2 && swap_quantity.symbol == YieldConfig::usdt_symbol) ||
         (deviation <= -2 && swap_quantity.symbol == YieldConfig::yl_symbol) ) {
		double random_v = hash_to_random(hash) * 0.00001;
		if (random_v > 0.01) 
			random_v = 0.01;
		if (random_v < 0.001)
			random_v = 0.001;
		double slippage_value = random_v * pow(2, abs(deviation) - 2);
		if (slippage_value + trade_slip > 0.9) {
			slippage_value = 0.9 - trade_slip;
		}

		slippage.set_amount( quantity.amount * slippage_value );

		print_f("stable price = %, yl price = %, random_v = %, deviation = %, slippage = %, trade_slip = %\n",
				stable_price, yl_price_by_usdt, random_v, deviation, slippage_value, trade_slip);

		swap_quantity -= slippage;

        ctx.payer = YieldConfig::fund_pool;
	}

    print_f ("usdt: %, yl: %, yl price: %, slippage: %\n", usdt_balance, ylt_balance, yl_price_by_usdt, slippage);

    if (swap_quantity.symbol == YieldConfig::usdt_symbol) {
        auto yl_quantity = (ylt_balance * swap_quantity.amount) / (usdt_balance.amount + swap_quantity.amount);
        return yl_quantity;

    } else if (swap_quantity.symbol == YieldConfig::yl_symbol) {
        auto usdt_quantity = (usdt_balance * swap_quantity.amount) / (ylt_balance.amount - swap_quantity.amount);
        return usdt_quantity;
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
    yield_core::notify_action notify_action( YieldConfig::contract_core, { "ylecore.code"_n, "active"_n } );
    notify_action.send( user, quantity );
}

bool yield_swap::transfer_token( const name& from, const name& to, const asset& quantity)
{
    token::transfer_action transfer_action( YieldConfig::contract_token, { from, "active"_n } );
    transfer_action.send( from, to, quantity, "transfer YL TOKEN" );
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

} // namespace yle
