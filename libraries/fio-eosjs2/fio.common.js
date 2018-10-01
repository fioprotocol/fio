// General Configuration parameters
class Config {
}
Config.MaxAccountCreationAttempts=3;
Config.EosUrl='http://127.0.0.1:8888';
Config.DefaultPrivateKey = "5K2HBexbraViJLQUJVJqZc42A8dxkouCmzMamdrZsLHhUHv77jF"; // fioname11111
Config.NewAccountBuyRamQuantity="100.0000 SYS";
Config.NewAccountStakeNetQuantity="100.0000 SYS";
Config.NewAccountStakeCpuQuantity="100.0000 SYS";
Config.NewAccountTransfer=false;
Config.LogLevel=3; // Log levels 0 - 5 with increasing verbosity.

// Helper static functions
class Helper {
    static typeOf( obj ) {
        return ({}).toString.call( obj ).match(/\s(\w+)/)[1].toLowerCase();
    }

    static checkTypes( args, types ) {
        args = [].slice.call( args );
        for ( var i = 0; i < types.length; ++i ) {
            if ( Helper.typeOf( args[i] ) != types[i] ) {
                throw new TypeError( 'param '+ i +' must be of type '+ types[i] );
            }
        }
    }

    static randomString(length, chars) {
        var result = '';
        for (var i = length; i > 0; --i) result += chars[Math.floor(Math.random() * chars.length)];
        return result;
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