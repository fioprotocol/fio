#!/usr/bin/env python3

import collections
import os
import random
import re
import shlex
import string
import subprocess
import sys

hostname = "localhost"
# hostname="0.0.0.0"
# hostname="10.201.200.11"

CORE_SYMBOL = 'FIO'
# CORE_SYMBOL='SYS'

nPort = 8889
wPort = 9899

log_file = open("launcher.detailed.log", "w")


# pylint: disable=too-few-public-methods
class Utils(object):
    Debug = True
    EosClientPath = "programs/cleos/cleos"

    @staticmethod
    def checkOutput(cmd):
        assert (isinstance(cmd, list))
        popen = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (output, error) = popen.communicate()
        if popen.returncode != 0:
            raise subprocess.CalledProcessError(returncode=popen.returncode, cmd=cmd, output=error)
        return output.decode("utf-8")


# SyncStrategy=namedtuple("ChainSyncStrategy", "name id arg")
class Account(object):
    # pylint: disable=too-few-public-methods

    def __init__(self, name):
        self.name = name

        self.ownerPrivateKey = None
        self.ownerPublicKey = None
        self.activePrivateKey = None
        self.activePublicKey = None

    def __str__(self):
        return "Name: %s" % (self.name)


def getAccounts(count):
    """ Static list of account keys. Max count 10."""
    Info = collections.namedtuple("Info", "name ownerPrivate ownerPublic activePrivate activePublic")
    keys = [Info("user1", "5JLxoeRoMDGBbkLdXJjxuh3zHsSS7Lg6Ak9Ft8v8sSdYPkFuABF",
                 "EOS5oBUYbtGTxMS66pPkjC2p8pbA3zCtc8XD4dq9fMut867GRdh82",
                 "5K2HBexbraViJLQUJVJqZc42A8dxkouCmzMamdrZsLHhUHv77jF",
                 "EOS5GpUwQtFrfvwqxAv24VvMJFeMHutpQJseTz8JYUBfZXP2zR8VY"),
            Info("user2", "5JCpqkvsrCzrAC3YWhx7pnLodr3Wr9dNMULYU8yoUrPRzu269Xz",
                 "EOS7uRvrLVrZCbCM2DtCgUMospqUMnP3JUC1sKHA8zNoF835kJBvN",
                 "5JA5zQkg1S59swPzY6d29gsfNhNPVCb7XhiGJAakGFa7tEKSMjT",
                 "EOS8ApHc48DpXehLznVqMJgMGPAaJoyMbFJbfDLyGQ5QjF7nDPuvJ"),
            Info("user3", "5HvaoRV9QrbbxhLh6zZHqTzesFEG5vusVJGbUazFi5xQvKMMt6U",
                 "EOS8NToQB65dZHv28RXSBBiyMCp55M7FRFw6wf4G3GeRt1VsiknrB",
                 "5KQPk78a4Srm11NnuxGnWGNJuZtGaMErQ31XFDvXwzyS3Ru7AJQ",
                 "EOS8JzoVTmdFCnjs7x2qEq7A4cKgfRatvnohgngUPnZs8XfeFknjL"),
            Info("user4", "5K3s1yePVjrsCJbeyoXGAfJn9yCKKZi6E9oyw59mt5LaJG3Bm1N",
                 "EOS6ekFcLhgeS26uWuEUxzpnj8DB36M43DfF4axAdqBC9y42wP4bZ",
                 "5JqMPkY5AZMYe4LSu1byGPbyjwin9uE9uB3a14WC8ULtYvjaJzE",
                 "EOS8f7yfEhAVajCwCowJEuxJE4djuuVhp8dt9iXcSehJq3z4Zi4fM"),
            Info("user5", "5JAs6QTmEyVjJGbQf1e5MjWLoNqcUAMyZLbLwvDhaG3gYBawDwK",
                 "EOS5TZ4uoBu89S96f2dmBkwG5PsvYxRit3UxfbnVrwJdDFk3S3y2X",
                 "5KcNHxGqm9csmFq8VPa5dGHZ99H8qDGj5F5yWgg6u3D4PUkUoEp",
                 "EOS5n5qzFgoAEwXJhQquKpuHHKhGxpHrabvztSDgDKbrZkFL9fsGs"),
            Info("user6", "5JvWX1RkiaRS7jgMTb9oZsFKy77jJHkzEv78JRFkjxdDQR3d6wN",
                 "EOS7JHQcridAXNWPFsYFCvatJvfrZC8JKUJqWsxjnZbyLVbG89QPL",
                 "5KDDv35PHWGkReThhNsj2j23DCJzvWTLvZbSTUj8BFNicrJXPqG",
                 "EOS7UcoHGbP9p4sVMfQdNDkDdjoNUkrtDjZvCmLBWZ2YNuX788JRA"),
            Info("user7", "5JKQTYUjxWQbWt53z6Prrb3PUNtEiG5GcyXv3yHTK1nJaxzABrk",
                 "EOS7ziyUx4eQ3QGSGw1NwHpXJJ5ctbpfpuhCxt3beSxJ1oqspRqdR",
                 "5JHsotwiGMAjrTu3oCiXYjzJ3dEz9mU7HFc4UigvWgEksbpaVkc",
                 "EOS8D6vbjF1gK9BGqT2LZypsdU2N7dPaEXH7Kiz4zAc164UmVnUUZ"),
            Info("user8", "5K7Dah3CB17j6hpQ5vnBebN2yzGkt5K6A2xV7KshKyAfAUJyY8J",
                 "EOS88CmuS1QFeWwCWNS13dFf64y4ZBoMezQnmbSfQEDiC3n2FqEGp",
                 "5K8dPJgVYWDxfEURh1cvrRzqc7uvDLqNHjxs83Rf2n8Hu7rqhmp",
                 "EOS5C6dQV7xSjHKxWi4A4DYC29psNLWEYfqoWv1tjvo6yBi2HRC3x"),
            Info("user9", "5HvvvAF32bADDzyqcc3kRN4GVKGAZdVFXQexDScJEqTnpk29Kdv",
                 "EOS6YMqC4quk1z4bV6EXrXMzik4SAfeYAscaWmWnhgVSLxSKcqR9Y",
                 "5JDGpznJSJw8cKQYtEuctobAu2zVmD3Rw3fWwPzDuF4XbriseLy",
                 "EOS7uTxpJWW1qS52fyEdf9s2NvqhMcKTKpyuNK7GwffVf6wh3w1Jy"),
            Info("user10", "5JUgV5evcQGJkniK7rH1HckBAvRKsJP1Zgbv39UWnyTBEkMyWMj",
                 "EOS794cEVBvrj9bx1ecV1kszf6UHzQtTeBKvKNoFwT7Dg4MAAWvdV",
                 "5JiSAASb4PZTajemTr4KTgeNBWUxrz6SraHGUvNihSskDA9aY5b",
                 "EOS5Q9AkSnazMPMbEWLSMUkxQWVPhipshJE2qpiFDCZ1awphZQ3VD")]

    assert (count <= len(keys))

    # pylint: disable=redefined-outer-name
    accounts = []
    for i in range(0, count):
        name = keys[i].name
        account = Account(name)
        account.ownerPrivateKey = keys[i].ownerPrivate
        account.ownerPublicKey = keys[i].ownerPublic
        account.activePrivateKey = keys[i].activePrivate
        account.activePublicKey = keys[i].activePublic
        accounts.append(account)
        if Utils.Debug: print("name: %s, key(owner): ['%s', '%s], key(active): ['%s', '%s']" % (
            keys[i].name, keys[i].ownerPublic, keys[i].ownerPrivate, keys[i].activePublic, keys[i].activePrivate))

    return accounts


def createAccounts(count):
    """ Dynamically generate accounts info."""
    # pylint: disable=redefined-outer-name
    accounts = []
    p = re.compile('Private key: (.+)\nPublic key: (.+)\n', re.MULTILINE)
    for _ in range(0, count):
        try:
            cmd = "%s create key --to-console" % (Utils.EosClientPath)
            if Utils.Debug: print("cmd: %s" % (cmd))
            keyStr = Utils.checkOutput(cmd.split())
            m = p.search(keyStr)
            if m is None:
                print("ERROR: Owner key creation regex mismatch")
                break

            ownerPrivate = m.group(1)
            ownerPublic = m.group(2)

            cmd = "%s create key --to-console" % (Utils.EosClientPath)
            if Utils.Debug: print("cmd: %s" % (cmd))
            keyStr = Utils.checkOutput(cmd.split())
            m = p.match(keyStr)
            if m is None:
                print("ERROR: Active key creation regex mismatch")
                break

            activePrivate = m.group(1)
            activePublic = m.group(2)

            name = ''.join(random.choice(string.ascii_lowercase) for _ in range(12))
            account = Account(name)
            account.ownerPrivateKey = ownerPrivate
            account.ownerPublicKey = ownerPublic
            account.activePrivateKey = activePrivate
            account.activePublicKey = activePublic
            accounts.append(account)
            if Utils.Debug: print("name: %s, key(owner): ['%s', '%s], key(active): ['%s', '%s']" % (
                name, ownerPublic, ownerPrivate, activePublic, activePrivate))

        except subprocess.CalledProcessError as ex:
            msg = ex.output.decode("utf-8")
            print("ERROR: Exception during key creation. %s" % (msg))
            break

    if count != len(accounts):
        print("Account keys creation failed. Expected %d, actual: %d" % (count, len(accounts)))
        return None

    return accounts


def startup(logFileStr):
    logFileName = 'logging.json'
    if logFileStr:
        print("Writing out logging input file: " + logFileName)
        with open(logFileName, 'w') as the_file:
            the_file.write(logFileStr)

    script = """
#set -x
pkill -9 nodeos
pkill -9 keosd
rm -rf test_wallet_0
rm -rf var
#programs/eosio-launcher/eosio-launcher -p 1 -n 1 -s mesh -d 1 -i 2018-09-12T16:21:19.132 -f --p2p-plugin net --boot --specific-num 00 --specific-nodeos "--http-server-address $MY_INTERFACE:8888" --enable-gelf-logging 0 --nodeos "--max-transaction-time 50000 --abi-serializer-max-time-ms 990000 --filter-on * --p2p-max-nodes-per-host 1 --contracts-console"
echo Launching EOS nodes listening on host %(hostname)s, port %(nPort)s
programs/eosio-launcher/eosio-launcher -p 1 -n 1 -s mesh -d 1 -f --p2p-plugin net --specific-num 00 --specific-nodeos "--http-server-address %(hostname)s:%(nPort)s" --enable-gelf-logging 0 --nodeos "--max-transaction-time 50000 --abi-serializer-max-time-ms 990000 --filter-on * --p2p-max-nodes-per-host 25 --contracts-console --logconf %(logName)s --chain-state-db-size-mb 10240 --disable-ram-billing-notify-checks -e"
if [ $? -ne 0 ]; then
    echo "Launcher failed"
    exit 1
fi
# Privelaged account creation needs to happen before bios
programs/keosd/keosd --data-dir test_wallet_0 --config-dir test_wallet_0 --http-server-address=%(hostname)s:%(wPort)s &>/dev/null &
sleep 1
programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s wallet create --to-console &>/dev/null &
# import eosio key
EOSIO_PRVT_KEY=$(cat etc/eosio/node_bios/config.ini | grep private-key | cut -c 73-123)
programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s wallet import --private-key $EOSIO_PRVT_KEY
pubsyskey=EOS7isxEua78KPVbGzKemH4nj2bWE52gqj8Hkac3tc7jKNvpfWzYS
prisyskey=5KBX1dwHME4VyuUss2sYM25D5ZTDvyYrbEz37UJqwAVAsR4tGuY
echo Creating FIO system accounts with private key: $prisyskey, public key: $pubsyskey
programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s  create account eosio fio.token $pubsyskey $pubsyskey
programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s  create account eosio fio.system $pubsyskey $pubsyskey
programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s  create account eosio fio.fee $pubsyskey $pubsyskey
programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s  create account eosio fio.finance $pubsyskey $pubsyskey
# kill keosd as bios script launches its own
pkill -9 keosd
echo Running nodeos bios steps
bash bios_boot.sh
if [ $? -ne 0 ]; then
    echo "bios_boot.sh failed"
    exit 1
fi
# bios_boot.sh kills all keosd instances, so restarting it
rm -rf test_wallet_0
programs/keosd/keosd --data-dir test_wallet_0 --config-dir test_wallet_0 --http-server-address=%(hostname)s:%(wPort)s &>/dev/null &
sleep 1
programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s wallet create --to-console
#programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s wallet create --file data-dir/pass-file
# import eosio key
programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s wallet import --private-key $EOSIO_PRVT_KEY
programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s wallet import --private-key $prisyskey
programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s system buyram eosio fio.system "100000.000 FIO" -p eosio@active
programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s system delegatebw eosio fio.system "10000.000 FIO" "10000.000 FIO" -p eosio@active
    """ % {"hostname": hostname, "logName": logFileName, "nPort": nPort, "wPort": wPort}
    if Utils.Debug: print("script: %s" % (script))
    sys.stdout.flush()
    ret = os.system("bash -c '%s'" % script)
    if ret != 0:
        return (False, ret)

    cmd = 'programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s  --wallet-url http://%(hostname)s:%(wPort)s push action -j eosio.token transfer \'{"from":"eosio","to":"fio.system","quantity":"200000000.0000 FIO","memo":"init transfer"}\' --permission eosio@active' % {
        "hostname": hostname, "nPort": nPort, "wPort": wPort}

    print(cmd)
    print(cmd, file=log_file)
    log_file.flush()
    ret = subprocess.call(shlex.split(cmd), stdout=log_file, stderr=log_file)
    if ret != 0:
        return (False, ret)

    return (True, None)


# pylint: disable=redefined-outer-name
def activateAccounts(accounts):
    """Activate accounts on the chain."""
    for i in range(len(accounts)):
        cmd = "programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s wallet import --private-key %(key)s" % {
            "hostname": hostname, "key": accounts[i].ownerPrivateKey, "nPort": nPort, "wPort": wPort}
        print(cmd)
        print(cmd, file=log_file)
        log_file.flush()
        ret = subprocess.call(cmd.split(), stdout=log_file, stderr=log_file)
        if ret != 0:
            return (False, ret)

        cmd = "programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s --wallet-url http://%(hostname)s:%(wPort)s wallet import --private-key %(key)s" % {
            "hostname": hostname, "key": accounts[i].activePrivateKey, "nPort": nPort, "wPort": wPort}
        print(cmd)
        print(cmd, file=log_file)
        log_file.flush()
        ret = subprocess.call(cmd.split(), stdout=log_file, stderr=log_file)
        if ret != 0:
            return (False, ret)

        cmd = 'programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s  --wallet-url http://%(hostname)s:%(wPort)s system newaccount -j --transfer --stake-net "1000000.0000 %(symbol)s" --stake-cpu "1000000.0000 %(symbol)s" --buy-ram "100000.0000 %(symbol)s" eosio %(name)s %(owner)s %(active)s' % {
            "hostname": hostname, "symbol": CORE_SYMBOL, "name": accounts[i].name, "owner": accounts[i].ownerPublicKey,
            "active": accounts[i].activePublicKey, "nPort": nPort, "wPort": wPort}
        # print(cmd.split())
        print(cmd)
        print(cmd, file=log_file)
        log_file.flush()
        ret = subprocess.call(shlex.split(cmd), stdout=log_file, stderr=log_file)
        if ret != 0:
            return (False, ret)

        cmd = 'programs/cleos/cleos --no-auto-keosd --url http://%(hostname)s:%(nPort)s  --wallet-url http://%(hostname)s:%(wPort)s push action -j eosio.token transfer \'{"from":"eosio","to":"%(to)s","quantity":"10000.0000 %(symbol)s","memo":"init transfer"}\' --permission eosio@active' % {
            "hostname": hostname, "symbol": CORE_SYMBOL, "to": accounts[i].name, "nPort": nPort, "wPort": wPort}
        print(cmd)
        print(cmd, file=log_file)
        log_file.flush()
        ret = subprocess.call(shlex.split(cmd), stdout=log_file, stderr=log_file)
        if ret != 0:
            return (False, ret)

    return (True, None)


logFileStr = """
{
  "includes": [],
  "appenders": [{
      "name": "stderr",
      "type": "console",
      "args": {
        "stream": "std_error",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ]
      },
      "enabled": true
    },{
      "name": "stdout",
      "type": "console",
      "args": {
        "stream": "std_out",
        "level_colors": [{
            "level": "debug",
            "color": "green"
          },{
            "level": "warn",
            "color": "brown"
          },{
            "level": "error",
            "color": "red"
          }
        ]
      },
      "enabled": true
    },{
      "name": "net",
      "type": "gelf",
      "args": {
        "endpoint": "10.160.11.21:12201",
        "host": "bios"
      },
      "enabled": true
    }
  ],
  "loggers": [{
      "name": "default",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr",
        "net"
      ]
    },{
      "name": "net_plugin_impl",
      "level": "debug",
      "enabled": true,
      "additivity": false,
      "appenders": [
        "stderr",
        "net"
      ]
    }
  ]
}
"""

if __name__ == '__main__':
    try:
        ret1 = startup(logFileStr)
        if not ret1[0]:
            print("ERROR: Failed to startup.")
            exit(1)

        names = ('fioname11111', 'fioname22222', 'fioname33333')
        # names=('hello.code',) # need a comma at the end to indicate this is a single element tuple, else P breaks string into tuple of characters.
        accounts = getAccounts(len(names))
        for i in range(len(names)):
            accounts[i].name = names[i]

        ret1 = activateAccounts(accounts)
        if not ret1[0]:
            print("ERROR: Failed to create accounts.")
            exit(1)

    finally:
        log_file.close()

exit(0)
