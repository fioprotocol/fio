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


	async requestfunds (requestid, requestor, requestee, chain, asset, quantity, memo, ownerpubkey, activepubkey) {
		fiocommon.Helper.checkTypes( arguments, ['string','string','string','string','string','string','string','string','string'] );
		
		      await fiocommon.helper.getAccount(requestor).catch(rej => {
                    console.log("requestor " + requestor + " does not exist")
                    throw rej;
               });
									
		
			   await fiocommon.helper.getAccount(requestee).catch(rej => {
                    console.log("requestee " + requestee + " does not exist")
                    throw rej;
               });
		
			
		
		   const result = await this.api.transact({
            actions: [
                {
                    account: 'requestor',
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
                        owner: {
                            threshold: 1,
                            keys: [{
                                key: ownerpubkey,
                                weight: 1
                            }],
                            accounts: [],
                            waits: []
                        },
                        active: {
                            threshold: 1,
                            keys: [{
                                key: activepubkey,
                                weight: 1
                            }],
                            accounts: [],
                            waits: []
                        },
                    },
                }
            ]
        }, {
            blocksBehind: 3,
            expireSeconds: 30,
        }).catch(rej => {
            console.log(`api.transact promise rejection handler.`)
            throw rej;
        });
		

	}

	async cancelrqst (fioappid, requestor, memo) {
	fiocommon.Helper.checkTypes( arguments, ['string','string','string'] );



	}

	
	async reportrqst (fioappid, requestee, obtid, memo) {
        fiocommon.Helper.checkTypes( arguments, ['string','string','string'] );

		
		
	}
	
	async rejectrqst (fioappid, requestee, memo) {
        fiocommon.Helper.checkTypes( arguments, ['string','string','string'] );

		
		
	}

	
	
module.exports = {FioFinance};