const eosjs2 = require('../eosjs2');
const minimist = require('minimist');
const fetch = require('node-fetch');                            // node only; not needed in browsers
const { TextDecoder, TextEncoder } = require('text-encoding');  // node, IE11 and IE Edge Browsers

// General Configuration parameters
class Config {
}
Config.MaxAccountCreationAttempts=3;
Config.EosUrl =                     'http://127.0.0.1:8888';
Config.SystemAccount =              "fio.system";
Config.SystemAccountKey =           "5KBX1dwHME4VyuUss2sYM25D5ZTDvyYrbEz37UJqwAVAsR4tGuY"; // ${Config.SystemAccount}
Config.NewAccountBuyRamQuantity=    "100.0000 FIO";
Config.NewAccountStakeNetQuantity=  "100.0000 FIO";
Config.NewAccountStakeCpuQuantity=  "100.0000 FIO";
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

// FIO methods class
class Fio {
    constructor() {
        const rpc = new eosjs2.Rpc.JsonRpc(Config.EosUrl, { fetch });
        const signatureProvider = new eosjs2.SignatureProvider([Config.SystemAccountKey]);
        this.api = new eosjs2.Api({ rpc, signatureProvider, textDecoder: new TextDecoder, textEncoder: new TextEncoder });
    }

    // Transfer entity quantity e.g. "4.010 SYS" from account to "to" account with a memo.
    // Returns tuple [status, eos response]
    async transfer(from, to, quantity, memo) {
        Helper.checkTypes( arguments, ['string', 'string', 'string', 'string'] );

        const result = await this.api.transact({
            actions: [{
                account: 'eosio.token',
                name: 'transfer',
                authorization: [{
                    actor: from,
                    permission: 'active',
                }],
                data: {
                    from: from,
                    to: to,
                    quantity: quantity,
                    memo: memo,
                },
            }]
        }, {
            blocksBehind: 3,
            expireSeconds: 30,
        }).catch(rej => {
            console.log(`api.transact promise rejection handler.`)
            throw rej;
        });
        //console.log(JSON.stringify(result, null, 2));
        return [true, result];
    }

    // Generates a random private-public key pair.
    // Returns an promise array. arr[0] is private key, arr[1] is public key
    async generateKeys () {
        let {PrivateKey, PublicKey, Signature, Aes, key_utils, config} = require('eosjs-ecc')
        let privateKey = await PrivateKey.randomKey().catch(rej => {
            console.log(`PrivateKey.randomKey() promise rejection handler.`)
            throw rej;
        });

        let privateWif = privateKey.toWif();

        // Convert to a public key
        let pubkey = PrivateKey.fromString(privateWif).toPublic().toString();

        if (Config.LogLevel > 4) {
            console.log(`Private key: ${privateWif}`);
            console.log(`Public key: ${pubkey}`);
        }
        return [true, privateWif, pubkey];
    }

    // Generates a 12 random character EOS user name. No collision guarantees.
    generateUserName() {
        return [true, Helper.randomString(12, '12345abcdefghijklmnopqrstuvwxyz')];
    }

    // Will create a give username on the EOS chain.
    // Returns tuple [status, eos response]
    async activateAccount(creator, accountName, ownerPublicKey, ownerPrivateKey, activePublicKey, activePrivateKey, buyRamQuantity, stakeNetQuantity, stakeCpuQuantity, transfer) {
        Helper.checkTypes( arguments, ['string', 'string', 'string', 'string', 'string', 'string', 'string', 'string', 'string', 'boolean'] );

        if (Config.LogLevel > 3) {
            console.log(`Owner private key: ${ownerPrivateKey}, Owner public key : ${ownerPublicKey}`);
            console.log(`Active private key: ${activePrivateKey}, Active public key : ${activePublicKey}`);
        }

        // Execute newaccount transactions
        const result = await this.api.transact({
            actions: [
                {
                    account: 'eosio',
                    name: 'newaccount',
                    authorization: [{
                        actor: creator,
                        permission: 'active',
                    }],
                    data: {
                        creator: creator,
                        name: accountName,
                        owner: {
                            threshold: 1,
                            keys: [{
                                key: ownerPublicKey,
                                weight: 1
                            }],
                            accounts: [],
                            waits: []
                        },
                        active: {
                            threshold: 1,
                            keys: [{
                                key: activePublicKey,
                                weight: 1
                            }],
                            accounts: [],
                            waits: []
                        },
                    },
                },
                {
                    account: 'eosio',
                    name: 'buyram',
                    authorization: [{
                        actor: creator,
                        permission: 'active',
                    }],
                    data: {
                        payer: creator,
                        receiver: accountName,
                        quant: buyRamQuantity
                    },
                },
                {
                    account: 'eosio',
                    name: 'delegatebw',
                    authorization: [{
                        actor: creator,
                        permission: 'active',
                    }],
                    data: {
                        from: creator,
                        receiver: accountName,
                        stake_net_quantity: stakeNetQuantity,
                        stake_cpu_quantity: stakeCpuQuantity,
                        transfer: (transfer ? 1 : 0),
                    }
                }
            ]
        }, {
            blocksBehind: 3,
            expireSeconds: 30,
        }).catch(rej => {
            console.log(`api.transact promise rejection handler.`)
            throw rej;
        });

        //console.log(JSON.stringify(result, null, 2));
        return [true, result];
    }

    // Get account details
    // Returns tuple [status, eos response]
    async getAccount(accountName) {
        Helper.checkTypes( arguments, ['string',] );

        const Url=Config.EosUrl + '/v1/chain/get_account';
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

    /***
     * Grants FIO system code account ${${Config.SystemAccount}@eosio.code} permission to run as new account.
     * @param accountName       New account name
     * @param activePrivateKey  New account private key
     * @param activePublicKey   New account public key
     * @param systemAccount     FIO system account e.g. ${Config.SystemAccount}
     * @returns {Promise<*[]>}  promise result is an array with boolean status and updateauth JSON response
     */
    async grantCodeTransferPermission(accountName, activePrivateKey, activePublicKey, systemAccount) {
        Helper.checkTypes(arguments, ['string', 'string', 'string', 'string']);

        if (Config.LogLevel > 3) {
            console.log(`Active public key : ${activePublicKey}`);
        }

        const rpc = new eosjs2.Rpc.JsonRpc(Config.EosUrl, { fetch });
        const signatureProvider = new eosjs2.SignatureProvider([activePrivateKey]);
        let api = new eosjs2.Api({ rpc, signatureProvider, textDecoder: new TextDecoder, textEncoder: new TextEncoder });

        const result = await api.transact({
            actions: [{
                account: 'eosio',
                name: 'updateauth',
                authorization: [{
                    actor: accountName,
                    permission: 'active',
                }],
                data: {
                    account: accountName,
                    permission: 'active',
                    parent: 'owner',
                    auth: {
                        threshold: 1,
                        keys: [{
                            key: activePublicKey,
                            weight: 1
                        }],
                        accounts: [{
                            permission: {
                                actor: systemAccount,
                                permission: 'eosio.code'
                            },
                            weight: 1
                        }],
                        waits: []
                    }
                },
            }]
        }, {
            blocksBehind: 3,
            expireSeconds: 30,
        }).catch(rej => {
            console.log(`api.transact promise rejection handler.`)
            throw rej;
        });
        //console.log(JSON.stringify(result, null, 2));
        return [true, result];
    }

    /***
     * Register a FIO domain or name
     * @param name              domain or FIO name to be registered
     * @param requestor         requestor account name
     * @param requestorActivePrivateKey  requestor active private key
     * @param owner             name registration contract owner
     * @returns {Promise<*[]>}  promise result is an array with boolean status and registername JSON response
     */
    async registerName(name, requestor, requestorActivePrivateKey, owner=Config.SystemAccount) {
        Helper.checkTypes(arguments, ['string', 'string', 'string']);

        if (Config.LogLevel > 3) {
            console.log(`Requestor ${requestor} registering name : ${name}`);
        }

        const rpc = new eosjs2.Rpc.JsonRpc(Config.EosUrl, { fetch });
        // include keys for both system and requestor active
        const signatureProvider = new eosjs2.SignatureProvider([Config.SystemAccountKey, requestorActivePrivateKey]);
        let api = new eosjs2.Api({ rpc, signatureProvider, textDecoder: new TextDecoder, textEncoder: new TextEncoder });

        const result = await api.transact({
            actions: [{
                account: owner,
                name: 'registername',
                authorization: [{
                    actor: requestor,
                    permission: 'active',
                },{
                    actor: owner,
                    permission: "active"
                }],
                data: {
                    name: name,
                    requestor: requestor
               },
            }]
        }, {
            blocksBehind: 3,
            expireSeconds: 30,
        }).catch(rej => {
            console.log(`api.transact promise rejection handler.`)
            throw rej;
        });
        //console.log(JSON.stringify(result, null, 2));
        return [true, result];
    }


    // Create random username and create new EOS account. Will re-attempt $(Config.MaxAccountCreationAttempts) times.
    // Returns tuple [status, eos response, accountName, accountOwnerKeys, accountActiveKeys]. accountOwnerKeys, accountActiveKeys are further string arrays of format [privateKey, publicKey].
    async createAccount(creator) {
        Helper.checkTypes( arguments, ['string',] );

        let maxAttempts=Config.MaxAccountCreationAttempts;
        let count=1;
        while(true) {
            if (Config.LogLevel > 2) {
                console.log(`Account creation Attempt ${count}`);
            }
            try {
                // Generate owner keys
                let ownerKey = await this.generateKeys().catch(rej => {
                    console.log(`Owner key generateKeys promise rejection handler.`)
                    throw rej;
                });
                if (!ownerKey[0]) {
                    throw new FioError(ownerkey);
                }

                // Generate active keys
                let activeKey = await this.generateKeys().catch(rej => {
                    console.log(`Active key generateKeys promise rejection handler.`)
                    throw rej;
                });
                if (!activeKey[0]) {
                    throw new FioError(activeKey);
                }
                if (Config.LogLevel > 3) {
                    console.log(`Owner private key: ${ownerKey[1]}, Owner public key : ${ownerKey[2]}`);
                    console.log(`Active private key: ${activeKey[1]}, Active public key : ${activeKey[2]}`);
                }

                // Generate user name
                let username = this.generateUserName();
                if (!username[0]) {
                    throw new FioError(username);
                }
                if (Config.LogLevel > 3) {
                    console.log(`User name  : ${username[1]}`);
                }

                // Activate account on EOS chain
                let buyRamQuantity=Config.NewAccountBuyRamQuantity;
                let stakeNetQuantity=Config.NewAccountStakeNetQuantity;
                let stakeCpuQuantity=Config.NewAccountStakeCpuQuantity;
                let transfer=Config.NewAccountTransfer;
                let activateAccountResult = await this.activateAccount(creator, username[1], ownerKey[2], ownerKey[1], activeKey[2], activeKey[1], buyRamQuantity, stakeNetQuantity, stakeCpuQuantity, transfer).catch(rej => {
                    console.log(`activateAccount promise rejection handler.`)
                    throw rej;
                });
                if (!activateAccountResult[0]) {
                    throw new FioError(activateAccountResult);
                }

                // Validate new account exists on EOS chain
                let getAccountResult = await this.getAccount(username[1]).catch(rej => {
                    console.log(`getAccount promise rejection handler.`)
                    throw rej;
                });
                if (!getAccountResult[0]) {
                    throw new FioError(getAccountResult);
                }

                let grantCodeTransferPermission = await this.grantCodeTransferPermission(username[1], activeKey[1], activeKey[2], Config.SystemAccount).catch(rej => {
                    console.log(`grantCodePermission promise rejection handler.`)
                    throw rej;
                });

                return [true,getAccountResult[1],username[1],[ownerKey[1],ownerKey[2]],[activeKey[1],activeKey[2]]];
            } catch (err) {
                if (count >= 3) {
                    if (err instanceof FioError) {
                        return err.details;
                    }
                    throw err;
                }
                count++;
            }
        }
    }

}

// test function
async function testFunction() {
    try {

        fio = new Fio();

        let createAccountResult = await fio.createAccount(Config.SystemAccount)
            .catch(rej => {
                console.log(`createAccount rejection handler.`)
                throw rej;
            });
        if (createAccountResult[0]) {
            console.log("Account creation successful.");
            console.log(`Account name: ${createAccountResult[2]}\nOwner key: ${createAccountResult[3][0]}\nOwner key: ${createAccountResult[3][1]}\n` +
                `Active key: ${createAccountResult[4][0]}\nActive key: ${createAccountResult[4][1]}\n`);
            // console.log(`Account name: ${result[1]}`);

        } else {
            console.log("Account creation failed.");
            console.log(JSON.stringify(createAccountResult[1], null, 2));
            return -1;
        }

        // async transfer(from, to, quantity, memo) {
        let transferResult = await fio.transfer(Config.SystemAccount, createAccountResult[2], "200.0000 FIO", "initial transfer")
            .catch(rej => {
                console.log(`transfer rejection handler.`)
                throw rej;
            });
        if (transferResult[0]) {
            console.log("transfer successful.");
        } else {
            console.log("transfer failed.");
            return -1;
        }

        let registerNameResult = await fio.registerName(createAccountResult[2], createAccountResult[2], createAccountResult[4][0])
            .catch(rej => {
                console.log(`registerName domain rejection handler.`)
                throw rej;
            });
        if (registerNameResult[0]) {
            console.log("Domain registration succeeded.");
        } else {
            console.log("Domain registration failed.");
            return -1;
            // console.log(JSON.stringify(result[1], null, 2));
        }

        registerNameResult = await fio.registerName("ciju."+createAccountResult[2], createAccountResult[2], createAccountResult[4][0])
            .catch(rej => {
                console.log(`registerName name rejection handler.`)
                throw rej;
            });
        if (registerNameResult[0]) {
            console.log("Name registration succeeded.");
        } else {
            console.log("Name registration failed.");
            return -1;
            // console.log(JSON.stringify(result[1], null, 2));
        }

        // fio.lookupNameByAddress("abcdefgh","ETH").then(result => {
        //     if (result[0]) {
        //         console.log("Name lookup by address successful.");
        //         console.log(`Resolved name: ${result[1]["name"]}`);
        //     } else {
        //         console.log("Create account transaction failed.");
        //         console.log(JSON.stringify(result[1], null, 2));
        //     }
        // })
        //     .catch(rej => {
        //         console.log(`lookupNameByAddress rejection handler.`)
        //         throw rej;
        //     });

    } catch (err) {
        console.log('Caught exception in main: ' + err);
    }
}

// main
(async() => {
    testFunction()
})();
