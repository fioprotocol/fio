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

// main function
try {
    let args = minimist(process.argv.slice(2), {
        alias: {
            h: 'help',
			v: 'requestid', 
            s: 'requestor',
			r: 'requestee',
            f: 'fioappid',
            o: 'obtid',
			c: 'chain',
			a: 'asset',
			q: 'quantity',
			m: 'memo'
        },
        default: {
            help: false,
			requestid: '9000',
            requestor: 'user11111111',
			requestee: 'user22222222',
            fioappid: '9000',
            obtid: 'x9000',
			chain: 'FIO',
			asset: 'FIO',
			quantity: '100',
			memo: "Test Memo"
        },
    });

    if (args.help) {
        console.log("arguments: [-h|--help]\n");
        console.log("[-v|--requestid <requestid default 9000>]\n");
        console.log("[-s|--requestor <requestor default user11111111>]\n");
        console.log("[-r|--requestee <requestid default user22222222>]\n");
        console.log("[-f|--fioappid <fioappid default 9000>]\n");
        console.log("[-o|--obtid <obtid default x9000>]\n");
        console.log("[-c|--chain <chain default FIO>]\n");
        console.log("[-a|--asset <asset default FIO>]\n");
        console.log("[-q|--quantity <quantity default 100>]\n");
        console.log("[-m|--memo <memo default Test Memo>]\n");
        return 0;
    }

	fio = new fiofinance.FioFinance();

	let requestid1=args.requestid;
    let requestid2="9001"
    let requestid3="9002"
    let requestor=args.requestor;
	let requestee=args.requestee;
	let chain=args.chain;
	let asset=args.asset;
	let quantity=args.quantity;
	let memo=args.memo;
    let fioappid1="9000";
    let fioappid2="9001";
    let fioappid3="9002";
    let obtid="x9003";
    

    ///////////TEST 1////////////
	
            
    console.log(`Test 1`);
	console.log(`Request ID: ${requestid1}`);
    console.log(`Requestor account: ${requestor}`);
	console.log(`Requestee account: ${requestee}`);
	console.log(`Chain: ${chain}`);
	console.log(`Asset: ${asset}`);
	console.log(`Quantity: ${quantity}`);
	console.log(`Memo:  ${memo}`);
    console.log(`fioappid: ${fioappid1}`);

    
     
	// Send the funds request
    fio.requestfunds(requestid1, requestor, requestee, chain, asset, quantity, memo).then (result => {
        console.log("requestfunds transaction successfully pushed.");
    }).catch(rej => {
        console.log(`requestfunds test 1 promise rejection handler.`);
        console.log(rej);
    });
    
    
    //reject the request
    fio.rejectrqst(fioappid1, requestee, memo).then (result => {
        console.log("rejectrqst transaction successfully pushed.");
    }).catch(rej => {
        console.log(`rejectrqst test 1 promise rejection handler.`);
        console.log(rej);
    });
    
    ///////////TEST 2/////////////
   
              
    console.log(`Test 2`);
	console.log(`Request ID: ${requestid2}`);
    console.log(`Requestor account: ${requestor}`);
	console.log(`Requestee account: ${requestee}`);
	console.log(`Chain: ${chain}`);
	console.log(`Asset: ${asset}`);
	console.log(`Quantity: ${quantity}`);
	console.log(`Memo:  ${memo}`);
    console.log(`fioappid: ${fioappid2}`);
    console.log(`obtid: ${obtid}`); 

    
    //send a new request
    memo = "test2";
    let temp = requestor;
    requestor = requestee;
    requestee = temp;

    //**** TO DO **** How to change private keys to sign the next transaction with ??
    
    fio.requestfunds(requestid2, requestor, requestee, chain, asset, quantity, memo).then (result => {
        console.log("requestfunds transaction successfully pushed.");
    }).catch(rej => {
        console.log(`requestfunds test 2  promise rejection handler.`);
        console.log(rej);
    });
    
    //approve the request
    memo = "approved";
    fio.reportrqst(fioappid2, requestee, obtid, memo).then (result => {
        console.log("reportrqst transaction successfully pushed.");
    }).catch(rej => {
        console.log(`reportrqst test 2 promise rejection handler.`);
        console.log(rej);
    });
    
    
    ///////////TEST 3/////////////
    
 
    console.log(`Test 3`);
	console.log(`Request ID: ${requestid3}`);
    console.log(`Requestor account: ${requestor}`);
	console.log(`Requestee account: ${requestee}`);
	console.log(`Chain: ${chain}`);
	console.log(`Asset: ${asset}`);
	console.log(`Quantity: ${quantity}`);
	console.log(`Memo:  ${memo}`);
    console.log(`fioappid: ${fioappid1}`);
 

 
    temp = requestor;
    requestor = requestee;
    requestee = temp;
    
    //send a new request
        fio.requestfunds(requestid3, requestor, requestee, chain, asset, quantity, "Test 3").then (result => {
        console.log("requestfunds transaction successfully pushed.");
    }).catch(rej => {
        console.log(`requestfunds test 3 promise rejection handler.`);
        console.log(rej);
    });
    
    //Whoops! Cancel the request
    fio.cancelrqst(requestid3, requestor, "cancelled").then (result => {
        console.log("cancelrqst transaction successfully pushed.");
    }).catch(rej => {
        console.log(`cancelrqst test 3 promise rejection handler.`);
        console.log(rej);
    });
    
    
                                                                                                                     
        
} catch (err) {
    console.log('Caught exception in main: ' + err);
}
