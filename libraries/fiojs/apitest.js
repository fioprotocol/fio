const minimist = require('minimist');
const fetch = require('node-fetch');  
const fiocommon=require('./fio.common.js');
const { Api, JsonRpc, RpcError, JsSignatureProvider } = require('eosjs');
const { TextDecoder, TextEncoder } = require('text-encoding');

const assert = require('assert');

async function main() {
	
		console.log(`Creating the default brd domain, adam.brd address.`)
		const test = registertest("brd");
		const test2 = registertest("adam.brd");
		const test3 = registertest("vulture.brd");
	
    }

main()
    .then(text => {
        return 0;
    })
    .catch(err => {
        console.error('Caught exception in main: ' + err, err.stack);
        // Deal with the fact the chain failed
    });


async function registertest(name)
{
	  const rpc = new JsonRpc(fiocommon.Config.EosUrl, { fetch });
        // include keys for requestor active
     const signatureProvider = new JsSignatureProvider([fiocommon.Config.SystemAccountKey]);
     const api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });

	const result = await api.transact({
            actions: [{
                account: "fio.system",
                name: 'registername',
                authorization: [{
                    actor: "fio.system",
                    permission: 'active',
                }],
                data: {
                    name: name,
                    requestor: "fio.system"
				},
			}]
			}, {
				blocksBehind: 3,
				expireSeconds: 30,
			}).catch(rej => {
				console.log(`Failed to create name.`)
				throw rej;
			});

	return result;

}