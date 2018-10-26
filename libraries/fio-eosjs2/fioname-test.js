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

// main function
try {
    let args = minimist(process.argv.slice(2), {
        alias: {
            h: 'help',
            c: 'creator'
        },
        default: {
            help: false,
            creator: 'fioname11111'
        },
    });

    if (args.help) {
        console.log("arguments: [-h|--help] [-c|--creator <creator account name default fioname11111>]");
        return 0;
    }

    fio = new fioname.Fio();

    let creator=args.creator;
    console.log(`Owner account ${creator}`);

    fio.createAccount(creator).then (result => {
        if (result[0]) {
            console.log("Create account transaction successful.");
            console.log(`New Account name: ${result[2]}`);
            console.log(`Account owner keys: Private key: ${result[3][0]}, public key: ${result[3][1]}`);
            console.log(`Account active keys: Private key: ${result[4][0]}, public key: ${result[4][1]}`);
            if (fiocommon.Config.LogLevel > 3){
                console.log(JSON.stringify(result[1], null, 2));
            }
        } else {
            console.log("Create account transaction failed.");
            console.log(JSON.stringify(result[1], null, 2));
        }
    }).catch(rej => {
        console.log(`createAccount promise rejection handler.`);
        console.log(rej);
    });

        let createAccountResult = await fio.createAccount(Config.SystemAccount)
            .catch(rej => {
                console.log(`createAccount rejection handler.`)
                throw rej;
            });
        if (createAccountResult[0]) {
            console.log("Account creation successful.");
            console.log(`Account name: ${createAccountResult[2]}\nOwner key: ${createAccountResult[3][0]}\nOwner key: ${createAccountResult[3][1]}\n` +
                `Active key: ${createAccountResult[4][0]}\nActive key: ${createAccountResult[4][1]}\n`);
            // console.log(`Account name: ${result[1]}`);

        } else {
            console.log("Account creation failed.");
            console.log(JSON.stringify(createAccountResult[1], null, 2));
            return -1;
        }

        // async transfer(from, to, quantity, memo) {
        let transferResult = await fio.transfer(Config.SystemAccount, createAccountResult[2], "200.0000 FIO", "initial transfer")
            .catch(rej => {
                console.log(`transfer rejection handler.`)
                throw rej;
            });
        if (transferResult[0]) {
            console.log("transfer successful.");
        } else {
            console.log("transfer failed.");
            return -1;
        }

        let registerNameResult = await fio.registerName(createAccountResult[2], createAccountResult[2], createAccountResult[4][0])
            .catch(rej => {
                console.log(`registerName domain rejection handler.`)
                throw rej;
            });
        if (registerNameResult[0]) {
            console.log("Domain registration succeeded.");
        } else {
            console.log("Domain registration failed.");
            return -1;
            // console.log(JSON.stringify(result[1], null, 2));
        }

        registerNameResult = await fio.registerName("ciju."+createAccountResult[2], createAccountResult[2], createAccountResult[4][0])
            .catch(rej => {
                console.log(`registerName name rejection handler.`)
                throw rej;
            });
        if (registerNameResult[0]) {
            console.log("Name registration succeeded.");
        } else {
            console.log("Name registration failed.");
            return -1;
            // console.log(JSON.stringify(result[1], null, 2));
        }

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

} catch (err) {
    console.log('Caught exception in main: ' + err);
}
