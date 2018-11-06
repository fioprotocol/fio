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

    let result = await fiocommon.Helper.execute("tests/launcher.py", false)
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

async function test_mas44(creator) {
    fiocommon.Helper.checkTypes( arguments, ['string'] );

    console.log("Start MAS 44 test.");
    fio = new fioname.Fio();

    console.log("create a new account");
    let createAccountResult = await fio.createAccount(fiocommon.Config.SystemAccount)
        .catch(rej => {
            console.error(rej.stack);
            assert(false, "EXCEPTION: createAccount()");
        });
    assert(createAccountResult[0], "FAIL: createAccount()");

    console.log(`validate account ${createAccountResult[2]} exists`);
    let getAccountResult = await fio.getAccount(createAccountResult[2]).catch(rej => {
        console.error(rej.stack);
        assert(true, "EXCEPTION: getAccount()");
    });
    assert(getAccountResult[0], "FAIL getAccount() " + createAccountResult[2]);

    let quantity= "200.0000 FIO";
    console.log(`Give new account ${createAccountResult[2]} some(${quantity}) funds.`);
    let transferResult = await fio.transfer(fiocommon.Config.SystemAccount, createAccountResult[2], quantity, "initial transfer")
        .catch(rej => {
            console.error(rej.stack);
            assert(true, "transfer() failed");
        });
    assert(transferResult[0], "transfer() failed");

    let domain="amzn";
    console.log(`create domain "${domain}"`);
    let registerNameResult = await fio.registerName(domain, createAccountResult[2], createAccountResult[4][0])
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
    assert(!getAccountResult[1].expiration, "Expiration expected to be empty.");

    // Check for domain "junk", which since it doesn't exist should trigger exception
    let condition = false;
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
    assert(!getAccountResult[1].expiration, "Expiration expected to be empty.");

    // // create FIO name dan.amzn
    // let name="dan."+domain;
    // registerNameResult = await fio.registerName(name, createAccountResult[2], createAccountResult[4][0])
    //     .catch(rej => {
    //         console.error(rej.stack);
    //         assert(false, "registerName() 2 failed");
    //     });
    // assert(registerNameResult[0], "registerName() 2 failed");
    //
    // await fio.getAccount(name)
    //     .catch(rej => {
    //         assert(false, "Failed to find name "+ name);
    //     })
    //
    // let invalidName="junk."+domain;
    // condition=false;
    // await fio.getAccount(invalidName)
    //     .catch(rej => {
    //         condition = true;
    //     })
    // assert(condition, "Found unregistered name "+ invalidName);
    //
    // let address="0x123456789";
    // let chain="btc";
    // let addAddressResult = await fio.addaddress(name, address, chain)
    //
    // console.log("End MAS 44 test.");
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

        await test_mas44(args.creator)

        return [true, ""];
    } finally {
        if (!args.leaverunning) {
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