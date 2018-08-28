/** FioName Token implementation
 *  @author Adam Androulidakis
 *  @file fioname.cpp
 *  @copyright defined in eos/LICENSE.txt
 */
#include "fioname.hpp"

namespace eosio {
    using std::string;
    using eosio::asset;

    // @abi action
    void fioname::create( account_name issuer, string fioName ) {
        require_auth( _self );

	// Check if issuer account exists 
	eosio_assert( is_account( issuer ), "issuer account does not exist");

        // Valid symbol
        asset supply(0, string_to_symbol(0, fioName.c_str()));

        auto symbol = supply.symbol;
        eosio_assert( symbol.is_valid(), "invalid fioname" );
        eosio_assert( supply.is_valid(), "invalid supply");

        // Check if currency with symbol already exists
        currency_index currency_table( _self, symbol.name() );
        auto existing_currency = currency_table.find( symbol.name() );
        eosio_assert( existing_currency == currency_table.end(), "token with this name already exists" );

        // Create new currency
        currency_table.emplace( _self, [&]( auto& currency ) {
           currency.supply = supply;
           currency.issuer = issuer;
        });
    } // fioname::create

    // @abi action
    void fioname::issue( account_name to,
                     asset quantity,
                     vector<string> uris,
		     string name,
                     string memo)
    {

	    eosio_assert( is_account( to ), "to account does not exist");

        // e,g, Get EOS from 3 EOS
        symbol_type symbol = quantity.symbol;
        eosio_assert( symbol.is_valid(), "invalid fioname" );
        
        eosio_assert( symbol.precision() == 0, "quantity must be a whole number" );
        eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

	    eosio_assert( name.size() <= 32, "name has more than 32 bytes" );
    	eosio_assert( name.size() > 0, "name is empty" );

        // Ensure currency has been created
        auto symbol_name = symbol.name();
        currency_index currency_table( _self, symbol_name );
        
        auto existing_currency = currency_table.find( symbol_name );
        eosio_assert( existing_currency != currency_table.end(), "token with fioname does not exist, create token before issue" );
        const auto& st = *existing_currency;

        // Ensure have issuer authorization and valid quantity
        require_auth( st.issuer );
        eosio_assert( quantity.is_valid(), "invalid quantity" );
        eosio_assert( quantity.amount > 0, "must issue positive quantity of FIO name tokens" );
        eosio_assert( symbol == st.supply.symbol, "symbol precision mismatch" );


        // Increase supply
	    add_supply( quantity );

        // Check that number of tokens matches uri size
        eosio_assert( quantity.amount == uris.size(), "mismatch between number of tokens and uris provided" );

        // Mint nfts
        for(auto const& uri: uris) {
            mint( to, st.issuer, asset{1, symbol}, uri, name);
        }

        // Add balance to account
        add_balance( to, quantity, st.issuer );
    } //fioname::issue

       // @abi action
       void fioname::transfer( account_name from,
                        account_name to,
                        id_type      id,
                        string       memo )
    {
        // Ensure authorized to send from account
        eosio_assert( from != to, "cannot transfer to self" );
        require_auth( from );

        // Ensure 'to' account exists
        eosio_assert( is_account( to ), "to account does not exist");

	    // Check memo size and print
        eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );
	    
        // Ensure token ID exists
        auto sender_token = tokens.find( id );
        eosio_assert( sender_token != tokens.end(), "token with specified ID does not exist" );
        
    	// Ensure owner owns token
        eosio_assert( sender_token->owner == from, "sender does not own token with specified ID");
       	

	    const auto& st = *sender_token;

	
    	// Notify both recipients
        require_recipient( from );
        require_recipient( to );

        // Transfer NFT from sender to receiver
        tokens.modify( st, from, [&]( auto& token ) {
	        token.owner = to;
        });

        // Change balance of both accounts
        sub_balance( from, st.value );
        add_balance( to, st.value, from );
    }

    void fioname::mint( account_name owner,
                    account_name ram_payer,
                    asset value,
                    string uri,
					string name)
    {
        // Add token with creator paying for RAM
        tokens.emplace( ram_payer, [&]( auto& token ) {
            token.id = tokens.available_primary_key();
            token.uri = uri;
            token.owner = owner;
            token.value = value;
			token.name = name;
			token.ownerPubKey = "EOS8RoQ2EdaPB862W5H94yGP4Y16NAAeuhqhdtHY71mL3SsbfAEUQ";
			token.domain = "Dapix";
        });
    }

    // @abi action
    void fioname::setrampayer(account_name payer, id_type id)
    {
	require_auth(payer);
	
	// Ensure token ID exists
	auto payer_token = tokens.find( id );
	eosio_assert( payer_token != tokens.end(), "token with specified ID does not exist" );
	
	// Ensure payer owns token
	eosio_assert( payer_token->owner == payer, "payer does not own token with specified ID");
	
	const auto& st = *payer_token;	
	
	// Notify payer
	require_recipient( payer );
	
	// Set owner as a RAM payer 
	/*tokens.modify( st, payer, [&]( auto& token ) {
		token.owner = 0;		
	});

	tokens.modify( st, payer, [&]( auto& token ) {
		token.owner = payer;
	});*/

	tokens.erase(payer_token);
	tokens.emplace(payer, [&](auto& token){
		token.id = st.id;
		token.uri = st.uri;
		token.owner = st.owner;
		token.value = st.value;
		token.name = st.name;
	});
	

	sub_balance( payer, st.value );
	add_balance( payer, st.value, payer );
    }

   
    // @abi action
    void fioname::cleartokens()
    {
	 require_auth(_self);

	 vector<uint64_t> keys;
	 for(auto it=tokens.begin(); it != tokens.end(); ++it){
	 
		keys.push_back(it->id);
	 }

	 for(auto& key : keys) {
	 
	 	auto it = tokens.find(key);
		if( it != tokens.end()) {
		
			tokens.erase(it);
		}
	 }
    }

    // @abi action
    void fioname::clearsymbol(asset value)
    {
	require_auth(_self);

	currency_index cur(_self, value.symbol.name());
	auto it2 = cur.find(value.symbol.name());
	if(it2 != cur.end()) {
        	cur.erase(it2);
        }
    }

    // @abi action
    void fioname::clearbalance(account_name owner, asset value)
    {	    
    	require_auth(_self);

		account_index acc(_self, owner);
		auto it1 = acc.find(value.symbol.name());
		if(it1 != acc.end()) 
		{
			acc.erase(it1);
		}
    }

    void fioname::sub_balance( account_name owner, asset value ) {
        account_index from_acnts( _self, owner );

        const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
        eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );


        if( from.balance.amount == value.amount ) {
            from_acnts.erase( from );
        } else {
            from_acnts.modify( from, owner, [&]( auto& a ) {
                a.balance -= value;
            });
        }
    }

    void fioname::add_balance( account_name owner, asset value, account_name ram_payer )
    {
        account_index to_accounts( _self, owner );
        auto to = to_accounts.find( value.symbol.name() );
        if( to == to_accounts.end() ) {
            to_accounts.emplace( ram_payer, [&]( auto& a ){
                a.balance = value;
            });
        } else {
            to_accounts.modify( to, 0, [&]( auto& a ) {
                a.balance += value;
            });
        }
    }

    void fioname::sub_supply( asset quantity ) {
        auto symbol_name = quantity.symbol.name();
        currency_index currency_table( _self, symbol_name );
        auto current_currency = currency_table.find( symbol_name );

        currency_table.modify( current_currency, 0, [&]( auto& currency ) {
            currency.supply -= quantity;
        });
    }

    void fioname::add_supply( asset quantity )
    {
        auto symbol_name = quantity.symbol.name();
        currency_index currency_table( _self, symbol_name );
        auto current_currency = currency_table.find( symbol_name );

        currency_table.modify( current_currency, 0, [&]( auto& currency ) {
            currency.supply += quantity;
        });
    }
	

EOSIO_ABI( fioname, (create)(issue)(transfer)(setrampayer)(cleartokens)(clearsymbol)(clearbalance)  )

} /// namespace eosio

