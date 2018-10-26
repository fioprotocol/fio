/** Common file implementation for Fio Javascript SDK
 *  Description: FioFinance common file
 *  @author Ciju John
 *  @file fio.common.js
 *  @copyright Dapix
 *
 *  Changes: 10-8-2018 Adam Androulidakis
 */


// General Configuration parameters
class Config {
}
Config.MaxAccountCreationAttempts=3;
Config.EosUrl =                     'http://localhost:8888';
Config.SystemAccount =              "fio.system";
Config.SystemAccountKey =           "5KBX1dwHME4VyuUss2sYM25D5ZTDvyYrbEz37UJqwAVAsR4tGuY"; // ${Config.SystemAccount}
Config.NewAccountBuyRamQuantity=    "100.0000 FIO";
Config.NewAccountStakeNetQuantity=  "100.0000 FIO";
Config.NewAccountStakeCpuQuantity=  "100.0000 FIO";
Config.NewAccountTransfer=false;
Config.TestAccount="fioname11111";
Config.FioFinanceAccount="fio.finance";
Config.LogLevel=3; // Log levels 0 - 5 with increasing verbosity.
Config.FinalizationTime=20;     // time in milliseconds to transaction finalization

// Helper static functions
class Helper {
    static typeOf( obj ) {
				
		return  ({}).toString.call( obj ).match(/\s(\w+)/)[1].toLowerCase();
		
    }

    static checkTypes( args, types ) {
        args = [].slice.call( args );
        for ( var i = 0; i < types.length; ++i ) {
            if ( Helper.typeOf( args[i] ) != types[i] )
                throw new TypeError( 'param '+ i +' must be of type '+ types[i]+ ', found type '+ Helper.typeOf( args[i] ) );
				
			if (args[i] === "") {
				throw new Error('Null or empty parameter' + types[i]);
            }
        }
    }


    static randomString(length, chars) {
        var result = '';
        for (var i = length; i > 0; --i) result += chars[Math.floor(Math.random() * chars.length)];
        return result;
    }
	
	
   	// Get account details
    // Returns tuple [status, eos response]
    async getAccount(accountName) {
        checkTypes( arguments, ['string',] );

        const Url=fiocommon.Config.EosUrl + '/v1/chain/get_account';
        const Data=`{"account_name": "${accountName}"}`;

        //optional parameters
        const otherParams={
            headers:{"content-type":"application/json; charset=UTF-8"},
            body:Data,
            method:"POST"
        };

        let result = await fetch(Url, otherParams)
            .then(res => {
                if (!res.ok){
                    throw new FioError(res.json(),'Network response was not ok.');
                }
                return res.json()
            })
            .catch(rej => {
                console.log(`fetch rejection handler.`)
                throw rej;
            });

        return [true, result];
    }
	
	
}

// Custom Error with details object. Details is context specific.
class FioError extends Error {
    constructor(details, ...params) {
        // Pass remaining arguments (including vendor specific ones) to parent constructor
        super(...params);

        // Custom debugging information
        this.details = details; // this is a promise with error details
    }
}

module.exports = {Config,Helper,FioError};
