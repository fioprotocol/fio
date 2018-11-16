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

async function startup() {
    if (fiocommon.Config.LogLevel > 4) console.log("Enter startup().");

    let result = await fiocommon.Helper.execute("tests/startupNodeos.py", false)
        .catch(rej => {
            console.error(`Helper.execute() promise rejection handler.`);
            throw rej;
        });

    if (fiocommon.Config.LogLevel > 4) console.log("Exit startup()");
    return [true, result];
}

async function shutdown() {
    try {
        let result = await fiocommon.Helper.execute("/usr/bin/pkill -9 nodeos", false);
        result = await fiocommon.Helper.execute("/usr/bin/pkill -9 keosd", false);
        return [true, result];
    } catch (e) {
        console.error("Helper.execute() threw exception");
        throw e;
    }
}

async function testFunction(creator) {
    fiocommon.Helper.checkTypes( arguments, ['string'] );

    console.log("*** START tests ***");
    fio = new fioname.Fio();

    console.log("create a new account");
    let createAccountResult = await fio.createAccount(fiocommon.Config.SystemAccount)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: createAccount()");
        });
    assert(createAccountResult[0], "FAIL: createAccount()");

    let account =                   createAccountResult[2];
    let accountActivePrivateKey =   createAccountResult[4][0];

    console.log(`validate account ${account} exists`);
    let getAccountResult = await fio.getAccount(account).catch(rej => {
        console.error(rej.stack);
        assert(true, "EXCEPTION: getAccount()");
    });
    assert(getAccountResult[0], "FAIL getAccount() " + account);

    let quantity= "200.0000 FIO";
    console.log(`Give new account ${account} some(${quantity}) funds.`);
    let transferResult = await fio.transfer(fiocommon.Config.SystemAccount, account, quantity, "initial transfer")
        .catch(rej => {
            console.error(rej.stack);
            assert(true, "transfer() failed");
        });
    assert(transferResult[0], "transfer() failed");


    console.log(`Get currency balance prior to register domain for account "${account}".`);
    let getCurrencyBalanceResult = await fio.getCurrencyBalance(account)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: getCurrencyBalance(), account: "+ account);
        });
    assert(getAccountResult[0], "FAIL getCurrencyBalance(), account: "+ account);
    let originalBalance =  parseFloat(getCurrencyBalanceResult[1][0].split(" "));
    if (fiocommon.Config.LogLevel > 3) { console.log(`Original balance: ${originalBalance}`); }

    let domain="amzn";
    console.log(`create domain "${domain}"`);
    let registerNameResult = await fio.registerName(domain, account, accountActivePrivateKey)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: registerName() " + domain);
        });
    assert(registerNameResult[0], "FAIL: registerName() " + domain);
    await fiocommon.Helper.sleep(fiocommon.Config.FinalizationTime);    // approximate time to finalize transaction

    console.log(`validate domain "${domain}" exists.`);
    getAccountResult = null;
    getAccountResult =  await fio.lookupByName(domain)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: lookupByName() "+ domain);
        });
    assert(getAccountResult[0], "FAIL lookupByName() " + domain);
    if (fiocommon.Config.LogLevel > 3) {
        console.log(JSON.stringify(getAccountResult[1], null, 2));
        console.log(`is_registered: ${getAccountResult[1].is_registered}`);
    }
    assert(getAccountResult[1].is_registered == "true", "Didn't find domain : " + domain);
    assert(getAccountResult[1].is_domain == "true", "Didn't tag domain properly. Expected: true");
    assert(!getAccountResult[1].address, "Address expected to be empty.");
    assert(getAccountResult[1].expiration, "Expiration should not be empty.");

    console.log(`Get currency balance after register domain for account "${account}".`)
    getCurrencyBalanceResult = await fio.getCurrencyBalance(account)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: getCurrencyBalance(), account: "+ account);
        });
    assert(getAccountResult[0], "FAIL getCurrencyBalance(), account: "+ account);
    let newBalance =  parseFloat(getCurrencyBalanceResult[1][0].split(" "));
    if (fiocommon.Config.LogLevel > 3) { console.log(`New balance: ${newBalance}`); }

    let actualPayment = originalBalance - newBalance;
    let expectedPayment = (fiocommon.Config.pmtson ? fiocommon.TrxFee.domregiter : 0.0);
    console.log(`Register domain payment validation. Expected: ${expectedPayment}, Actual: ${actualPayment}`);
    assert(Math.abs(actualPayment - expectedPayment) <= 0.01, `Invalid register domain payment. Expected: ${expectedPayment}, Actual: ${actualPayment}`);

    // Check for invalid domain doesn't exist.
    let invalidDomain="junk";
    console.log(`validate invalid domain "${invalidDomain}" doesn't exists.`);
    getAccountResult = null;
    getAccountResult = await fio.lookupByName(invalidDomain)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: lookupByName() "+ invalidDomain);
        })
    assert(getAccountResult[0], "FAIL lookupByName() " + invalidDomain);
    if (fiocommon.Config.LogLevel > 3) {
        console.log(JSON.stringify(getAccountResult[1], null, 2));
    }
    assert(getAccountResult[1].is_registered == "false", "Found invalid domain: " + invalidDomain);
    assert(getAccountResult[1].is_domain == "false", "Didn't tag domain properly. Expected: true");
    assert(!getAccountResult[1].address, "Address expected to be empty.");
    assert(!getAccountResult[1].expiration, "Expiration should be empty.");

    console.log(`Get currency balance prior to register name for account "${account}".`);
    getCurrencyBalanceResult = await fio.getCurrencyBalance(account)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: getCurrencyBalance(), account: "+ account);
        });
    assert(getAccountResult[0], "FAIL getCurrencyBalance(), account: "+ account);
    originalBalance =  parseFloat(getCurrencyBalanceResult[1][0].split(" "));
    if (fiocommon.Config.LogLevel > 3) { console.log(`Original balance: ${originalBalance}`); }

    // create valid FIO name
    let name="dan."+domain;
    registerNameResult = await fio.registerName(name, account, accountActivePrivateKey)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: registerName() " + name);
        });
    assert(registerNameResult[0], "FAIL: registerName() " + name);

    console.log(`validate name "${name}" exists.`);
    getAccountResult = null;
    getAccountResult =  await fio.lookupByName(name)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: lookupByName() "+ name);
        });
    assert(getAccountResult[0], "FAIL lookupByName() " + name);
    if (fiocommon.Config.LogLevel > 3) {
        console.log(JSON.stringify(getAccountResult[1], null, 2));
    }
    assert(getAccountResult[1].is_registered == "true", "Didn't find name : " + name);
    assert(getAccountResult[1].is_domain == "false", "Not a domain. Expected: false");
    assert(!getAccountResult[1].address, "Address expected to be empty.");
    assert(getAccountResult[1].expiration, "Expiration should not be empty.");

    console.log(`Get currency balance after register name for account "${account}".`)
    getCurrencyBalanceResult = await fio.getCurrencyBalance(account)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: getCurrencyBalance(), account: "+ account);
        });
    assert(getAccountResult[0], "FAIL getCurrencyBalance(), account: "+ account);
    newBalance =  parseFloat(getCurrencyBalanceResult[1][0].split(" "));
    if (fiocommon.Config.LogLevel > 3) { console.log(`New balance: ${newBalance}`); }

    actualPayment = originalBalance - newBalance;
    expectedPayment = (fiocommon.Config.pmtson ? fiocommon.TrxFee.nameregister : 0.0);
    console.log(`Register name payment validation. Expected: ${expectedPayment}, Actual: ${actualPayment}`);
    assert(Math.abs(actualPayment - expectedPayment) <= 0.01, `Invalid register name payment. Expected: ${expectedPayment}, Actual: ${actualPayment}`);


    // Check for invalid name doesn't exist
    let invalidName="junk."+domain;
    console.log(`validate invalid name "${invalidName}" doesn't exists.`);
    getAccountResult = null;
    getAccountResult =  await fio.lookupByName(invalidName)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: lookupByName() "+ invalidName);
        });
    assert(getAccountResult[0], "FAIL lookupByName() " + invalidName);
    if (fiocommon.Config.LogLevel > 3) {
        console.log(JSON.stringify(getAccountResult[1], null, 2));
    }
    assert(getAccountResult[1].is_registered == "false", "Found invalid name: " + name);
    assert(getAccountResult[1].is_domain == "false", "Not a domain. Expected: false");
    assert(!getAccountResult[1].address, "Address expected to be empty.");
    assert(!getAccountResult[1].expiration, "Expiration should be empty.");

    console.log("*** START MAS 54 test ***");
    console.log(`Get currency balance prior to add address for account "${account}".`);
    getCurrencyBalanceResult = await fio.getCurrencyBalance(account)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: getCurrencyBalance(), account: "+ account);
        });
    assert(getAccountResult[0], "FAIL getCurrencyBalance(), account: "+ account);
    originalBalance =  parseFloat(getCurrencyBalanceResult[1][0].split(" "));
    if (fiocommon.Config.LogLevel > 3) { console.log(`Original balance: ${originalBalance}`); }

    let address="0x123456789";
    let chain="btc";
    console.log(`Add address. name: ${name}, address: ${address}, chain: ${chain}`);
    let addAddressResult = await fio.addaddress(name, address, chain, account, accountActivePrivateKey)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: addaddress(), name: "+ name+ ", address: "+ address+ ", chain: "+ chain);
        });
    assert(getAccountResult[0], "FAIL addaddress(), name: "+ name+ ", address: "+ address+ ", chain: "+ chain);

    console.log(`validate address "${address}" is set.`);
    getAccountResult = null;
    getAccountResult =  await fio.lookupByName(name, chain)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: lookupByName() "+ name);
        });
    assert(getAccountResult[0], "FAIL lookupByName() " + name);
    if (fiocommon.Config.LogLevel > 3) {
        console.log(JSON.stringify(getAccountResult[1], null, 2));
    }
    assert(getAccountResult[1].is_registered == "true", "Didn't find name : " + name);
    assert(getAccountResult[1].is_domain == "false", "Not a domain. Expected: false");
    assert(getAccountResult[1].address == address, "Address expected to be "+ address);
    assert(getAccountResult[1].expiration, "Expiration should not be empty.");

    console.log(`Get currency balance after add address for account "${account}".`)
    getCurrencyBalanceResult = await fio.getCurrencyBalance(account)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: getCurrencyBalance(), account: "+ account);
        });
    assert(getAccountResult[0], "FAIL getCurrencyBalance(), account: "+ account);
    newBalance =  parseFloat(getCurrencyBalanceResult[1][0].split(" "));
    if (fiocommon.Config.LogLevel > 3) { console.log(`New balance: ${newBalance}`); }

    actualPayment = originalBalance - newBalance;
    expectedPayment = (fiocommon.Config.pmtson ? fiocommon.TrxFee.upaddress : 0.0);
    console.log(`Add address payment validation. Expected: ${expectedPayment}, Actual: ${actualPayment}`);
    assert(Math.abs(actualPayment - expectedPayment) <= 0.01, `Invalid add address payment. Expected: ${expectedPayment}, Actual: ${actualPayment}`);

    console.log("*** END MAS 54 test ***")

    console.log("*** END tests ***")
    // return [true, ""];
}

async function main() {
    let args = minimist(process.argv.slice(2), {
        alias: {
            h: 'help',
            c: 'creator',
            l: 'leaverunning',
            d: 'debug'
        },
        default: {
            help: false,
            creator: 'fio.system',
            leaverunning: false,
            debug: false
        },
    });

    if (args.help) {
        console.log("arguments: [-h|--help] [-c|--creator <creator account name default fio.system>] [-l|--leaverunning] [-d|--debug]");
        return 0;
    }

    console.log(`Owner account ${args.creator}`);
    console.log(`Leave Running ${args.leaverunning}`);
    console.log(`Debug ${args.debug}`);

    if (args.debug) {
        fiocommon.Config.LogLevel = 5;
    }

    if (fiocommon.Config.LogLevel > 4) console.log("Enter main sync");
    try {
        if (fiocommon.Config.LogLevel > 4) console.log("Start startup()");

        console.log("Standing up nodeos.");
        let result = await startup()
            .catch(rej => {
                console.error(`startup() promise rejection handler.`);
                throw rej;
            });
        if (!result[0]) {
            console.error("ERROR: startup() failed.");
            return [false, result[1]];
        }
        if (fiocommon.Config.LogLevel > 4) console.log("End startup()");
        console.log("nodeos successfully configured.");

        let contract="fio.name";
        let contractDir=contract;
        let wasmFile= contract + ".wasm";
        let abiFile= contract + ".abi";
        console.log(`Setting contract: ${contract}`);
        result = await fiocommon.Helper.setContract(fiocommon.Config.SystemAccount, "contracts/"+contractDir, wasmFile, abiFile)
            .catch(rej => {
                console.error(`Helper.setContract() promise rejection handler.`);
                throw rej;
            });
        if (!result[0]) {
            console.error("ERROR: Helper.setContract() failed.");
            return [false, result[1]];
        }
        console.log("Contract fio.name set successfully.");

        await testFunction(args.creator)
            .catch(rej => {
                console.error(`testFunction() promise rejection handler.`);
                throw rej;
            });

        return [true, ""];
    } finally {
        if (!args.leaverunning) {
            fiocommon.Helper.sleep(100);    // allow time for logs to be updated before shutdown
            if (fiocommon.Config.LogLevel > 4) console.log("start shutdown()");
            console.log("Shuting down nodeos and keosd.");
            await shutdown().catch(rej => {
                if (fiocommon.Config.LogLevel > 4) console.log(`shutdown promise rejection handler: ${rej}`);
                throw rej;
            });
            if (fiocommon.Config.LogLevel > 4) console.log("End shutdown()");
        }
    }
    if (fiocommon.Config.LogLevel > 4) console.log("End main sync");

    if (fiocommon.Config.LogLevel > 4) console.log("Exit main.");
}

main()
    .then(text => {
        return 0;
    })
    .catch(err => {
        console.error('Caught exception in main: ' + err, err.stack);
        // Deal with the fact the chain failed
    });