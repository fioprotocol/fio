/** FioName test file implementation for Fio Javascript SDK
 *  Description: For testing account creation, fioname registration and adding addresses  
 *  @author Ciju John
 *  @file fioname-test.js
 *  @copyright Dapix
 *
 *  Changes: 10-8-2018 Adam Androulidakis
 */


const minimist = require('minimist');
const assert = require('assert');

const fioname=require('./fioname.js');
const fiocommon=require('./fio.common.js');


async function testCreateAccount(testContext) {
    fiocommon.Helper.checkTypes(arguments, ['object']);

    let logLevel = testContext["logLevel"];

    fio = new fioname.Fio();

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "Test create account happy path");
    let createAccountResult = await fio.createAccount(fiocommon.Config.SystemAccount)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: createAccount()");
        });
    assert(createAccountResult[0], "FAIL: createAccount()");

    let account =                   createAccountResult[2];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `validate account ${account} exists`);
    let getAccountResult = await fio.getAccount(account).catch(rej => {
        fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
        assert(true, "EXCEPTION: getAccount()");
    });
    assert(getAccountResult[0], "FAIL getAccount() " + account);
}

async function test_registerName(testContext) {
    fiocommon.Helper.checkTypes(arguments, ['object']);

    let creator = testContext["creator"];
    let logLevel = testContext["logLevel"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_registerName ***");

    fio = new fioname.Fio();

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "Create an account.");
    let createAccountResult = await fio.createAccount(fiocommon.Config.SystemAccount)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: createAccount()");
        });
    let account =                   createAccountResult[2];
    let accountActivePrivateKey =   createAccountResult[4][0];

    let quantity= "200.0000 FIO";
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Give new account ${account} some(${quantity}) funds.`);
    let transferResult = await fio.transfer(fiocommon.Config.SystemAccount, account, quantity, "initial transfer")
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(true, "transfer() failed");
        });
    assert(transferResult[0], "transfer() failed");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Get currency balance prior to register domain for account "${account}".`);
    let getCurrencyBalanceResult = await fio.getCurrencyBalance(account)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: getCurrencyBalance(), account: "+ account);
        });
    assert(getCurrencyBalanceResult[0], "FAIL getCurrencyBalance(), account: "+ account);
    let originalBalance =  parseFloat(getCurrencyBalanceResult[1][0].split(" "));
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Original balance: ${originalBalance}`);

    let domain="amzn";
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `create domain "${domain}"`);
    let registerNameResult = await fio.registerName(domain, account, accountActivePrivateKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: registerName() " + domain);
        });
    assert(registerNameResult[0], "FAIL: registerName() " + domain);
    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `validate domain "${domain}" exists.`);
    getAccountResult = null;
    getAccountResult =  await fio.lookupByName(domain)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: lookupByName() "+ domain);
        });
    assert(getAccountResult[0], "FAIL lookupByName() " + domain);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, JSON.stringify(getAccountResult[1], null, 2));
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `is_registered: ${getAccountResult[1].is_registered}`);

    assert(getAccountResult[1].is_registered == "true", "Didn't find domain : " + domain);
    assert(getAccountResult[1].is_domain == "true", "Didn't tag domain properly. Expected: true");
    assert(!getAccountResult[1].address, "Address expected to be empty.");
    assert(getAccountResult[1].expiration, "Expiration should not be empty.");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START MAS 111 tests ***");
    let actionResponseStr = registerNameResult[1].processed.action_traces[0].receipt.response;
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "Validate registername response.");
    assert(actionResponseStr, "registername action response cannot be empty.");
    let actionResponseJson = JSON.parse(actionResponseStr);
    assert(actionResponseJson.status == "OK", "registername action response status should be 'OK'.");
    assert(actionResponseJson.fio_name == domain, `registername action response 'fio_name' should be ${domain}.`);

    let actualExipration = actionResponseJson.expiration;
    let expectedExpiration = Math.floor(Date.now() / 1000) + fiocommon.Config.NameRegisterExpiration;
    // check if the expected and actul expiration are within a 5 minute range
    assert(Math.abs(expectedExpiration - actualExipration) <= 300, `Invalid registername expiration. Expected: ${expectedExpiration}, Actual: ${actualExipration}`);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END MAS 111 tests ***");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Get currency balance after register domain for account "${account}".`);
    getCurrencyBalanceResult = await fio.getCurrencyBalance(account)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: getCurrencyBalance(), account: "+ account);
        });
    assert(getAccountResult[0], "FAIL getCurrencyBalance(), account: "+ account);
    let newBalance =  parseFloat(getCurrencyBalanceResult[1][0].split(" "));
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New balance: ${newBalance}`);

    let actualPayment = originalBalance - newBalance;
    let expectedPayment = (fiocommon.Config.pmtson ? fiocommon.TrxFee.domregiter : 0.0);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Register domain payment validation. Expected: ${expectedPayment}, Actual: ${actualPayment}`);
    assert(Math.abs(actualPayment - expectedPayment) <= 0.01, `Invalid register domain payment. Expected: ${expectedPayment}, Actual: ${actualPayment}`);

    // Check for invalid domain doesn't exist.
    let invalidDomain="junk";
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `validate invalid domain "${invalidDomain}" doesn't exists.`);
    getAccountResult = null;
    getAccountResult = await fio.lookupByName(invalidDomain)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: lookupByName() "+ invalidDomain);
        });
    assert(getAccountResult[0], "FAIL lookupByName() " + invalidDomain);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, JSON.stringify(getAccountResult[1], null, 2));

    assert(getAccountResult[1].is_registered == "false", "Found invalid domain: " + invalidDomain);
    assert(getAccountResult[1].is_domain == "false", "Didn't tag domain properly. Expected: true");
    assert(!getAccountResult[1].address, "Address expected to be empty.");
    assert(!getAccountResult[1].expiration, "Expiration should be empty.");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Get currency balance prior to register name for account "${account}".`);
    getCurrencyBalanceResult = await fio.getCurrencyBalance(account)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: getCurrencyBalance(), account: "+ account);
        });
    assert(getAccountResult[0], "FAIL getCurrencyBalance(), account: "+ account);
    originalBalance =  parseFloat(getCurrencyBalanceResult[1][0].split(" "));
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Original balance: ${originalBalance}`);

    // create valid FIO name
    let name="dan."+domain;
    registerNameResult = await fio.registerName(name, account, accountActivePrivateKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: registerName() " + name);
        });
    assert(registerNameResult[0], "FAIL: registerName() " + name);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `validate name "${name}" exists.`);
    getAccountResult = null;
    getAccountResult =  await fio.lookupByName(name)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: lookupByName() "+ name);
        });
    assert(getAccountResult[0], "FAIL lookupByName() " + name);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, JSON.stringify(getAccountResult[1], null, 2));

    assert(getAccountResult[1].is_registered == "true", "Didn't find name : " + name);
    assert(getAccountResult[1].is_domain == "false", "Not a domain. Expected: false");
    assert(!getAccountResult[1].address, "Address expected to be empty.");
    assert(getAccountResult[1].expiration, "Expiration should not be empty.");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START MAS 111 tests ***");
    actionResponseStr = registerNameResult[1].processed.action_traces[0].receipt.response;
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "Validate registername response.");
    assert(actionResponseStr, "registername action response cannot be empty.");
    actionResponseJson = JSON.parse(actionResponseStr);
    assert(actionResponseJson.status == "OK", "registername action response status should be 'OK'.");
    assert(actionResponseJson.fio_name == name, `registername action response 'fio_name' should be ${name}.`);

    actualExipration = actionResponseJson.expiration;
    expectedExpiration = Math.floor(Date.now() / 1000) + fiocommon.Config.NameRegisterExpiration;
    // check if the expected and actul expiration are within a 5 minute range
    assert(Math.abs(expectedExpiration - actualExipration) <= 300, `Invalid registername expiration. Expected: ${expectedExpiration}, Actual: ${actualExipration}`);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END MAS 111 tests ***");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Get currency balance after register name for account "${account}".`);
    getCurrencyBalanceResult = await fio.getCurrencyBalance(account)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: getCurrencyBalance(), account: "+ account);
        });
    assert(getAccountResult[0], "FAIL getCurrencyBalance(), account: "+ account);
    newBalance =  parseFloat(getCurrencyBalanceResult[1][0].split(" "));
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New balance: ${newBalance}`);

    actualPayment = originalBalance - newBalance;
    expectedPayment = (fiocommon.Config.pmtson ? fiocommon.TrxFee.nameregister : 0.0);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Register name payment validation. Expected: ${expectedPayment}, Actual: ${actualPayment}`);
    assert(Math.abs(actualPayment - expectedPayment) <= 0.01, `Invalid register name payment. Expected: ${expectedPayment}, Actual: ${actualPayment}`);


    // Check for invalid name doesn't exist
    let invalidName="junk."+domain;
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `validate invalid name "${invalidName}" doesn't exists.`);
    getAccountResult = null;
    getAccountResult =  await fio.lookupByName(invalidName)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: lookupByName() "+ invalidName);
        });
    assert(getAccountResult[0], "FAIL lookupByName() " + invalidName);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, JSON.stringify(getAccountResult[1], null, 2));

    assert(getAccountResult[1].is_registered == "false", "Found invalid name: " + name);
    assert(getAccountResult[1].is_domain == "false", "Not a domain. Expected: false");
    assert(!getAccountResult[1].address, "Address expected to be empty.");
    assert(!getAccountResult[1].expiration, "Expiration should be empty.");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_registerName ***")
}

async function test_addAddress(testContext) {
    fiocommon.Helper.checkTypes(arguments, ['object']);

    let creator = testContext["creator"];
    let logLevel = testContext["logLevel"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_addAddress ***");

    fio = new fioname.Fio();

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "Create an account.");
    let createAccountResult = await fio.createAccount(fiocommon.Config.SystemAccount)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: createAccount()");
        });
    let account =                   createAccountResult[2];
    let accountActivePrivateKey =   createAccountResult[4][0];

    let quantity= "200.0000 FIO";
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Give new account ${account} some(${quantity}) funds.`);
    let transferResult = await fio.transfer(fiocommon.Config.SystemAccount, account, quantity, "initial transfer")
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(true, "transfer() failed");
        });

    let domain="msft";
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `create domain "${domain}"`);
    let registerNameResult = await fio.registerName(domain, account, accountActivePrivateKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: registerName() " + domain);
        });
    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    let name="dan."+domain;
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `create name "${name}"`);
    registerNameResult = await fio.registerName(name, account, accountActivePrivateKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: registerName() " + name);
        });

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START MAS 54 test ***");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Get currency balance prior to add address for account "${account}".`);
    let getCurrencyBalanceResult = await fio.getCurrencyBalance(account)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: getCurrencyBalance(), account: "+ account);
        });
    assert(getCurrencyBalanceResult[0], "FAIL getCurrencyBalance(), account: "+ account);
    let originalBalance =  parseFloat(getCurrencyBalanceResult[1][0].split(" "));
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `Original balance: ${originalBalance}`);

    let address="0x123456789";
    let chain="btc";
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Add address. name: ${name}, address: ${address}, chain: ${chain}`);
    let addAddressResult = await fio.addaddress(name, address, chain, account, accountActivePrivateKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: addaddress(), name: "+ name+ ", address: "+ address+ ", chain: "+ chain);
        });
    assert(addAddressResult[0], "FAIL addaddress(), name: "+ name+ ", address: "+ address+ ", chain: "+ chain);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `validate address "${address}" is set.`);
    let getAccountResult =  await fio.lookupByName(name, chain)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: lookupByName() "+ name);
        });
    assert(getAccountResult[0], "FAIL lookupByName() " + name);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, JSON.stringify(getAccountResult[1], null, 2));

    assert(getAccountResult[1].is_registered == "true", "Didn't find name : " + name);
    assert(getAccountResult[1].is_domain == "false", "Not a domain. Expected: false");

    // TBD: address storage is currently broken in fioname contract
    // assert(getAccountResult[1].address == address, "Address expected to be "+ address);
    assert(getAccountResult[1].expiration, "Expiration should not be empty.");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Get currency balance after add address for account "${account}".`);
    getCurrencyBalanceResult = await fio.getCurrencyBalance(account)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: getCurrencyBalance(), account: "+ account);
        });
    assert(getCurrencyBalanceResult[0], "FAIL getCurrencyBalance(), account: "+ account);
    let newBalance =  parseFloat(getCurrencyBalanceResult[1][0].split(" "));
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelDebug, `New balance: ${newBalance}`);

    let actualPayment = originalBalance - newBalance;
    let expectedPayment = (fiocommon.Config.pmtson ? fiocommon.TrxFee.upaddress : 0.0);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Add address payment validation. Expected: ${expectedPayment}, Actual: ${actualPayment}`);
    assert(Math.abs(actualPayment - expectedPayment) <= 0.01, `Invalid add address payment. Expected: ${expectedPayment}, Actual: ${actualPayment}`);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END MAS 54 test ***");
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_addAddress ***")
}

async function test_addfiopubadd(testContext) {
    fiocommon.Helper.checkTypes(arguments, ['object']);

    let creator = testContext["creator"];
    let logLevel = testContext["logLevel"];

    fio = new fioname.Fio();

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START test_addfiopubadd ***");
    let pubAddress ="";
    let pub_key ="";
    let addfiopubaddResult;
    let addfiopubaddTestSuccess=false;

    // TBD: 400 error response handling is broken correctly, commenting the test
    // console.log("addfiopubadd: Testing addfiopubadd action with empty public address parameter (expect 400 error response)")
    // pubAddress =" ";
    // pub_key ="xyz";
    // addfiopubaddTestSuccess=false
    // addfiopubaddResult = await fio.addfiopubadd(pubAddress, pub_key, fiocommon.Config.SystemAccount, fiocommon.Config.SystemAccountKey)
    //     .catch(rej => {
    //         if (fiocommon.Config.LogLevel > 4) { console.error(rej); }
    //
    //         // expect the 400 error to be part of the thrown exception
    //         let addfiopubaddStr = rej.json;
    //         let expectedResponseCode="400";
    //         let expectedResponseMessage="UnAuthorized";
    //
    //         addfiopubaddStr = JSON.stringify(addfiopubaddStr)
    //         let addfiopubaddJson = JSON.parse(addfiopubaddStr)
    //         // console.log(JSONify(addfiopubaddJson));
    //
    //         assert(addfiopubaddJson.code == expectedResponseCode, "addfiopubadd action response invalid code.");
    //         assert(addfiopubaddJson.message == expectedResponseMessage, "addfiopubadd action response invalid message.");
    //         addfiopubaddTestSuccess=true;
    //     });
    // assert(addfiopubaddTestSuccess, "addfiopubadd() with empty public address should have failed.");
    //
    // TBD: 400 error response handling is broken correctly, commenting the test
    // console.log("addfiopubadd: Testing addfiopubadd with empty public key parameter (expect 400 error response)")
    // pubAddress ="xyz";
    // pub_key ="";
    // addfiopubaddTestSuccess=false
    // addfiopubaddResult = await fio.addfiopubadd(pubAddress, pub_key, fiocommon.Config.SystemAccount, fiocommon.Config.SystemAccountKey)
    //     .catch(rej => {
    //         if (fiocommon.Config.LogLevel > 4) { console.error(rej); }
    //
    //         // expect the 400 error to be part of the thrown exception
    //         let addfiopubaddStr = rej.json;
    //         let expectedResponseCode="400";
    //         let expectedResponseMessage="UnAuthorized";
    //
    //         addfiopubaddStr = JSON.stringify(addfiopubaddStr)
    //         let addfiopubaddJson = JSON.parse(addfiopubaddStr)
    //         // console.log(JSONify(addfiopubaddJson));
    //
    //         assert(addfiopubaddJson.code == expectedResponseCode, "addfiopubadd action response invalid code.");
    //         assert(addfiopubaddJson.message == expectedResponseMessage, "addfiopubadd action response invalid message.");
    //         addfiopubaddTestSuccess=true;
    //     });
    // assert(addfiopubaddTestSuccess, "addfiopubadd() with empty public key should have failed.");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "create a new account");
    let createAccountResult = await fio.createAccount(fiocommon.Config.SystemAccount)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej.stack);
            assert(false, "EXCEPTION: createAccount()");
        });
    assert(createAccountResult[0], "FAIL: createAccount()");

    let account =                   createAccountResult[2];
    let accountActivePrivateKey =   createAccountResult[4][0];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "addfiopubadd: Testing addfiopubadd action invocation with wrong account permission (expect 401 error response)");
    pubAddress ="abc";
    pub_key ="xyz";
    addfiopubaddTestSuccess=false;
    addfiopubaddResult = await fio.addfiopubadd(pubAddress, pub_key, account, accountActivePrivateKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej);

            // expect the 401 error to be part of the thrown exceptioon
            let addfiopubaddStr = rej.json;
            let expectedResponseCode="401";
            let expectedResponseMessage="UnAuthorized";

            addfiopubaddStr = JSON.stringify(addfiopubaddStr);
            let addfiopubaddJson = JSON.parse(addfiopubaddStr);
            // console.log(JSONify(addfiopubaddJson));

            assert(addfiopubaddJson.code == expectedResponseCode, "addfiopubadd action response invalid code.");
            assert(addfiopubaddJson.message == expectedResponseMessage, "addfiopubadd action response invalid message.");
            addfiopubaddTestSuccess=true;
        });
    assert(addfiopubaddTestSuccess, "addfiopubadd() with wrong signature should have failed.");

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "addfiopubadd: Testing action invocation happy path.");
    pubAddress ="abc";
    pub_key ="xyz";
    addfiopubaddResult = await fio.addfiopubadd(pubAddress, pub_key, fiocommon.Config.SystemAccount, fiocommon.Config.SystemAccountKey)
        .catch(rej => {
            fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelError, rej);
            assert(false, "EXCEPTION: addfiopubadd(), pub address: "+ pubAddress+ ", pub key: "+ pub_key);
        });
    assert(addfiopubaddResult[0], "FAIL addfiopubadd(), pub address: "+ pubAddress+ ", pub key: "+ pub_key);
    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelTrace, JSON.stringify(addfiopubaddResult[1], null, 2));

    assert(addfiopubaddResult[1]["processed"]["action_traces"][0]["receipt"]["response"], "FAIL addfiopubadd(), expected response");
    let responseStr=addfiopubaddResult[1]["processed"]["action_traces"][0]["receipt"]["response"];
    response=JSON.parse(responseStr);
    let expectedResponseStatus="OK";
    let expectedResponsePubKey=pub_key;
    let expectedResponsePubAddress=pubAddress;
    assert(response["status"] == expectedResponseStatus, "FAIL addfiopubadd(), expected response status: "+ expectedResponseStatus);
    assert(response["pub_key"] == expectedResponsePubKey, "FAIL addfiopubadd(), expected response public key: "+ expectedResponsePubKey + ".");
    assert(response["pub_address"] == expectedResponsePubAddress, "FAIL addfiopubadd(), expected response public address: "+ expectedResponsePubKey + ".");

    // // TBD: 400 error response handling is broken correctly, commenting the test
    // console.log("addfiopubadd: Testing duplicate public address failure.")
    // pubAddress ="abc";
    // pub_key ="xyz1";
    // addfiopubaddTestSuccess=false
    // addfiopubaddResult = await fio.addfiopubadd(pubAddress, pub_key, fiocommon.Config.SystemAccount, fiocommon.Config.SystemAccountKey)
    //     .catch(rej => {
    //         console.error(rej);
    //         if (fiocommon.Config.LogLevel > 4) { console.error(rej); }
    //
    //         // expect the 400 error to be part of the thrown exception
    //         let addfiopubaddStr = rej.json;
    //         let expectedResponseCode="400";
    //         // let expectedResponseMessage="UnAuthorized";
    //
    //         addfiopubaddStr = JSON.stringify(addfiopubaddStr)
    //         let addfiopubaddJson = JSON.parse(addfiopubaddStr)
    //         // console.log(JSONify(addfiopubaddJson));
    //
    //         // assert(addfiopubaddJson.code == expectedResponseCode, "addfiopubadd action response invalid code.");
    //         // assert(addfiopubaddJson.message == expectedResponseMessage, "addfiopubadd action response invalid message.");
    //         addfiopubaddTestSuccess=true;
    //     });
    // assert(addfiopubaddTestSuccess, "addfiopubadd() duplicate action invocation should have failed.");

    // TBD validate the address - key mapping has been done.

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END test_addfiopubadd ***")
}

async function testFunction(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let logLevel    = testContext["logLevel"];

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** START tests ***");

    await testCreateAccount(testContext);

    await test_registerName(testContext);

    await test_addAddress(testContext);

    await test_addfiopubadd(testContext);

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, "*** END tests ***")
    // return [true, ""];
}

async function setContract(testContext) {
    fiocommon.Helper.checkTypes( arguments, ['object'] );

    let logLevel    = testContext["logLevel"];

    let contract="fio.name";
    let contractDir=contract;
    let wasmFile= contract + ".wasm";
    let abiFile= contract + ".abi";

    fiocommon.Helper.Log(logLevel >= fiocommon.Config.LogLevelInfo, `Setting contract: ${contract}`);
    result = await fiocommon.Helper.setContract(fiocommon.Config.SystemAccount, "contracts/"+contractDir, wasmFile, abiFile)
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
    fio = new fioname.Fio();

    let args = minimist(process.argv.slice(2), {
        alias: {
            h: 'help',
            d: 'debug',
            l: 'leaverunning',
            c: 'creator'
        },
        default: {
            help: false,
            debug: fiocommon.Config.LogLevelError,
            leaverunning: false,
            creator: 'fio.system'
        },
    });

    if (args.help) {
        console.log(`arguments:\n`+
            `\t[-h|--help]\n`+
            `\t[-d|--debug] <1-5> default: ${fiocommon.Config.LogLevelError}\n`+
            `\t[-l|--leaverunning]\n`+
            `\t[-c|--creator <creator account name default fio.system>]\n`);
        return 0;
    }

    let logLevel = args.debug;
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
            "logLevel": logLevel,
            "creator": args.creator
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
    fiocommon.Helper.Log(logLevel > fiocommon.Config.LogLevelInfo, "End main sync");

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