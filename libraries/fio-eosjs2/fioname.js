/** FioName implementation for Fio Javascript SDK
 *  Description: Fio class provided for generating user keys, usernames,
 creating accounts, registering fionames, adding addresses
 *  @author Ciju John
 *  @file fioname.js
 *  @copyright Dapix
 *
 *  Changes: 10-8-2018 Adam Androulidakis
 */


const eosjs2 = require('../eosjs2');
const fetch = require('node-fetch');                            // node only; not needed in browsers
const { TextDecoder, TextEncoder } = require('text-encoding');  // node, IE11 and IE Edge Browsers
const fiocommon=require('./fio.common.js');

// FIO methods class
class Fio {
    constructor() {
        const rpc = new eosjs2.Rpc.JsonRpc(fiocommon.Config.EosUrl, { fetch });
        const signatureProvider = new eosjs2.SignatureProvider([fiocommon.Config.SystemAccountKey]);
        this.api = new eosjs2.Api({ rpc, signatureProvider, textDecoder: new TextDecoder, textEncoder: new TextEncoder });
    }

    // Transfer entity quantity e.g. "4.010 SYS" from account to "to" account with a memo.
    // Returns tuple [status, eos response]
    async transfer(from, to, quantity, memo) {
        fiocommon.Helper.checkTypes( arguments, ['string', 'string', 'string', 'string'] );

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

        if (fiocommon.Config.LogLevel > 4) {
            console.log(`Private key: ${privateWif}`);
            console.log(`Public key: ${pubkey}`);
        }
        return [true, privateWif, pubkey];
    }

    // Generates a 12 random character EOS user name. No collision guarantees.
    generateUserName() {
        return [true, fiocommon.Helper.randomString(12, '12345abcdefghijklmnopqrstuvwxyz')];
    }

    // Will create a give username on the EOS chain.
    // Returns tuple [status, eos response]
    async activateAccount(creator, accountName, ownerPublicKey, ownerPrivateKey, activePublicKey, activePrivateKey, buyRamQuantity, stakeNetQuantity, stakeCpuQuantity, transfer) {
        fiocommon.Helper.checkTypes( arguments, ['string', 'string', 'string', 'string', 'string', 'string', 'string', 'string', 'string', 'boolean'] );

        if (fiocommon.Config.LogLevel > 3) {
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
        fiocommon.Helper.checkTypes( arguments, ['string',] );

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
                    throw new fiocommon.FioError(res.json(),'Network response was not ok.');
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
        fiocommon.Helper.checkTypes(arguments, ['string', 'string', 'string', 'string']);

        if (fiocommon.Config.LogLevel > 3) {
            console.log(`Active public key : ${activePublicKey}`);
        }

        const rpc = new eosjs2.Rpc.JsonRpc(fiocommon.Config.EosUrl, { fetch });
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
    async registerName(name, requestor, requestorActivePrivateKey, owner=fiocommon.Config.SystemAccount) {
        fiocommon.Helper.checkTypes(arguments, ['string', 'string', 'string']);

        if (fiocommon.Config.LogLevel > 3) {
            console.log(`Requestor ${requestor} registering name : ${name}`);
        }

        const rpc = new eosjs2.Rpc.JsonRpc(fiocommon.Config.EosUrl, { fetch });
        // include keys for requestor active
        const signatureProvider = new eosjs2.SignatureProvider([requestorActivePrivateKey]);
        let api = new eosjs2.Api({ rpc, signatureProvider, textDecoder: new TextDecoder, textEncoder: new TextEncoder });

        const result = await api.transact({
            actions: [{
                account: owner,
                name: 'registername',
                authorization: [{
                    actor: requestor,
                    permission: 'active',
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

    async availCheck(name) {
        fiocommon.Helper.checkTypes(arguments, ['string']);

        if (fiocommon.Config.LogLevel > 3) {
            console.log(`Check name : ${name}`);
        }

        const Url=fiocommon.Config.EosUrl + '/v1/chain/avail_check';
        const Data=`{"fio_name": "${name}"}`;
        if (fiocommon.Config.LogLevel > 3) {
            console.log(`url: ${Url}`);
            console.log(`data: ${Data}`);
        }

        //optional parameters
        const otherParams={
            headers:{"content-type":"application/json; charset=UTF-8"},
            body:Data,
            method:"POST"
        };

        let result = await fetch(Url, otherParams)
            .then(res => {
                if (!res.ok){
                    throw new fiocommon.FioError(res.json(),'Network response was not ok.');
                }
                return res.json()
            })
            .catch(rej => {
                console.log(`fetch rejection handler.`)
                throw rej;
            });

        return [true, result];
    }

    async lookupByName(name, chain=null) {
        fiocommon.Helper.checkTypes(arguments, ['string']);

        if (fiocommon.Config.LogLevel > 3) {
            console.log(`Lookup key by name : ${name}, chain: ${chain}`);
        }

        const Url=fiocommon.Config.EosUrl + '/v1/chain/fio_name_lookup';
        const Data=`{"fio_name": "${name}","chain":"${chain}"}`;
        if (fiocommon.Config.LogLevel > 3) {
            console.log(`url: ${Url}`);
            console.log(`data: ${Data}`);
        }

        //optional parameters
        const otherParams={
            headers:{"content-type":"application/json; charset=UTF-8"},
            body:Data,
            method:"POST"
        };

        let result = await fetch(Url, otherParams)
            .then(res => {
                if (!res.ok){
                    throw new fiocommon.FioError(res.json(),'Network response was not ok.');
                }
                return res.json()
            })
            .catch(rej => {
                console.log(`fetch rejection handler.`)
                throw rej;
            });

        return [true, result];
    }

    async getCurrencyBalance(account, symbol="FIO") {
        fiocommon.Helper.checkTypes(arguments, ['string']);

        // if (fiocommon.Config.LogLevel > 3) {
            console.log(`Lookup account ${account}, symbol ${symbol} balance`);
        // }

        const Url=fiocommon.Config.EosUrl + '/v1/chain/get_currency_balance';
        const Data=`{"account": "${account}","code":"${fiocommon.Config.TokenAccount}","symbol": "${symbol}"}`;
        if (fiocommon.Config.LogLevel > 3) {
            console.log(`url: ${Url}`);
            console.log(`data: ${Data}`);
        }

        //optional parameters
        const otherParams={
            headers:{"content-type":"application/json; charset=UTF-8"},
            body:Data,
            method:"POST"
        };

        let result = await fetch(Url, otherParams)
            .then(res => {
                if (!res.ok){
                    throw new fiocommon.FioError(res.json(),'Network response was not ok.');
                }
                return res.json()
            })
            .catch(rej => {
                console.log(`fetch rejection handler.`)
                throw rej;
            });

        return [true, result];
    }


    // Create random username and create new EOS account. Will re-attempt $(Config.MaxAccountCreationAttempts) times.
    // Returns tuple [status, eos response, accountName, accountOwnerKeys, accountActiveKeys]. accountOwnerKeys, accountActiveKeys are further string arrays of format [privateKey, publicKey].
    async createAccount(creator) {
        fiocommon.Helper.checkTypes( arguments, ['string',] );

        let maxAttempts=fiocommon.Config.MaxAccountCreationAttempts;
        let count=1;
        while(true) {
            if (fiocommon.Config.LogLevel > 2) {
                console.log(`Account creation Attempt ${count}`);
            }
            try {
                // Generate owner keys
                let ownerKey = await this.generateKeys().catch(rej => {
                    console.log(`Owner key generateKeys promise rejection handler.`)
                    throw rej;
                });
                if (!ownerKey[0]) {
                    throw new fiocommon.FioError(ownerKey);
                }

                // Generate active keys
                let activeKey = await this.generateKeys().catch(rej => {
                    console.log(`Active key generateKeys promise rejection handler.`)
                    throw rej;
                });
                if (!activeKey[0]) {
                    throw new fiocommon.FioError(activeKey);
                }
                if (fiocommon.Config.LogLevel > 3) {
                    console.log(`Owner private key: ${ownerKey[1]}, Owner public key : ${ownerKey[2]}`);
                    console.log(`Active private key: ${activeKey[1]}, Active public key : ${activeKey[2]}`);
                }

                // Generate user name
                let username = this.generateUserName();
                if (!username[0]) {
                    throw new fiocommon.FioError(username);
                }
                if (fiocommon.Config.LogLevel > 3) {
                    console.log(`User name  : ${username[1]}`);
                }

                // Activate account on EOS chain
                let buyRamQuantity=fiocommon.Config.NewAccountBuyRamQuantity;
                let stakeNetQuantity=fiocommon.Config.NewAccountStakeNetQuantity;
                let stakeCpuQuantity=fiocommon.Config.NewAccountStakeCpuQuantity;
                let transfer=fiocommon.Config.NewAccountTransfer;
                let activateAccountResult = await this.activateAccount(creator, username[1], ownerKey[2], ownerKey[1], activeKey[2], activeKey[1], buyRamQuantity, stakeNetQuantity, stakeCpuQuantity, transfer).catch(rej => {
                    console.log(`activateAccount promise rejection handler.`)
                    throw rej;
                });
                if (!activateAccountResult[0]) {
                    throw new fiocommon.FioError(activateAccountResult);
                }

                // Validate new account exists on EOS chain
                let getAccountResult = await this.getAccount(username[1]).catch(rej => {
                    console.log(`getAccount promise rejection handler.`)
                    throw rej;
                });
                if (!getAccountResult[0]) {
                    throw new fiocommon.FioError(getAccountResult);
                }

                let grantCodeTransferPermission = await this.grantCodeTransferPermission(username[1], activeKey[1], activeKey[2], fiocommon.Config.SystemAccount).catch(rej => {
                    console.log(`grantCodePermission promise rejection handler.`)
                    throw rej;
                });

                return [true,getAccountResult[1],username[1],[ownerKey[1],ownerKey[2]],[activeKey[1],activeKey[2]]];
            } catch (err) {
                if (count >= 3) {
                    if (err instanceof fiocommon.FioError) {
                        return err.details;
                    }
                    throw err;
                }
                count++;
            }
        }
    }

    /***
     * FIO name lookup by chain address
     * @param key       User key (blockchain specific account name)
     * @param chain     Three character blockchain acronym e.g. BTC, ETC, EOS etc.
     * @returns {Promise<*[]>}  Promise if successfully will return resolved name embedded in JSON string. JSON format: `{"name":"adam.dapix"}`
     */
    async lookupNameByAddress(key, chain) {
        fiocommon.Helper.checkTypes( arguments, ['string','string'] );

        const Url=fiocommon.Config.EosUrl + '/v1/chain/fio_key_lookup';
        const Data=`{"key": "${key}","chain":"${chain}"}`;
        if (fiocommon.Config.LogLevel > 3) {
            console.log(`url: ${Url}`);
            console.log(`data: ${Data}`);
        }

        //optional parameters
        const otherParams={
            headers:{"content-type":"application/json; charset=UTF-8"},
            body:Data,
            method:"POST"
        };

        let result = await fetch(Url, otherParams)
            .then(res => {
                if (!res.ok){
                    throw new fiocommon.FioError(res.json(),'Network response was not ok.');
                }
                return res.json()
            })
            .catch(rej => {
                console.log(`fetch rejection handler.`)
                throw rej;
            });

        return [true, result];
    }


    // Add address to the fioname
    // **PLEASE READ** fioname11111 account is currently
    // hard coded to fiocommon.Config.TestAccount (fio.common.js) and the actor should be changed to the 
    // account name (public address of the wallet user in production use.
    async addaddress(fioname, address, chain, requestor, requestorActivePrivateKey, owner=fiocommon.Config.SystemAccount) {
        fiocommon.Helper.checkTypes(arguments, ['string', 'string', 'string', 'string', 'string']);

        if (fiocommon.Config.LogLevel > 3) {
            console.log(`Requestor ${requestor} adding address. FIO name: ${fioname}, address: ${address}, chain: ${chain}`);
        }

        const rpc = new eosjs2.Rpc.JsonRpc(fiocommon.Config.EosUrl, {fetch});
        // include keys for both system and requestor active
        const signatureProvider = new eosjs2.SignatureProvider([requestorActivePrivateKey]);
        let api = new eosjs2.Api({rpc, signatureProvider, textDecoder: new TextDecoder, textEncoder: new TextEncoder});

        const result = await api.transact({
            actions: [{
                account: owner,
                name: 'addaddress',
                authorization: [{
                    actor: requestor,
                    permission: 'active',
                }],
                data: {
                    fio_user_name: fioname,
                    chain: chain,
                    address: address,
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
    } //addaddress


} //Fio class

module.exports = {Fio};
