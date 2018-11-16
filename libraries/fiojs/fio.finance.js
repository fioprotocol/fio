/** FioFinance implementation file for FIO Javascript SDK
 *  Description: FioFinance class to make requests, cancel requests,
 *               approve requests, reject requests, and lookup requests for a user
 *  @author Adam Androulidakis
 *  @file fio.finance.js
 *  @copyright Dapix
 *
 *  Changes: 10-8-2018 Adam Androulidakis
 */


const eosjs2 = require('../eosjs2');
const fetch = require('node-fetch');                            // node only; not needed in browsers
const { TextDecoder, TextEncoder } = require('text-encoding');  // node, IE11 and IE Edge Browsers
const fiocommon=require('./fio.common.js');

// FIO methods class
class FioFinance {
    constructor() {
        const rpc = new eosjs2.Rpc.JsonRpc(fiocommon.Config.EosUrl, { fetch });
        const signatureProvider = new eosjs2.SignatureProvider([fiocommon.Config.SystemAccountKey]);
        this.api = new eosjs2.Api({ rpc, signatureProvider, textDecoder: new TextDecoder, textEncoder: new TextEncoder });
    }

    /***
     *
     * @param requestid         Requestor specific requestid, intended for requestor side tracking
     * @param requestor         Requestor
     * @param requestee         Requestee
     * @param chain             Request chain acronym e.g. BTC. Refer
     * @param asset             Request asset type e.g. FIO
     * @param quantity          Request asset quantity e.g. 10.0000
     * @param memo              Request memo
     * @param requestorKey      Requestor private key
     * @returns {Promise<void>} Promise return array with status and JSON chain result
     */
    async requestfunds (requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey) {
        fiocommon.Helper.checkTypes( arguments, ['number','string','string','string','string','string','string','string'] );

        if (fiocommon.Config.LogLevel > 4) {
            console.log(`In Fio.requestfunds()`);
        }

        // not doing any chain side validation e.g. account validaty. This keeps this method lightweight. Expect all
        // validation to be performed in the "requestfunds" chain call.

        const rpc = new eosjs2.Rpc.JsonRpc(fiocommon.Config.EosUrl, { fetch });
        const signatureProvider = new eosjs2.SignatureProvider([requestorKey]);
        let api = new eosjs2.Api({ rpc, signatureProvider, textDecoder: new TextDecoder, textEncoder: new TextEncoder });

        const result = await api.transact({
            actions: [
                {
                    account: fiocommon.Config.FioFinanceAccount,
                    name: 'requestfunds',
                    authorization: [{
                        actor: requestor,
                        permission: 'active',
                    }],
                    data: {
                        requestid: requestid,
                        requestor: requestor,
                        requestee: requestee,
                        chain: chain,
                        asset: asset,
                        quantity: quantity,
                        memo: memo,
                    },
                }
            ]
        }, {
            blocksBehind: 3,
            expireSeconds: 30,
        }).catch(rej => {
            console.log(`api.transact promise rejection handler (requestfunds). `);
            throw rej;
        });

        if (fiocommon.Config.LogLevel > 4) {
            console.log(`Response: ${JSON.stringify(result, null, 2)}`);
        }
        return [true, result];
    }

    /***
     * Get pending request by requestee name
     *
     * @param requestee         Requestee
     * @param limit             Max match results
     * @returns {Promise<*[]>}  Promise return array with status and JSON chain result
     */
    async getPendingRequestByRequestee(requestee, limit = 10) {
        fiocommon.Helper.checkTypes( arguments, ['string', 'number'] );

        const Url=fiocommon.Config.EosUrl + '/v1/chain/get_table_rows';
        // const Data=`{"account_name": "${accountName}"}`;
        const Data=`{"json": true, "code": "${fiocommon.Config.FioFinanceAccount}", "scope": "${fiocommon.Config.FioFinanceAccount}", "table": "pendrqsts",
        "table_key": "", "lower_bound": "", "upper_bound": "${requestee}", "limit": ${limit}, "key_type": "name", "index_position": "2", "encode_type": "dec"}`;

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

        if (fiocommon.Config.LogLevel > 4) {
            console.log(`Response: ${JSON.stringify(result, null, 2)}`);
        }
        return [true, result];
    }

    /***
     * Get pending request by FIO App Id
     *
     * @param fioappid          fio app id
     * @returns {Promise<*[]>}  Promise return array with status and JSON chain result
     */
    async getPendingRequestByFioAppId(fioappid) {
        fiocommon.Helper.checkTypes( arguments, ['number'] );

        const Url=fiocommon.Config.EosUrl + '/v1/chain/get_table_rows';

        let tables = ["pendrqsts", "prsrqsts"];
        let result;
        for (let i = 0; i < tables.length; i++) {
            const Data=`{"json": true, "code": "${fiocommon.Config.FioFinanceAccount}", "scope": "${fiocommon.Config.FioFinanceAccount}", "table": "${tables[i]}",
        "table_key": "", "lower_bound": "${fioappid}", "upper_bound": "${fioappid+1}", "limit": 1, "key_type": "", "index_position": "1", "encode_type": "dec"}`;

            //optional parameters
            const otherParams={
                headers:{"content-type":"application/json; charset=UTF-8"},
                body:Data,
                method:"POST"
            };

            result = await fetch(Url, otherParams)
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

            if (result["rows"].length > 0) {
                break;
            }
        }

        if (fiocommon.Config.LogLevel > 4) {
            console.log(`Response: ${JSON.stringify(result, null, 2)}`);
        }
        return [true, result];
    }

    /***
     * Get FIO transaction logs by FIO App Id.
     *
     * @param fioappid          FIO App Id
     * @param limit             Max match results
     * @returns {Promise<*[]>}  Promise return array with status and JSON chain result
     */
    async getTrxLogsByFioAppId(fioappid, limit = 10) {
        fiocommon.Helper.checkTypes( arguments, ['number', 'number'] );

        const Url=fiocommon.Config.EosUrl + '/v1/chain/get_table_rows';

        const Data=`{"json": true, "code": "${fiocommon.Config.FioFinanceAccount}", "scope": "${fiocommon.Config.FioFinanceAccount}", "table": "trxlogs",
        "table_key": "", "lower_bound": "${fioappid}", "upper_bound": "${fioappid+1}", "limit": ${limit}, "key_type": "", "index_position": "1", "encode_type": "dec"}`;

        // optional parameters
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

        if (fiocommon.Config.LogLevel > 4) {
            console.log(`Response: ${JSON.stringify(result, null, 2)}`);
        }
        return [true, result];
    }

    /***
     *
     * @param requestid         Requestor specific requestid, intended for requestor side tracking
     * @param requestor         Requestor
     * @param memo              Request cancellation memo
     * @param requestorKey      Requestor private key
     * @returns {Promise<*[]>}  Promise return array with status and JSON chain result
     */
    async cancelrqst (requestid, requestor, memo, requestorKey) {
        fiocommon.Helper.checkTypes( arguments, ['number','string','string','string'] );

        const rpc = new eosjs2.Rpc.JsonRpc(fiocommon.Config.EosUrl, { fetch });
        const signatureProvider = new eosjs2.SignatureProvider([requestorKey]);
        let api = new eosjs2.Api({ rpc, signatureProvider, textDecoder: new TextDecoder, textEncoder: new TextEncoder });

        const result = await api.transact({
            actions: [
                {
                    account: fiocommon.Config.FioFinanceAccount,
                    name: 'cancelrqst',
                    authorization: [{
                        actor: requestor,
                        permission: 'active',
                    }],
                    data: {
                        requestid: requestid,
                        requestor: requestor,
                        memo: memo
                    },
                }
            ]
        }, {
            blocksBehind: 3,
            expireSeconds: 30,
        }).catch(rej => {
            console.log(`api.transact promise rejection handler (cancelrqst). `)
            throw rej;
        });

        if (fiocommon.Config.LogLevel > 4) {
            console.log(`Response: ${JSON.stringify(result, null, 2)}`);
        }
        return [true, result];
    }

    /***
     *
     * @param fioappid          Blockchain generated fioappid
     * @param requestee         Requestee
     * @param obtid             Other Blockchain Id
     * @param memo              Report(approve) memo
     * @param requesteeKey      Requestee private key
     * @returns {Promise<*[]>}  Promise return array with status and JSON chain result
     */
    async reportrqst (fioappid, requestee, obtid, memo, requesteeKey) {
        fiocommon.Helper.checkTypes( arguments, ['number','string','string','string','string'] );

        const rpc = new eosjs2.Rpc.JsonRpc(fiocommon.Config.EosUrl, { fetch });
        const signatureProvider = new eosjs2.SignatureProvider([requesteeKey]);
        let api = new eosjs2.Api({ rpc, signatureProvider, textDecoder: new TextDecoder, textEncoder: new TextEncoder });

        const result = await api.transact({
            actions: [
                {
                    account: fiocommon.Config.FioFinanceAccount,
                    name: 'reportrqst',
                    authorization: [{
                        actor: requestee,
                        permission: 'active',
                    }],
                    data: {
                        fioappid: fioappid,
                        requestee: requestee,
                        obtid: obtid,
                        memo: memo
                    },
                }
            ]
        }, {
            blocksBehind: 3,
            expireSeconds: 30,
        }).catch(rej => {
            console.log(`api.transact promise rejection handler (reportrqst).`)
            throw rej;
        });

        if (fiocommon.Config.LogLevel > 4) {
            console.log(`Response: ${JSON.stringify(result, null, 2)}`);
        }
        return [true, result];
    }

    /***
     *
     * @param fioappid          Blockchain generated fioappid
     * @param requestee         Requestee
     * @param memo              Rejection memo
     * @param requesteeKey      Requestee private key
     * @returns {Promise<*[]>}  Promise return array with status and JSON chain result
     */
    async rejectrqst (fioappid, requestee, memo, requesteeKey) {
        fiocommon.Helper.checkTypes( arguments, ['number','string','string','string'] );

        const rpc = new eosjs2.Rpc.JsonRpc(fiocommon.Config.EosUrl, { fetch });
        const signatureProvider = new eosjs2.SignatureProvider([requesteeKey]);
        let api = new eosjs2.Api({ rpc, signatureProvider, textDecoder: new TextDecoder, textEncoder: new TextEncoder });

        const result = await api.transact({
            actions: [
                {
                    account: fiocommon.Config.FioFinanceAccount,
                    name: 'rejectrqst',
                    authorization: [{
                        actor: requestee,
                        permission: 'active',
                    }],
                    data: {
                        fioappid: fioappid,
                        requestee: requestee,
                        memo: memo,
                    },
                }
            ]
        }, {
            blocksBehind: 3,
            expireSeconds: 30,
        }).catch(rej => {
            console.log(`api.transact promise rejection handler (rejectrqst).`)
            throw rej;
        });

        if (fiocommon.Config.LogLevel > 4) {
            console.log(`Response: ${JSON.stringify(result, null, 2)}`);
        }
        return [true, result];
    }
}

module.exports = {FioFinance};
