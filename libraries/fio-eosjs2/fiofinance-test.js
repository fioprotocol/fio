/** FioFinance-test for Fio Javascript SDK
 *  Description: FioFinance test file for fio.finance.js 
 *  @author Adam Androulidakis
 *  @file fiofinance-test.js
 *  @copyright Dapix
 *
 *  Changes: 10-8-2018 Adam Androulidakis
 */


const minimist = require('minimist');
const fiofinance=require('./fio.finance.js');
const fiocommon=require('./fio.common.js');

const assert = require('assert');

// /***
//  * Sleep function
//  * @param ms    milliseconds to sleep
//  * @returns {Promise}   void promise
//  */
// function sleep(ms) {
//     return new Promise(resolve => setTimeout(resolve, ms));
// }

// test function
async function testFunction(requestid, requestor, requestee, requestorKey, requesteeKey) {
    fiocommon.Helper.checkTypes( arguments, ['number','string','string','string','string'] );

    fio = new fiofinance.FioFinance();

    let chain =     "FIO";
    let asset =     "FIO";
    let quantity =  "10.0000";

    {
        ///////////TEST 1////////////
        console.log(`Starting test 1. Create request, reject request.`);

        let memo = "Request 1";

        let pendingRequestsCount;
        let result = await fio.getPendingRequestByRequestee(requestee, 100)
            .catch(rej => {
                console.log(`getPendingRequestByRequestee() 1 promise rejection handler.`);
                throw rej;
            });
        if (result[0]) {
            console.log(`getPendingRequestByRequestee() successful.`);
            pendingRequestsCount = result[1].rows.length;
        } else {
            throw new Error("fio.getPendingRequestByRequestee() 1 failed: " + result[1]);
        }
        console.log(`Starting pending requests count for requestee ${requestee}: ${pendingRequestsCount}`);

        // Send the funds request
        console.log(`Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);
        result = await fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
            .catch(rej => {
                console.log(`requestfunds() promise rejection handler.`);
                throw rej;
            });
        if (!result[0]) {
            throw new Error("fio.requestfunds() failed: " + result[1]);
        }
        else {
            console.log("requestfunds() successfull.");
            // console.log(result[1]);
        }
        await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

        let pendingRequest;
        result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
            .catch(rej => {
                console.log(`getPendingRequestByRequestee() 2 promise rejection handler.`);
                throw rej;
            });
        if (result[0]) {
            console.log(`getPendingRequestByRequestee() successful.`);
            console.log(`New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
            assert.strictEqual(pendingRequestsCount + 1, result[1].rows.length);
            pendingRequest = result[1].rows[result[1].rows.length - 1];
        } else {
            throw new Error("fio.getPendingRequestByRequestee() 2 failed: " + result[1]);
        }
        // console.log(`Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
        let fioAppId = pendingRequest["fioappid"];

        console.log(`Rejecting pending request FIO app id: ${fioAppId}`);
        //reject the request
        result = await fio.rejectrqst(fioAppId, requestee, memo, requesteeKey)
            .catch(rej => {
                console.log(`rejectrqst promise rejection handler.`);
                console.log(rej);
            });
        if (result[0]) {
            console.log(`rejectrqst() successful.`);
        } else {
            throw new Error("fio.rejectrqst() failed: " + result[1]);
        }
        await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

        result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
            .catch(rej => {
                console.log(`getPendingRequestByRequestee() 3 promise rejection handler.`);
                throw rej;
            });
        if (result[0]) {
            console.log(`getPendingRequestByRequestee() successful.`);
            console.log(`Current pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
            assert.strictEqual(pendingRequestsCount, result[1].rows.length);

        } else {
            throw new Error("fio.getPendingRequestByRequestee() 3 failed: " + result[1]);
        }
        if (result[1].rows.length > 0) { // Ensure rejected pending request isn't in the pending requests table
            pendingRequest = result[1].rows[result[1].rows.length - 1];
            // console.log(`Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
            let newFioAppId = pendingRequest["fioappid"];
            // console.log(`New pending request FIO app id: ${newFioAppId}`);
            assert.notStrictEqual(fioAppId, newFioAppId);
        }
        console.log(`Test 1 successful.`);
    }

    {
        ///////////TEST 2/////////////
        console.log(`Strating Test 2. Create request, report request.`);

        //send a new request
        let memo = "Request 2";
        requestee = [requestor, requestor = requestee][0];  // swap values
        requesteeKey = [requestorKey, requestorKey = requesteeKey][0];  // swap values

        // let rowsCount;
        let result = await fio.getPendingRequestByRequestee(requestee, 100)
            .catch(rej => {
                console.log(`getPendingRequestByRequestee() 1 promise rejection handler.`);
                throw rej;
            });
        if (result[0]) {
            console.log(`getPendingRequestByRequestee() successful.`);
            rowsCount = result[1].rows.length;
        } else {
            throw new Error("fio.getPendingRequestByRequestee() 1 failed: " + result[1]);
        }
        console.log(`Starting pending requests count for requestee ${requestee}: ${rowsCount}`);

        // Send the funds request
        console.log(`Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);
        result = await fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
            .catch(rej => {
                console.log(`requestfunds() promise rejection handler.`);
                throw rej;
            });
        if (!result[0]) {
            throw new Error("fio.requestfunds() failed: " + result[1]);
        }
        else {
            console.log("requestfunds() successfull.");
            // console.log(result[1]);
        }
        await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

        let pendingRequest;
        result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
            .catch(rej => {
                console.log(`getPendingRequestByRequestee() 2 promise rejection handler.`);
                throw rej;
            });
        if (result[0]) {
            console.log(`getPendingRequestByRequestee() successful.`);
            console.log(`New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
            assert.strictEqual(rowsCount + 1, result[1].rows.length);
            pendingRequest = result[1].rows[result[1].rows.length - 1];
        } else {
            throw new Error("fio.getPendingRequestByRequestee() 2 failed: " + result[1]);
        }
        // console.log(`Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
        let fioAppId = pendingRequest["fioappid"];

        console.log(`Reporting pending request FIO app id: ${fioAppId}`);
        //approve the request
        memo = "approved";
        let obtid = "0x123456789";
        result = await fio.reportrqst(fioAppId, requestee, obtid, memo, requesteeKey)
            .catch(rej => {
                console.log(`reportrqst promise rejection handler.`);
                console.log(rej);
            });
        if (result[0]) {
            console.log(`reportrqst() successful.`);
        } else {
            throw new Error("fio.reportrqst() failed: " + result[1]);
        }
        await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

        result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
            .catch(rej => {
                console.log(`getPendingRequestByRequestee() 3 promise rejection handler.`);
                throw rej;
            });
        if (result[0]) {
            console.log(`getPendingRequestByRequestee() successful.`);
            console.log(`Current pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
            assert.strictEqual(rowsCount, result[1].rows.length);

        } else {
            throw new Error("fio.getPendingRequestByRequestee() 3 failed: " + result[1]);
        }
        if (result[1].rows.length > 0) { // Ensure rejected pending request isn't in the pending requests table
            pendingRequest = result[1].rows[result[1].rows.length - 1];
            // console.log(`Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
            let newFioAppId = pendingRequest["fioappid"];
            // console.log(`New pending request FIO app id: ${newFioAppId}`);
            assert.notStrictEqual(fioAppId, newFioAppId);
        }
        console.log(`Test 2 successful.`);
    }

    {
        ///////////TEST 3/////////////
        console.log(`Starting Test 3. Create request, cancel request.`);

        //send a new request
        let memo = "Request 3";
        requestee = [requestor, requestor = requestee][0];  // swap values
        requesteeKey = [requestorKey, requestorKey = requesteeKey][0];  // swap values

        // let rowsCount;
        let result = await fio.getPendingRequestByRequestee(requestee, 100)
            .catch(rej => {
                console.log(`getPendingRequestByRequestee() 1 promise rejection handler.`);
                throw rej;
            });
        if (result[0]) {
            console.log(`getPendingRequestByRequestee() successful.`);
            rowsCount = result[1].rows.length;
        } else {
            throw new Error("fio.getPendingRequestByRequestee() 1 failed: " + result[1]);
        }
        console.log(`Starting pending requests count for requestee ${requestee}: ${rowsCount}`);

        // Send the funds request
        console.log(`Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);
        result = await fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
            .catch(rej => {
                console.log(`requestfunds() promise rejection handler.`);
                throw rej;
            });
        if (!result[0]) {
            throw new Error("fio.requestfunds() failed: " + result[1]);
        }
        else {
            console.log("requestfunds() successfull.");
            // console.log(result[1]);
        }
        await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

        let pendingRequest;
        result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
            .catch(rej => {
                console.log(`getPendingRequestByRequestee() 2 promise rejection handler.`);
                throw rej;
            });
        if (result[0]) {
            console.log(`getPendingRequestByRequestee() successful.`);
            console.log(`New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
            assert.strictEqual(rowsCount + 1, result[1].rows.length);
            pendingRequest = result[1].rows[result[1].rows.length - 1];
        } else {
            throw new Error("fio.getPendingRequestByRequestee() 2 failed: " + result[1]);
        }
        // console.log(`Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
        let fioAppId = pendingRequest["fioappid"];

        console.log(`Canceling pending request FIO app id: ${fioAppId}`);

        // Cancel the request
        memo = "cancelled";
        fio.cancelrqst(requestid, requestor, memo, requestorKey)
            .catch(rej => {
                console.log(`cancelrqst promise rejection handler.`);
                console.log(rej);
            });
        if (result[0]) {
            console.log(`cancelrqst() successful.`);
        } else {
            throw new Error("fio.cancelrqst() failed: " + result[1]);
        }
        await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

        result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
            .catch(rej => {
                console.log(`getPendingRequestByRequestee() 3 promise rejection handler.`);
                throw rej;
            });
        if (result[0]) {
            console.log(`getPendingRequestByRequestee() successful.`);
            console.log(`Current pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
            assert.strictEqual(rowsCount, result[1].rows.length);

        } else {
            throw new Error("fio.getPendingRequestByRequestee() 3 failed: " + result[1]);
        }
        if (result[1].rows.length > 0) { // Ensure rejected pending request isn't in the pending requests table
            pendingRequest = result[1].rows[result[1].rows.length - 1];
            // console.log(`Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
            let newFioAppId = pendingRequest["fioappid"];
            // console.log(`New pending request FIO app id: ${newFioAppId}`);
            assert.notStrictEqual(fioAppId, newFioAppId);
        }
        console.log(`Test 3 successful.`);
    }

}

let args = minimist(process.argv.slice(2), {
    alias: {
        h: 'help',
        i: 'requestid',
        s: 'requestor',
        r: 'requestee',
        k: 'requestorkey',
        e: 'requesteekey'
    },
    default: {
        help: false,
        requestid: 9000,
        requestor: 'fioname11111',
        requestee: 'fioname22222',
        requestorkey: "5K2HBexbraViJLQUJVJqZc42A8dxkouCmzMamdrZsLHhUHv77jF",
        requesteekey: "5JA5zQkg1S59swPzY6d29gsfNhNPVCb7XhiGJAakGFa7tEKSMjT"
    },
});

if (args.help) {
    console.log("arguments: [-h|--help]\n");
    console.log("[-i|--requestid <requestid default 9000>]\n");
    console.log("[-s|--requestor <requestor default fioname11111>]\n");
    console.log("[-r|--requestee <requestid default fioname22222>]\n");
    console.log("[-k|--requestorkey <requestor private key>]\n");
    console.log("[-e|--requesteekey <requestid private key>]\n");
    return 0;
}

console.log(`Request ID: ${args.requestid}`);
console.log(`Requestor account: ${args.requestor}`);
console.log(`Requestee account: ${args.requestee}`);

(async() => {
    await testFunction(args.requestid, args.requestor, args.requestee, args.requestorkey, args.requesteekey). catch (err => {
        console.log('Caught exception in main: ' + err);
    });
})();
