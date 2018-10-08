// General Configuration parameters
class Config {
}
Config.MaxAccountCreationAttempts=3;
Config.EosUrl='http://127.0.0.1:8888';
Config.DefaultPrivateKey = "5K2HBexbraViJLQUJVJqZc42A8dxkouCmzMamdrZsLHhUHv77jF"; // fioname11111
Config.NewAccountBuyRamQuantity="100.0000 FIO";
Config.NewAccountStakeNetQuantity="100.0000 FIO";
Config.NewAccountStakeCpuQuantity="100.0000 FIO";
Config.NewAccountTransfer=false;
Config.LogLevel=3; // Log levels 0 - 5 with increasing verbosity.

// Helper static functions
class Helper {
    static typeOf( obj ) {
				
		// convert to string and make lower case
        var processed =  ({}).toString.call( obj ).match(/\s(\w+)/)[1].toLowerCase();
		//Remove whitespaces and return
		return processed.replace(/^[ ]+|[ ]+$/g,'');
		
    }

    static checkTypes( args, types ) {
        args = [].slice.call( args );
        for ( var i = 0; i < types.length; ++i ) {
            if ( Helper.typeOf( args[i] ) != types[i] )
                throw new TypeError( 'param '+ i +' must be of type '+ types[i] );
				
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