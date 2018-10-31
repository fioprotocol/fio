#!/usr/bin/env python3

import re
import os
import subprocess
import random
import string
#import inspect
from datetime import datetime
#from sys import stdout
import shlex
import collections

hostname="localhost"
log_file=open("launcher.detailed.log", "w")

# pylint: disable=too-few-public-methods
class Utils(object):
    Debug=True
    EosClientPath="programs/cleos/cleos"

    @staticmethod
    def checkOutput(cmd):
        assert(isinstance(cmd, list))
        popen=subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (output,error)=popen.communicate()
        if popen.returncode != 0:
            raise subprocess.CalledProcessError(returncode=popen.returncode, cmd=cmd, output=error)
        return output.decode("utf-8")


# SyncStrategy=namedtuple("ChainSyncStrategy", "name id arg")
class Account(object):
    # pylint: disable=too-few-public-methods

    def __init__(self, name):
        self.name=name

        self.ownerPrivateKey=None
        self.ownerPublicKey=None
        self.activePrivateKey=None
        self.activePublicKey=None

    def __str__(self):
        return "Name: %s" % (self.name)


def getAccounts(count):
    """ Static list of account keys. Max count 10."""
    Info=collections.namedtuple("Info", "name ownerPrivate ownerPublic activePrivate activePublic")
    keys=[Info("user1", "5JLxoeRoMDGBbkLdXJjxuh3zHsSS7Lg6Ak9Ft8v8sSdYPkFuABF", "EOS5oBUYbtGTxMS66pPkjC2p8pbA3zCtc8XD4dq9fMut867GRdh82", "5K2HBexbraViJLQUJVJqZc42A8dxkouCmzMamdrZsLHhUHv77jF", "EOS5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY"),
          Info("user2", "5JCpqkvsrCzrAC3YWhx7pnLodr3Wr9dNMULYU8yoUrPRzu269Xz", "EOS7uRvrLVrZCbCM2DtCgUMospqUMnP3JUC1sKHA8zNoF835kJBvN", "5JA5zQkg1S59swPzY6d29gsfNhNPVCb7XhiGJAakGFa7tEKSMjT", "EOS8ApHc48DpXehLznVqMJgMGPAaJoyMbFJbfDLyGQ5QjF7nDPuvJ"),
          Info("user3", "5HvaoRV9QrbbxhLh6zZHqTzesFEG5vusVJGbUazFi5xQvKMMt6U", "EOS8NToQB65dZHv28RXSBBiyMCp55M7FRFw6wf4G3GeRt1VsiknrB", "5KQPk78a4Srm11NnuxGnWGNJuZtGaMErQ31XFDvXwzyS3Ru7AJQ", "EOS8JzoVTmdFCnjs7x2qEq7A4cKgfRatvnohgngUPnZs8XfeFknjL"),
          Info("user4", "5K3s1yePVjrsCJbeyoXGAfJn9yCKKZi6E9oyw59mt5LaJG3Bm1N", "EOS6ekFcLhgeS26uWuEUxzpnj8DB36M43DfF4axAdqBC9y42wP4bZ", "5JqMPkY5AZMYe4LSu1byGPbyjwin9uE9uB3a14WC8ULtYvjaJzE", "EOS8f7yfEhAVajCwCowJEuxJE4djuuVhp8dt9iXcSehJq3z4Zi4fM"),
          Info("user5", "5JAs6QTmEyVjJGbQf1e5MjWLoNqcUAMyZLbLwvDhaG3gYBawDwK", "EOS5TZ4uoBu89S96f2dmBkwG5PsvYxRit3UxfbnVrwJdDFk3S3y2X", "5KcNHxGqm9csmFq8VPa5dGHZ99H8qDGj5F5yWgg6u3D4PUkUoEp", "EOS5n5qzFgoAEwXJhQquKpuHHKhGxpHrabvztSDgDKbrZkFL9fsGs"),
          Info("user6", "5JvWX1RkiaRS7jgMTb9oZsFKy77jJHkzEv78JRFkjxdDQR3d6wN", "EOS7JHQcridAXNWPFsYFCvatJvfrZC8JKUJqWsxjnZbyLVbG89QPL", "5KDDv35PHWGkReThhNsj2j23DCJzvWTLvZbSTUj8BFNicrJXPqG", "EOS7UcoHGbP9p4sVMfQdNDkDdjoNUkrtDjZvCmLBWZ2YNuX788JRA"),
          Info("user7", "5JKQTYUjxWQbWt53z6Prrb3PUNtEiG5GcyXv3yHTK1nJaxzABrk", "EOS7ziyUx4eQ3QGSGw1NwHpXJJ5ctbpfpuhCxt3beSxJ1oqspRqdR", "5JHsotwiGMAjrTu3oCiXYjzJ3dEz9mU7HFc4UigvWgEksbpaVkc", "EOS8D6vbjF1gK9BGqT2LZypsdU2N7dPaEXH7Kiz4zAc164UmVnUUZ"),
          Info("user8", "5K7Dah3CB17j6hpQ5vnBebN2yzGkt5K6A2xV7KshKyAfAUJyY8J", "EOS88CmuS1QFeWwCWNS13dFf64y4ZBoMezQnmbSfQEDiC3n2FqEGp", "5K8dPJgVYWDxfEURh1cvrRzqc7uvDLqNHjxs83Rf2n8Hu7rqhmp", "EOS5C6dQV7xSjHKxWi4A4DYC29psNLWEYfqoWv1tjvo6yBi2HRC3x"),
          Info("user9", "5HvvvAF32bADDzyqcc3kRN4GVKGAZdVFXQexDScJEqTnpk29Kdv", "EOS6YMqC4quk1z4bV6EXrXMzik4SAfeYAscaWmWnhgVSLxSKcqR9Y", "5JDGpznJSJw8cKQYtEuctobAu2zVmD3Rw3fWwPzDuF4XbriseLy", "EOS7uTxpJWW1qS52fyEdf9s2NvqhMcKTKpyuNK7GwffVf6wh3w1Jy"),
          Info("user10", "5JUgV5evcQGJkniK7rH1HckBAvRKsJP1Zgbv39UWnyTBEkMyWMj", "EOS794cEVBvrj9bx1ecV1kszf6UHzQtTeBKvKNoFwT7Dg4MAAWvdV", "5JiSAASb4PZTajemTr4KTgeNBWUxrz6SraHGUvNihSskDA9aY5b", "EOS5Q9AkSnazMPMbEWLSMUkxQWVPhipshJE2qpiFDCZ1awphZQ3VD")]

    assert(count <= len(keys))

    # pylint: disable=redefined-outer-name
    accounts=[]
    for i in range(0, count):
        name=keys[i].name
        account=Account(name)
        account.ownerPrivateKey=keys[i].ownerPrivate
        account.ownerPublicKey=keys[i].ownerPublic
        account.activePrivateKey=keys[i].activePrivate
        account.activePublicKey=keys[i].activePublic
        accounts.append(account)
        if Utils.Debug: print("name: %s, key(owner): ['%s', '%s], key(active): ['%s', '%s']" % (keys[i].name, keys[i].ownerPublic, keys[i].ownerPrivate, keys[i].activePublic, keys[i].activePrivate))

    return accounts

def createAccounts(count):
    """ Dynamically generate accounts info."""
    # pylint: disable=redefined-outer-name
    accounts=[]
    p = re.compile('Private key: (.+)\nPublic key: (.+)\n', re.MULTILINE)
    for _ in range(0, count):
        try:
            cmd="%s create key --to-console" % (Utils.EosClientPath)
            if Utils.Debug: print("cmd: %s" % (cmd))
            keyStr=Utils.checkOutput(cmd.split())
            m=p.search(keyStr)
            if m is None:
                print("ERROR: Owner key creation regex mismatch")
                break

            ownerPrivate=m.group(1)
            ownerPublic=m.group(2)

            cmd="%s create key --to-console" % (Utils.EosClientPath)
            if Utils.Debug: print("cmd: %s" % (cmd))
            keyStr=Utils.checkOutput(cmd.split())
            m=p.match(keyStr)
            if m is None:
                print("ERROR: Active key creation regex mismatch")
                break

            activePrivate=m.group(1)
            activePublic=m.group(2)

            name=''.join(random.choice(string.ascii_lowercase) for _ in range(12))
            account=Account(name)
            account.ownerPrivateKey=ownerPrivate
            account.ownerPublicKey=ownerPublic
            account.activePrivateKey=activePrivate
            account.activePublicKey=activePublic
            accounts.append(account)
            if Utils.Debug: print("name: %s, key(owner): ['%s', '%s], key(active): ['%s', '%s']" % (name, ownerPublic, ownerPrivate, activePublic, activePrivate))

        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            print("ERROR: Exception during key creation. %s" % (msg))
            break

    if count != len(accounts):
        print("Account keys creation failed. Expected %d, actual: %d" % (count, len(accounts)))
        return None

    return accounts

def startup():
    script = """
# set -x
pkill -9 nodeos
pkill -9 keosd
rm -rf test_wallet_0
rm -rf var
./bootstrap/eos_boot.sh %(hostname)s
if [ $? -ne 0 ]; then
    echo "bootstrap/eos_boot.sh failed"
    exit 1
fi

programs/keosd/keosd --data-dir test_wallet_0 --config-dir test_wallet_0 --http-server-address=%(hostname)s:9899 &>/dev/null &
sleep 1
programs/cleos/cleos --url http://%(hostname)s:8889 --wallet-url http://%(hostname)s:9899 wallet create --to-console
# programs/cleos/cleos --url http://%(hostname)s:8889 --wallet-url http://%(hostname)s:9899 wallet create --file data-dir/pass-file
echo Importing eosio private key...
programs/cleos/cleos --url http://%(hostname)s:8889 --wallet-url http://%(hostname)s:9899 wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3

echo Importing eosio. private key...
# logfile=eosio-ignition-wd/bootlog.txt
# prisyskey=`grep "^Private key:" $logfile | tail -1 | sed "s/^Private key://"`
# pubsyskey=`grep "^Public key:" $logfile | tail -1 | sed "s/^Public key://"`
pubsyskey=EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
prisyskey=5KBX1dwHME4VyuUss2sYM25D5ZTDvyYrbEz37UJqwAVAsR4tGuY
echo eosio. public key: $pubsyskey
programs/cleos/cleos --url http://%(hostname)s:8889 --wallet-url http://%(hostname)s:9899 wallet import --private-key $prisyskey
    """ % {"hostname":hostname}
    if Utils.Debug: print("script: %s" % (script))
    ret = os.system("bash -c '%s'" % script)
    if ret != 0:
        return (False, ret)

    cmd='programs/cleos/cleos --url http://%s:8889  --wallet-url http://%s:9899 push action -j eosio.token transfer \'{"from":"eosio","to":"fio.system","quantity":"10000000000.0000 FIO","memo":"init transfer"}\' --permission eosio@active' % (hostname,hostname)

    print(cmd)
    print(cmd,file=log_file)
    log_file.flush()
    ret=subprocess.call(shlex.split(cmd),stdout=log_file,stderr=log_file)
    if ret != 0:
        return (False, ret)

    return (True, None)

# pylint: disable=redefined-outer-name
def activateAccounts(accounts):
    """Activate accounts on the chain."""
    for i in range(len(accounts)):
        cmd="programs/cleos/cleos --url http://%(hostname)s:8889 --wallet-url http://%(hostname)s:9899 wallet import --private-key %(key)s" % {"hostname":hostname, "key":accounts[i].ownerPrivateKey}
        print(cmd)
        print(cmd,file=log_file)
        log_file.flush()
        ret=subprocess.call(cmd.split(),stdout=log_file,stderr=log_file)
        if ret != 0:
            return (False, ret)

        cmd="programs/cleos/cleos --url http://%(hostname)s:8889 --wallet-url http://%(hostname)s:9899 wallet import --private-key %(key)s" % {"hostname":hostname, "key":accounts[i].activePrivateKey}
        print(cmd)
        print(cmd,file=log_file)
        log_file.flush()
        ret=subprocess.call(cmd.split(),stdout=log_file,stderr=log_file)
        if ret != 0:
            return (False, ret)

        cmd='programs/cleos/cleos --url http://%s:8889  --wallet-url http://%s:9899 system newaccount -j eosio %s %s %s --stake-net "100 FIO" --stake-cpu "100 FIO" --buy-ram "100 FIO"' % (hostname,hostname,accounts[i].name, accounts[i].ownerPublicKey, accounts[i].activePublicKey)

        print(cmd)
        print(cmd,file=log_file)
        log_file.flush()
        ret=subprocess.call(shlex.split(cmd),stdout=log_file,stderr=log_file)
        if ret != 0:
            return (False, ret)

        cmd='programs/cleos/cleos --url http://%s:8889  --wallet-url http://%s:9899 push action -j eosio.token transfer \'{"from":"eosio","to":"%s","quantity":"10000.0000 FIO","memo":"init transfer"}\' --permission eosio@active' % (hostname,hostname,accounts[i].name)

        print(cmd)
        print(cmd,file=log_file)
        log_file.flush()
        ret=subprocess.call(shlex.split(cmd),stdout=log_file,stderr=log_file)
        if ret != 0:
            return (False, ret)

    return (True, None)

if __name__ == '__main__':
    try:
        ret1=startup()
        if not ret1[0]:
            print("ERROR: Failed to startup.")
            exit(1)

        names=('fioname11111', 'fioname22222', 'fioname33333')
        # names=('fioname11111',)
        accounts=getAccounts(len(names))
        for i in range(len(names)):
            accounts[i].name=names[i]

        ret1=activateAccounts(accounts)
        if not ret1[0]:
            print("ERROR: Failed to create accounts.")
            exit(1)

    finally:
        log_file.close()

exit(0)
