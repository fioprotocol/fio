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

    async createAccount(creator) {
}

module.exports = {FioFinance};