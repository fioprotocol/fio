 
// define key and create signature
const actor = "exchange1111";
const account = "exchange1111";
const defaultPrivateKey = key;
const server = 'http://52.14.221.174:8889';
const rpc = new eosjs2_jsonrpc.JsonRpc(server);
const signatureProvider = new eosjs2_jssig.default([defaultPrivateKey]);
const api = new eosjs2.Api({ rpc, signatureProvider });


function transfer(var fromacct, var toacct, var qty)
{  
  if (fromacct === "") {
    throw new Error("Null or empty from_account.")
  }
  if (toacct === "") {
    throw new Error("Null or empty to_account.")
  }
  
  // Check if toacct exists, throw error if not
  // TODO
  
  
	if (key === "") {
		throw new Error("Null or empty key.")
	}
	if (qty === "") {
		throw new Error("Null or empty quantity.")
	}

  
	if (GetTotalAssets(fromAcct) < qty) {
		throw new Error("Not enough funds.")
	}

	(async () => {
    try {
      const result = await api.transact({
        actions: [{
            account: 'eosio.token',
            name: 'transfer',
            authorization: [{
                actor: fromacct,
                permission: 'active',
            }],
            data: {
                from: fromacct,
                to: toacct,
                quantity: qty,
                memo: '',
            },
        }]
      }, {
        blocksBehind: 3,
        expireSeconds: 30,
      });
      pre.textContent += '\n\nTransaction pushed!\n\n' + JSON.stringify(result, null, 2);
    } catch (e) {
      pre.textContent = '\nCaught exception: ' + e;
      if (e instanceof eosjs2_jsonrpc.RpcError)
        pre.textContent += '\n\n' + JSON.stringify(e.json, null, 2);
    }
  })();
	  
  
}


function register(var chain, var fio_user_name, var address)
{
  if (fio_user_name === "") {
    throw new Error("Null or empty fio user name.")
  }
  if (chain === "") {
    throw new Error("Null or empty chain.")
  }
  if (address === "") {
    throw new Error("Null or empty address.")
  }
  if (key === "") {
    throw new Error("Null or empty key.")
  }

// Make the transaction and return result
  (async () => {
    try {
      const result = await api.transact({
        actions: [{
            account: account,
            name: 'registername',
            authorization: [{
                actor: actor,
                permission: 'active',
            }],
            data: {
                name: name,
            },
        }]
      }, {
        blocksBehind: 3,
        expireSeconds: 30,
      });
      pre.textContent += '\n\nTransaction pushed!\n\n' + JSON.stringify(result, null, 2);
    } catch (e) {
      pre.textContent = '\nCaught exception: ' + e;
      if (e instanceof eosjs2_jsonrpc.RpcError)
        pre.textContent += '\n\n' + JSON.stringify(e.json, null, 2);
    }
  })();
}


function GetTotalAssets(var account)
{
	
	  if (publicaddress === "") {
    throw new Error("Null or empty address.")
  }
  
	var data = JSON.stringify(false);
	var xhr = new XMLHttpRequest();
	xhr.withCredentials = true;

	xhr.addEventListener("readystatechange", function () {
	  if (this.readyState === this.DONE) {
		console.log(this.responseText); // TODO filter the response text and return the available amount
	  }
	});
	
	var newurl = server + "/v1/chain/get_currency_balance?code=fioio.token&account=" + toacct + "&symbol=FIO";
	xhr.open("POST", newurl);
	
	xhr.send(data);

}

function GetAccount(var account)
{
	
		if (account === "") {
			throw new Error("Null or empty account.")
	}
  
		var data = JSON.stringify({
	  "account_name": account
	});

	var xhr = new XMLHttpRequest();
	xhr.withCredentials = true;

	xhr.addEventListener("readystatechange", function () {
	  if (this.readyState === this.DONE) {
		console.log(this.responseText); //TODO filter the response text and return public key for account owner
	  }
	});
	
	var newurl = server + "/v1/chain/get_account";
	xhr.open("POST", newurl);

	xhr.send(data);

	
}


function GetKeyAccount(var account)
{
	var data = JSON.stringify(false);

	var xhr = new XMLHttpRequest();
	xhr.withCredentials = true;

	xhr.addEventListener("readystatechange", function () {
	  if (this.readyState === this.DONE) {
		console.log(this.responseText);	//filter and return account belonging to the key
	  }
	});

	newurl = server  + "/v1/history/get_key_accounts";
	xhr.open("POST", newurl);

	xhr.send(data);
	
}


function AddAddress(var fio_name, var address, var chain)
{
	if (fio_name === "") {
    throw new Error("Null or empty fioname.")
  }
    if (address === "") {
    throw new Error("Null or empty address.")
  }
    if (chain === "") {
    throw new Error("Null or empty chain.")
  }
	
	(async () => {
    try {
      const result = await api.transact({
        actions: [{
            account: account,
            name: 'addaddress',
            authorization: [{
                actor: actor,
                permission: 'active',
            }],
            data: {
                fio_user_name: fio_name,
                chain: chain,
                address: address
            },
        }]
      }, {
        blocksBehind: 3,
        expireSeconds: 30,
      });
      pre.textContent += '\n\nTransaction pushed!\n\n' + JSON.stringify(result, null, 2);
    } catch (e) {
      pre.textContent = '\nCaught exception: ' + e;
      if (e instanceof eosjs2_jsonrpc.RpcError)
        pre.textContent += '\n\n' + JSON.stringify(e.json, null, 2);
    }
  })();
	
	
}