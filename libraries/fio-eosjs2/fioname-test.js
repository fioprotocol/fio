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

} catch (err) {
    console.log('Caught exception in main: ' + err);
}
