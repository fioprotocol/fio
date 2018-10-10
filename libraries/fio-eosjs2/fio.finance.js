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
        const signatureProvider = new eosjs2.SignatureProvider([fiocommon.Config.DefaultPrivateKey]);
        this.api = new eosjs2.Api({ rpc, signatureProvider, textDecoder: new TextDecoder, textEncoder: new TextEncoder });
    }


	async requestfunds (requestid, requestor, requestee, chain, asset, quantity, memo) {
		fiocommon.Helper.checkTypes( arguments, ['string','string','string','string','string','string','string'] );

			
		   const result = await this.api.transact({
            actions: [
                {
                    account: fiocommon.Config.TestAccount,
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
		

	}

	async cancelrqst (fioappid, requestor, memo) {
	fiocommon.Helper.checkTypes( arguments, ['string','string','string'] );

            const result = await this.api.transact({
            actions: [
                {
                    account: 'requestor',
                    name: 'cancelrqst',
                    authorization: [{
                        actor: requestor,
                        permission: 'active',
                    }],
                    data: {
                        fioappid: fioappid,
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

	}

	
	async reportrqst (fioappid, requestee, obtid, memo) {
        fiocommon.Helper.checkTypes( arguments, ['string','string','string','string'] );

		   const result = await this.api.transact({
           actions: [
                {
                    account: 'requestee',
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
		
	}
	
	async rejectrqst (fioappid, requestee, memo) {
        fiocommon.Helper.checkTypes( arguments, ['string','string','string'] );

		
		   const result = await this.api.transact({
            actions: [
                {
                    account: 'requestee',
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
		
	}

}
	
module.exports = {FioFinance};

