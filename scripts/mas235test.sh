#!/bin/bash

#0. add a known good domain
#1. add a domain name with caps
#2. add a domain name with leading -
#3. add a domain with an embedded -
#4. add a username with caps
#5. add a username with an embedded -
#6. add a username with trailng -
#7. add a name with an ivalid character
#8. add a name with double dots
#9. add an excessively long name
#10. add a name with unknown domain
#11. add a name  by a different actor

npass=0
nfail=0

expect_pass()
{
    echo expect success...
    echo $result | grep "status\":\"executed"
    if [ $? == 0 ]; then
        echo ***PASS***
        npass=$(($npass + 1))
    else
        echo ***FAIL*** - block info: $blockinfo
        nfail=$(($nfail + 1))
   fi
}

expect_fail()
{
    echo expect invalid...
    echo $result | grep "invalid_input"
    if [ $? == 0 ]; then
        echo ***PASS***
        npass=$(($npass + 1))
    else
        echo ***FAIL*** - block info: $blockinfo
        nfail=$(($nfail + 1))
   fi
}

get_trx()
{
    ../utils/sign-pack-trx.sh $1 > pack.out 2> pack.err
    packtrx=`grep "PCK RSP" pack.out | sed 's/PCK RSP: //'`
    blockinfo=`grep BLOCK pack.out`
}

run_a_test()
{
    get_trx $2
    sleep 1

    result=`curl ${nodeurl}${register} -d "$packtrx"`

    if [ $1 == "pass" ]; then
        expect_pass
    else
        expect_fail
    fi
    sleep 5
}


orgdir=$PWD

if [ `basename $PWD` != "build" ]; then
   cd build
fi


unset fioprivatekey
unset fiopubkey
unset tempactor

pkill nodeos
pkill keosd
rm -rf ~/eosio-wallet
rm ../walletkey.ini
rm -rf ../node2
rm -rf ~/Library/Application\ Support/eosio

pushd ..
./fioio_launch.sh 1
popd
sleep 5
nodeurl=http://localhost:8889
register=/v1/chain/register_fio_name
scope=fio.system

echo step 0. add a known good domain
run_a_test pass dapix

echo step 1. add a domain name with caps
run_a_test pass NOTdapix

echo step 2. add a domain name with leading -
run_a_test fail -bogusDom

echo step 3. add a domain with an embedded -
run_a_test pass dash-embedded

echo step 4. add a username with caps
run_a_test pass MyName.NOTdapix

echo step 5. add a username with an embedded -
run_a_test pass dash-ok.NOTdapix

echo step 6. add a username with trailng -
run_a_test fail bogus-.NOTdapix

echo step 7. add a name with invalid character
run_a_test fail bad^char.NOTdapix

echo step 8. add a name with second dot
run_a_test fail double.dot.NOTdapix

echo step 9. add an excessively long name
longname=5xxJwttkBMpCijBWiLx75hHTkYGgDm5gmny7nvnss4s1FoZWPxNui.5JwttkBMpCijBWiLx75hHTkYGgDm5gmny7nvnss4s1FoZWPxNuixx
run_a_test fail $longname

echo step 10. add a name with unknown domain
run_a_test fail auser.nodomain

echo step 11. add a name  by a different actor
sleep 1
cleos -u ${nodeurl} create key -f tempkey
export fioprivatekey=`grep 'Private' tempkey | awk '{print $NF}'`
export fiopubkey=`grep 'Public key' tempkey | awk '{print $NF}'`
export tempactor=`grep 'Actor' tempkey | awk '{print $NF}'`

run_a_test pass altactor.dapix

unset fioprivatekey
unset fiopubkey
unset tempactor


cleos -u ${nodeurl} get table fio.system ${scope} domains
cleos -u ${nodeurl} get table fio.system ${scope} fionames

pkill nodeos
pkill keosd
echo '*****************************'
echo $npass tests pass, $nfail tests failed
