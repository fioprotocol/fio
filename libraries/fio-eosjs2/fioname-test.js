/** FioName test file implementation for Fio Javascript SDK
 *  Description: For testing account creation, fioname registration and adding addresses  
 *  @author Ciju John
 *  @file fioname-test.js
 *  @copyright Dapix
 *
 *  Changes: 10-8-2018 Adam Androulidakis
 */


const minimist = require('minimist');
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

// test function
async function legacyTests(creator) {
    fiocommon.Helper.checkTypes( arguments, ['string'] );

    console.log("Starting test function.");
    fio = new fioname.Fio();

    fio.createAccount(creator).then(result => {
        if (result[0]) {
            console.log("Create account transaction successful.");
            console.log(`New Account name: ${result[2]}`);
            console.log(`Account owner keys: Private key: ${result[3][0]}, public key: ${result[3][1]}`);
            console.log(`Account active keys: Private key: ${result[4][0]}, public key: ${result[4][1]}`);
            if (fiocommon.Config.LogLevel > 3) {
                console.log(JSON.stringify(result[1], null, 2));
            }
        } else {
            console.error("Create account transaction failed.");
            console.error(JSON.stringify(result[1], null, 2));
        }
    }).catch(rej => {
        console.error(`createAccount promise rejection handler.`);
        throw rej;
    });

    let createAccountResult = await fio.createAccount(fiocommon.Config.SystemAccount)
        .catch(rej => {
            console.error(`createAccount rejection handler.`)
            throw rej;
        });
    if (createAccountResult[0]) {
        console.log("Account creation successful.");
        console.log(`Account name: ${createAccountResult[2]}\nOwner key: ${createAccountResult[3][0]}\nOwner key: ${createAccountResult[3][1]}\n` +
            `Active key: ${createAccountResult[4][0]}\nActive key: ${createAccountResult[4][1]}\n`);
        // console.log(`Account name: ${result[1]}`);

    } else {
        console.error("Account creation failed.");
        console.error(JSON.stringify(createAccountResult[1], null, 2));
        throw new Error("Account creation failed.");
    }

    // async transfer(from, to, quantity, memo) {
    let transferResult = await fio.transfer(fiocommon.Config.SystemAccount, createAccountResult[2], "200.0000 FIO", "initial transfer")
        .catch(rej => {
            console.error(`transfer rejection handler.`)
            throw rej;
        });
    if (transferResult[0]) {
        console.log("transfer successful.");
    } else {
        console.error("transfer failed.");
        throw new Error("Transfer failed.");
    }

    let registerNameResult = await fio.registerName(createAccountResult[2], createAccountResult[2], createAccountResult[4][0])
        .catch(rej => {
            console.error(`registerName domain rejection handler.`)
            throw rej;
        });
    if (registerNameResult[0]) {
        console.log("Domain registration succeeded.");
    } else {
        console.error("Domain registration failed.");
        throw new Error("Domain registration failed.");
        // console.log(JSON.stringify(result[1], null, 2));
    }

    registerNameResult = await fio.registerName("ciju." + createAccountResult[2], createAccountResult[2], createAccountResult[4][0])
        .catch(rej => {
            console.error(`registerName name rejection handler.`)
            throw rej;
        });
    if (registerNameResult[0]) {
        console.log("Name registration succeeded.");
    } else {
        console.error("Name registration failed.");
        throw new Error("Name registration failed.");
        // console.log(JSON.stringify(result[1], null, 2));
    }

    return [true, registerNameResult[1]];

    // TBD: We need to figure out how to sign with non-default private keys
    // api.signatureProvider.availableKeys.push("5JA5zQkg1S59swPzY6d29gsfNhNPVCb7XhiGJAakGFa7tEKSMjT");
    // fio.transfer('exchange1111', 'currency1111', '0.0001 SYS', '').then (result => {

    // fio.transfer(creator, 'currency1111', '0.0001 SYS', '').then (result => {
    //     if (result[0]) {
    //         console.log("Transfer transaction successful.");
    //     } else {
    //         console.log("Transfer transaction failed.");
    //     }
    //     console.log(JSON.stringify(result[1], null, 2));
    // }).catch(rej => {
    //         console.log(`transfer promise rejection handler.`)
    //         console.log(rej);
    //     });

    // fio.getAccount("exchange1111").then (result => {
    //     if (result[0]) {
    //         console.log("Get account transaction successful.");
    //     } else {
    //         console.log("Get account transaction failed.");
    //     }
    //     console.log(JSON.stringify(result[1], null, 2));
    // }).catch(rej => {
    //     console.log(`getAccount promise rejection handler.`);
    //     console.log(rej);
    //     if (rej instanceof FioError) {
    //         rej.details.then(details=> {
    //             console.log("name: "+ details.constructor.name);
    //             console.log("typeof: "+ typeof details);
    //             console.log("Error details: "+ JSON.stringify(details, null, 2));
    //         })
    //             .catch(err=>{
    //                 console.log(`FioError details promise rejection handler.`);
    //                 throw err;
    //             });
    //     }
    // });

    // fio.lookupNameByAddress("abcdefgh","ETH").then(result => {
    //     if (result[0]) {
    //         console.log("Name lookup by address successful.");
    //         console.log(`Resolved name: ${result[1]["name"]}`);
    //     } else {
    //         console.log("Create account transaction failed.");
    //         console.log(JSON.stringify(result[1], null, 2));
    //     }
    // })
    //     .catch(rej => {
    //         console.log(`lookupNameByAddress rejection handler.`)
    //         throw rej;
    //     });
}

asyn function


async function main() {
    let args = minimist(process.argv.slice(2), {
        alias: {
            h: 'help',
            c: 'creator'
        },
        default: {
            help: false,
            creator: 'fio.system'
        },
    });

    if (args.help) {
        console.log("arguments: [-h|--help] [-c|--creator <creator account name default fio.system>]");
        return 0;
    }

    console.log(`Owner account ${args.creator}`);

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

        console.log("Setting contract: fio.name");
        result = await fiocommon.Helper.setContract(fiocommon.Config.SystemAccount, "contracts/fio.name", "fio.name.wasm", "fio.name.abi")
            .catch(rej => {
                console.error(`Helper.setContract() promise rejection handler.`);
                throw rej;
            });
        if (!result[0]) {
            console.error("ERROR: Helper.setContract() failed.");
            return [false, result[1]];
        }
        console.log("Contract fio.name set successfully.");

        console.log("Running test function");
        result = await legacyTests(args.creator)
            .catch(rej => {
                console.error(`Helper.legacyTests() promise rejection handler.`);
                throw rej;
            });
        if (!result[0]) {
            console.error("ERROR: Helper.legacyTests() failed.");
            return [false, result[1]];
        }
        console.log("Successfully run legacyTests function.");

        return [true, result[1]];
    } finally {
        if (fiocommon.Config.LogLevel > 4) console.log("start shutdown()");
        console.log("Shuting down nodeos and keosd.");
        await shutdown().catch(rej => {
            if (fiocommon.Config.LogLevel > 4) console.log(`shutdown promise rejection handler: ${rej}`);
            throw rej;
        });
        if (fiocommon.Config.LogLevel > 4) console.log("End shutdown()");
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