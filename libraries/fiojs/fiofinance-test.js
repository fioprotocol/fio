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

async function test_requestfunds(testContext) {
    fiocommon.Helper.checkTypes(arguments, ['object']);

    let requestid = testContext["requestid"];
    let requestor = testContext["requestor"];
    let requestee = testContext["requestee"]
    let requestorKey = testContext["requestorKey"];
    let logLevel = testContext["logLevel"];
    let chain = testContext["chain"];
    let asset = testContext["asset"];
    let quantity = testContext["quantity"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_requestfunds ***")

    fio = new fiofinance.FioFinance();

    let memo = "Request 1";

    let pendingRequestsCount;
    let result = await
    fio.getPendingRequestByRequestee(requestee, 100)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 1 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "getPendingRequestByRequestee() successful.");

    pendingRequestsCount = result[1].rows.length;
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Starting pending requests count for requestee ${requestee}: ${pendingRequestsCount}`);

    // Send the funds request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);

    result = await
    fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "requestfunds() promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL requestfunds(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "requestfunds() successfull.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    let pendingRequest;
    result = await
    fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 2 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `getPendingRequestByRequestee() successful.`);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);

    assert.strictEqual(pendingRequestsCount + 1, result[1].rows.length);
    pendingRequest = result[1].rows[result[1].rows.length - 1];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_requestfunds ***")
}

async function test_rejectrqst(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let requestid   = testContext["requestid"];
    let requestor   = testContext["requestor"];
    let requestee   = testContext["requestee"]
    let requestorKey    = testContext["requestorKey"];
    let requesteeKey    = testContext["requesteeKey"];
    let logLevel    = testContext["logLevel"];
    let chain   = testContext["chain"];
    let asset   = testContext["asset"];
    let quantity    =  testContext["quantity"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_rejectrqst ***")

    fio = new fiofinance.FioFinance();

    let memo = "Request 1";

    // Retrieve startring pending requests count
    let pendingRequestsCount;
    let result = await fio.getPendingRequestByRequestee(requestee, 100)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 1 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee() 1");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "getPendingRequestByRequestee() successful.");

    pendingRequestsCount = result[1].rows.length;
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Starting pending requests count for requestee ${requestee}: ${pendingRequestsCount}`);

    // Send the funds request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);

    result = await fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "requestfunds() promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL requestfunds() 1");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "requestfunds() successfull.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 2 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `getPendingRequestByRequestee() successful.`);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);

    // assert.strictEqual(pendingRequestsCount + 1, result[1].rows.length);
    let pendingRequest = result[1].rows[result[1].rows.length - 1];


    // retrieve fioappid of latest pending request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, `Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
    let fioAppId = pendingRequest["fioappid"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Rejecting pending request FIO app id: ${fioAppId}`);
    //reject the request
    result = await fio.rejectrqst(fioAppId, requestee, memo, requesteeKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "rejectrqst promise rejection handler.");
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej);
        });
    assert(result[0], "FAIL fio.rejectrqst(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "rejectrqst() successful.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 3 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "getPendingRequestByRequestee() successful.");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Current pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
    assert.strictEqual(pendingRequestsCount, result[1].rows.length);

    if (result[1].rows.length > 0) { // Ensure rejected pending request isn't in the pending requests table
        pendingRequest = result[1].rows[result[1].rows.length - 1];
        fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, `Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
        let newFioAppId = pendingRequest["fioappid"];
        fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New pending request FIO app id: ${newFioAppId}`);
        assert.notStrictEqual(fioAppId, newFioAppId);
    }

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_rejectrqst ***")
}

async function test_invalidId_rejectrqst(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let requestid   = testContext["requestid"];
    let requestor   = testContext["requestor"];
    let requestee   = testContext["requestee"]
    let requestorKey    = testContext["requestorKey"];
    let requesteeKey    = testContext["requesteeKey"];
    let logLevel    = testContext["logLevel"];
    let chain   = testContext["chain"];
    let asset   = testContext["asset"];
    let quantity    =  testContext["quantity"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_invalidId_rejectrqst ***")

    fio = new fiofinance.FioFinance();

    let memo = "Request 1";

    // Send the funds request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);

    result = await fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "requestfunds() promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL requestfunds() 1");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "requestfunds() successfull.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 2 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `getPendingRequestByRequestee() successful.`);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);

    let pendingRequest = result[1].rows[result[1].rows.length - 1];

    // retrieve fioappid of latest pending request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, `Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
    let fioAppId = pendingRequest["fioappid"];
    fioAppId += 10 // invalid FIO App Id

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Rejecting invalid pending request FIO app id: ${fioAppId}`);

    rejectrqstSuccess=false
    result = await fio.rejectrqst(fioAppId, requestee, memo, requesteeKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, rej);

            // expect the 500 error to be part of the thrown exception
            let expectedResponseCode="500";
            let expectedResponseMessage="Internal Service Error - fc";

            rejectrqstJson = rej.json;
            assert(rejectrqstJson.code == expectedResponseCode, "rejectrqst action response invalid code.");
            assert(rejectrqstJson.message == expectedResponseMessage, "rejectrqst action response invalid message.");

            rejectrqstSuccess = true
        });
    assert(rejectrqstSuccess, "rejectrqst() with invalid FIO App Id should have failed.");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_invalidId_rejectrqst ***")
}

async function test_invalidRequestee_rejectrqst(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let requestid   = testContext["requestid"];
    let requestor   = testContext["requestor"];
    let requestee   = testContext["requestee"]
    let requestorKey    = testContext["requestorKey"];
    let requesteeKey    = testContext["requesteeKey"];
    let logLevel    = testContext["logLevel"];
    let chain   = testContext["chain"];
    let asset   = testContext["asset"];
    let quantity    =  testContext["quantity"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_invalidRequestee_rejectrqst ***")

    fio = new fiofinance.FioFinance();

    let memo = "Request 1";

    // Send the funds request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);

    result = await fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "requestfunds() promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL requestfunds() 1");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "requestfunds() successfull.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 2 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `getPendingRequestByRequestee() successful.`);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);

    let pendingRequest = result[1].rows[result[1].rows.length - 1];

    // retrieve fioappid of latest pending request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, `Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
    let fioAppId = pendingRequest["fioappid"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Rejecting invalid pending request FIO app id: ${fioAppId}`);

    rejectrqstSuccess=false
    result = await fio.rejectrqst(fioAppId, requestor, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, rej);

            // expect the 500 error to be part of the thrown exception
            let expectedResponseCode="500";
            let expectedResponseMessage="Internal Service Error - fc";

            rejectrqstJson = rej.json;
            assert(rejectrqstJson.code == expectedResponseCode, "rejectrqst action response invalid code.");
            assert(rejectrqstJson.message == expectedResponseMessage, "rejectrqst action response invalid message.");

            rejectrqstSuccess = true
        });
    assert(rejectrqstSuccess, "rejectrqst() with invalid FIO App Id should have failed.");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_invalidRequestee_rejectrqst ***")
}


async function test_reportrqst(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let requestid   = testContext["requestid"];
    let requestor   = testContext["requestor"];
    let requestee   = testContext["requestee"]
    let requestorKey    = testContext["requestorKey"];
    let requesteeKey    = testContext["requesteeKey"];
    let logLevel    = testContext["logLevel"];
    let chain   = testContext["chain"];
    let asset   = testContext["asset"];
    let quantity    =  testContext["quantity"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_reportrqst. Create request, report request. ***")

    fio = new fiofinance.FioFinance();

    //send a new request
    let memo = "Request 2";
    let result = await fio.getPendingRequestByRequestee(requestee, 100)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 1 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `getPendingRequestByRequestee() successful.`);
    let rowsCount = result[1].rows.length;
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Starting pending requests count for requestee ${requestee}: ${rowsCount}`);

    // Send the funds request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);
    result = await fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "requestfunds() promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL requestfunds(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "requestfunds() successfull.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 2 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `getPendingRequestByRequestee() successful.`);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
    assert.strictEqual(rowsCount + 1, result[1].rows.length);
    let pendingRequest = result[1].rows[result[1].rows.length - 1];
    let fioAppId = pendingRequest["fioappid"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Reporting pending request FIO app id: ${fioAppId}`);
    //approve the request
    memo = "approved";
    let obtid = "0x123456789";
    result = await fio.reportrqst(fioAppId, requestee, obtid, memo, requesteeKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, `reportrqst promise rejection handler.`);
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej);
            throw rej;
        });
    assert(result[0], "FAIL reportrqst(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `reportrqst() successful.`);

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, `getPendingRequestByRequestee() 3 promise rejection handler.`);
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `getPendingRequestByRequestee() successful.`);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Current pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
    assert.strictEqual(rowsCount, result[1].rows.length);

    if (result[1].rows.length > 0) { // Ensure rejected pending request isn't in the pending requests table
        pendingRequest = result[1].rows[result[1].rows.length - 1];
        // console.log(`Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
        let newFioAppId = pendingRequest["fioappid"];
        // console.log(`New pending request FIO app id: ${newFioAppId}`);
        assert.notStrictEqual(fioAppId, newFioAppId);
    }
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_reportrqst. ***")
}

async function test_invalidId_reportrqst(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let requestid   = testContext["requestid"];
    let requestor   = testContext["requestor"];
    let requestee   = testContext["requestee"]
    let requestorKey    = testContext["requestorKey"];
    let requesteeKey    = testContext["requesteeKey"];
    let logLevel    = testContext["logLevel"];
    let chain   = testContext["chain"];
    let asset   = testContext["asset"];
    let quantity    =  testContext["quantity"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_invalidId_reportrqst ***")

    fio = new fiofinance.FioFinance();

    let memo = "Request 1";

    // Send the funds request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);

    result = await fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "requestfunds() promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL requestfunds() 1");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "requestfunds() successfull.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 2 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `getPendingRequestByRequestee() successful.`);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);

    let pendingRequest = result[1].rows[result[1].rows.length - 1];

    // retrieve fioappid of latest pending request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, `Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
    let fioAppId = pendingRequest["fioappid"];
    fioAppId += 10 // invalid FIO App Id

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Reporting invalid pending request FIO app id: ${fioAppId}`);

    reportrqstSuccess=false
    //approve the request
    memo = "approved";
    let obtid = "0x123456789";
    result = await fio.reportrqst(fioAppId, requestee, obtid, memo, requesteeKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, rej);

            // expect the 500 error to be part of the thrown exception
            let expectedResponseCode="500";
            let expectedResponseMessage="Internal Service Error - fc";

            reportrqstJson = rej.json;
            assert(reportrqstJson.code == expectedResponseCode, "reportrqst action response invalid code.");
            assert(reportrqstJson.message == expectedResponseMessage, "reportrqst action response invalid message.");

            reportrqstSuccess = true
        });
    assert(reportrqstSuccess, "reportrqst() with invalid FIO App Id should have failed.");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_invalidId_reportrqst ***")
}

async function test_invalidRequestee_reportrqst(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let requestid   = testContext["requestid"];
    let requestor   = testContext["requestor"];
    let requestee   = testContext["requestee"]
    let requestorKey    = testContext["requestorKey"];
    let requesteeKey    = testContext["requesteeKey"];
    let logLevel    = testContext["logLevel"];
    let chain   = testContext["chain"];
    let asset   = testContext["asset"];
    let quantity    =  testContext["quantity"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_invalidRequestee_reportrqst ***")

    fio = new fiofinance.FioFinance();

    let memo = "Request 1";

    // Send the funds request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);

    result = await fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "requestfunds() promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL requestfunds() 1");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "requestfunds() successfull.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 2 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `getPendingRequestByRequestee() successful.`);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);

    let pendingRequest = result[1].rows[result[1].rows.length - 1];

    // retrieve fioappid of latest pending request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, `Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
    let fioAppId = pendingRequest["fioappid"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Rejecting invalid pending request FIO app id: ${fioAppId}`);

    reportrqstSuccess=false
    //approve the request
    memo = "approved";
    let obtid = "0x123456789";
    result = await fio.reportrqst(fioAppId, requestor, obtid, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, rej);

            // expect the 500 error to be part of the thrown exception
            let expectedResponseCode="500";
            let expectedResponseMessage="Internal Service Error - fc";

            reportrqstJson = rej.json;
            assert(reportrqstJson.code == expectedResponseCode, "reportrqst action response invalid code.");
            assert(reportrqstJson.message == expectedResponseMessage, "reportrqst action response invalid message.");

            reportrqstSuccess = true
        });
    assert(reportrqstSuccess, "reportrqst() with invalid FIO App Id should have failed.");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_invalidRequestee_reportrqst ***")
}


async function test_cancelrqst(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let requestid   = testContext["requestid"];
    let requestor   = testContext["requestor"];
    let requestee   = testContext["requestee"]
    let requestorKey    = testContext["requestorKey"];
    let requesteeKey    = testContext["requesteeKey"];
    let logLevel    = testContext["logLevel"];
    let chain   = testContext["chain"];
    let asset   = testContext["asset"];
    let quantity    =  testContext["quantity"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_cancelrqst. Create request, cancel request. ***")

    fio = new fiofinance.FioFinance();

    //send a new request
    let memo = "Request 3";

    let result = await fio.getPendingRequestByRequestee(requestee, 100)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 1 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "getPendingRequestByRequestee() successful.");
    let rowsCount = result[1].rows.length;
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Starting pending requests count for requestee ${requestee}: ${rowsCount}`);

    // Send the funds request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);
    result = await fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "requestfunds() promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL requestfunds(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "requestfunds() successfull.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 2 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "getPendingRequestByRequestee() successful.");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
    assert.strictEqual(rowsCount + 1, result[1].rows.length);
    let pendingRequest = result[1].rows[result[1].rows.length - 1];
    let fioAppId = pendingRequest["fioappid"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Canceling pending request FIO app id: ${fioAppId}`);

    // Cancel the request
    memo = "cancelled";
    result = await fio.cancelrqst(requestid, requestor, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "cancelrqst promise rejection handler.");
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, rej);
            throw rej;
        });
    assert(result[0], "FAIL cancelrqst(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "cancelrqst() successful.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 3 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "getPendingRequestByRequestee() successful.");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Current pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
    assert.strictEqual(rowsCount, result[1].rows.length);

    if (result[1].rows.length > 0) { // Ensure rejected pending request isn't in the pending requests table
        pendingRequest = result[1].rows[result[1].rows.length - 1];
        // console.log(`Pending request: ${JSON.stringify(pendingRequest, null, 2)}`);
        let newFioAppId = pendingRequest["fioappid"];
        // console.log(`New pending request FIO app id: ${newFioAppId}`);
        assert.notStrictEqual(fioAppId, newFioAppId);
    }

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_cancelrqst. ***")
}

async function test_invalidId_cancelrqst(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let requestid   = testContext["requestid"];
    let requestor   = testContext["requestor"];
    let requestee   = testContext["requestee"]
    let requestorKey    = testContext["requestorKey"];
    let requesteeKey    = testContext["requesteeKey"];
    let logLevel    = testContext["logLevel"];
    let chain   = testContext["chain"];
    let asset   = testContext["asset"];
    let quantity    =  testContext["quantity"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_invalidId_cancelrqst. Create request, cancel request. ***")

    fio = new fiofinance.FioFinance();

    //send a new request
    let memo = "Request 3";

    // Send the funds request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);
    let result = await fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "requestfunds() promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL requestfunds(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "requestfunds() successfull.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 2 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "getPendingRequestByRequestee() successful.");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
    let rowsCount = result[1].rows.length;
    let pendingRequest = result[1].rows[result[1].rows.length - 1];
    let fioAppId = pendingRequest["fioappid"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Canceling pending request FIO app id: ${fioAppId}`);

    // Cancel the request
    memo = "cancelled";
    let invalid_requestid=requestid + 100;
    cancelrqstSuccess=false
    result = await fio.cancelrqst(invalid_requestid, requestor, memo, requestorKey)
        .catch(rej => {
            // console.log(rej)
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, rej);

            // expect the 500 error to be part of the thrown exception
            let expectedResponseCode="500";
            let expectedResponseMessage="Internal Service Error - fc";

            cancelrqstJson = rej.json;
            assert(cancelrqstJson.code == expectedResponseCode, "cancelrqst action response invalid code.");
            assert(cancelrqstJson.message == expectedResponseMessage, "cancelrqst action response invalid message.");

            cancelrqstSuccess = true
        });
    assert(cancelrqstSuccess, "cancelrqst() with invalid FIO App Id should have failed.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 3 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "getPendingRequestByRequestee() successful.");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Current pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
    assert.strictEqual(rowsCount, result[1].rows.length);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_invalidId_cancelrqst. ***")
}

async function test_invalidRequestee_cancelrqst(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let requestid   = testContext["requestid"];
    let requestor   = testContext["requestor"];
    let requestee   = testContext["requestee"]
    let requestorKey    = testContext["requestorKey"];
    let requesteeKey    = testContext["requesteeKey"];
    let logLevel    = testContext["logLevel"];
    let chain   = testContext["chain"];
    let asset   = testContext["asset"];
    let quantity    =  testContext["quantity"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_invalidRequestee_cancelrqst. Create request, cancel request. ***")

    fio = new fiofinance.FioFinance();

    //send a new request
    let memo = "Request 3";

    // Send the funds request
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Creating funds request. requestor ${requestor}, requestee: ${requestee}, chain: ${chain}, asset: ${asset}, quantity:${quantity}, memo:${memo}`);
    let result = await fio.requestfunds(requestid, requestor, requestee, chain, asset, quantity, memo, requestorKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "requestfunds() promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL requestfunds(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "requestfunds() successfull.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 2 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "getPendingRequestByRequestee() successful.");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
    let rowsCount = result[1].rows.length;
    let pendingRequest = result[1].rows[result[1].rows.length - 1];
    let fioAppId = pendingRequest["fioappid"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Canceling pending request FIO app id: ${fioAppId}`);

    // Cancel the request
    memo = "cancelled";
    cancelrqstSuccess=false
    result = await fio.cancelrqst(requestid, requestee, memo, requesteeKey)
        .catch(rej => {
            // console.log(rej)
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, rej);

            // expect the 500 error to be part of the thrown exception
            let expectedResponseCode="500";
            let expectedResponseMessage="Internal Service Error - fc";

            cancelrqstJson = rej.json;
            assert(cancelrqstJson.code == expectedResponseCode, "cancelrqst action response invalid code.");
            assert(cancelrqstJson.message == expectedResponseMessage, "cancelrqst action response invalid message.");

            cancelrqstSuccess = true
        });
    assert(cancelrqstSuccess, "cancelrqst() with invalid FIO App Id should have failed.");

    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    result = await fio.getPendingRequestByRequestee(requestee, 100) // expected max 100 pending requests in test setup
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "getPendingRequestByRequestee() 3 promise rejection handler.");
            throw rej;
        });
    assert(result[0], "FAIL getPendingRequestByRequestee(). Response: " + result[1]);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, "getPendingRequestByRequestee() successful.");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Current pending requests count for requestee ${requestee}: ${result[1].rows.length}`);
    assert.strictEqual(rowsCount, result[1].rows.length);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_invalidRequestee_cancelrqst. ***")
}


// test function
async function testFunction(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let logLevel    = testContext["logLevel"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START tests ***");

    // shallow clone context object
    let testContext_1 = JSON.parse(JSON.stringify(testContext));

    await test_requestfunds(testContext);

    testContext_1["requestid"] += testContext_1["requestid"]
    await test_rejectrqst(testContext_1);

    testContext_1["requestid"] += testContext_1["requestid"]
    await test_invalidId_rejectrqst(testContext_1);

    testContext_1["requestid"] += testContext_1["requestid"]
    await test_invalidRequestee_rejectrqst(testContext_1);

    testContext_1["requestid"] += testContext_1["requestid"]
    // requestee = [requestor, requestor = requestee][0];  // swap values
    // requesteeKey = [requestorKey, requestorKey = requesteeKey][0];  // swap values
    await test_reportrqst(testContext_1);

    testContext_1["requestid"] += testContext_1["requestid"]
    await test_invalidId_reportrqst(testContext_1)

    testContext_1["requestid"] += testContext_1["requestid"]
    await test_invalidRequestee_reportrqst(testContext_1)
    //
    testContext_1["requestid"] += testContext_1["requestid"]
    await test_cancelrqst(testContext_1);

    testContext_1["requestid"] += testContext_1["requestid"]
    await test_invalidId_cancelrqst(testContext_1)

    testContext_1["requestid"] += testContext_1["requestid"]
    await test_invalidRequestee_cancelrqst(testContext_1)

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END tests ***")
}

async function setContract(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let logLevel    = testContext["logLevel"]

    let contract="fio.finance";
    let contractDir=contract;
    let wasmFile= contract + ".wasm";
    let abiFile= contract + ".abi";
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Setting contract: ${contract}`);
    result = await fiocommon.Helper.setContract(fiocommon.Config.FioFinanceAccount, "contracts/"+contractDir, wasmFile, abiFile)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "Helper.setContract() promise rejection handler.");
            throw rej;
        });
    if (!result[0]) {
        fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "ERROR: Helper.setContract() failed.");
        return [false, result[1]];
    }
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Contract ${contract} set successfully.`);
}

async function main() {
    let requestid=9000
    let requestor='fioname11111'
    let requestee='fioname22222'
    let requestorKey="5K2HBexbraViJLQUJVJqZc42A8dxkouCmzMamdrZsLHhUHv77jF"
    let requesteeKey="5JA5zQkg1S59swPzY6d29gsfNhNPVCb7XhiGJAakGFa7tEKSMjT"
    let args = minimist(process.argv.slice(2), {
        alias: {
            h: 'help',
            d: 'debug',
            l: 'leaverunning',
            i: 'requestid',
            s: 'requestor',
            r: 'requestee',
            k: 'requestorkey',
            e: 'requesteekey'
        },
        default: {
            help: false,
            debug: fiocommon.Config.LogLevelError,
            leaverunning: false,
            requestid: requestid,
            requestor: requestor,
            requestee: requestee,
            requestorKey: requestorKey,
            requesteeKey: requesteeKey
        },
    });

    if (args.help) {
        console.log(`arguments:\n`+
            `\t[-h|--help]\n`+
            `\t[-d|--debug] <1-5> default: ${fiocommon.Config.LogLevelError}\n`+
            `\t[-l|--leaverunning]\n`+
            `\t[-i|--requestid <requestid default ${requestid}>]\n`+
            `\t[-s|--requestor <requestor default ${requestor}>]\n`+
            `\t[-r|--requestee <requestid default ${requestee}>]\n`+
            `\t[-k|--requestorkey <requestor private key>]\n`+
            `\t[-e|--requesteekey <requestid private key>]`);
        return 0;
    }

    let logLevel = args.debug
    fiocommon.Config.LogLevel = logLevel;

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Owner account ${args.creator}`);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Leave Running ${args.leaverunning}`);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Debug ${args.debug}`);

    fiocommon.Helper.Log(logLevel > fiocommon.Config.LogLevelInfo, "Enter main sync");
    try {
        fiocommon.Helper.Log(logLevel > fiocommon.Config.LogLevelInfo, "Start startup()");

        fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "Standing up blockchain and wallet.");
        let result = await fiocommon.Helper.startup()
            .catch(rej => {
                fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "startup() promise rejection handler.");
                throw rej;
            });
        if (!result[0]) {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "ERROR: startup() failed.");
            return [false, result[1]];
        }
        fiocommon.Helper.Log(logLevel > fiocommon.Config.LogLevelInfo, "End startup()");
        fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "nodeos successfully configured.");

        var testContext = {
            "requestid": args.requestid,
            "requestor": args.requestor,
            "requestee": args.requestee,
            "requestorKey": args.requestorKey,
            "requesteeKey": args.requesteeKey,
            "logLevel": logLevel,
            "chain": "FIO",
            "asset" : "FIO",
            "quantity" : "10.0000"
        };

        await setContract(testContext)
            .catch(rej => {
                fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "setContract() promise rejection handler.");
                throw rej;
            });

        await testFunction(testContext)
            .catch(rej => {
                fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, "testFunction() promise rejection handler.");
                throw rej;
            });

        return [true, ""];
    } finally {
        if (!args.leaverunning) {
            fiocommon.Helper.sleep(100);    // allow time for logs to be updated before shutdown
            fiocommon.Helper.Log(logLevel > fiocommon.Config.LogLevelInfo, "start shutdown()");
            await fiocommon.Helper.shutdown().catch(rej => {
                fiocommon.Helper.Log(logLevel > fiocommon.Config.LogLevelInfo, `shutdown promise rejection handler: ${rej}`);
                throw rej;
            });
            fiocommon.Helper.Log(logLevel > fiocommon.Config.LogLevelInfo, "End shutdown()");
        }
    }
    fiocommon.Helper.Log(logLevel > fiocommon.Config.LologLevelgLevelInfo, "End main sync");

    fiocommon.Helper.Log(logLevel > fiocommon.Config.LogLevelInfo, "Exit main.");
}

main()
    .then(text => {
        return 0;
    })
    .catch(err => {
        console.error('Caught exception in main: ' + err, err.stack);
        // Deal with the fact the chain failed
    });