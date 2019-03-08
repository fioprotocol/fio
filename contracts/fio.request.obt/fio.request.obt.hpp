/** FIO Request OBT header file
 *  Description: Smart contract to track requests and OBT.
 *  @author Ciju John
 *  @file fio.request.obt.hpp
 *  @copyright Dapix
 *
 *  Changes:
 *  Ed Rotthoff 2/13/2019   modified the includes so they will build properly when used.
 */


#pragma once

#include <eosiolib/eosio.hpp>
#include <string>
#include <eosiolib/singleton.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/fio/types.hpp>


using std::string;

namespace fioio {

    using namespace eosio;

    // Transaction status
    enum class trxstatus {
        requested = 0,  // request sent
        rejected =  1,  // request rejected
        senttobc =  2   // payment sent to blockchain
    };

    // Structure for "FIO request" context.
    // @abi table fioreqctxts i64
    struct fioreqctxt {        // FIO funds request context; specific to requests native to FIO platform
        uint64_t    fioreqid;       // one up index starting at 0
        uint64_t    fromfioaddr;   // sender FIO address e.g. john.xyz
        uint64_t    tofioaddr;     // receiver FIO address e.g. jane.xyz
        string      topubaddr;      // chain specific receiver public address e.g 0xC8a5bA5868A5E9849962167B2F99B2040Cee2031
        string      amount;         // token quantity
        string      tokencode;      // token type e.g. BLU
        string      metadata;       // JSON formatted meta data e.g. {"memo":"utility payment"}
        uint64_t    fiotime;        // FIO blockchain request received timestamp

        uint64_t primary_key() const     { return fioreqid; }
       uint64_t by_receiver() const    { return tofioaddr; }
        EOSLIB_SERIALIZE(fioreqctxt, (fioreqid)(fromfioaddr)(tofioaddr)(topubaddr)(amount)(tokencode)(metadata)(fiotime))
    };
    // FIO requests contexts table
    typedef multi_index<N(fioreqctxts), fioreqctxt,
    indexed_by<N(byreceiver), const_mem_fun<fioreqctxt, uint64_t, &fioreqctxt::by_receiver> >
    > fiorequest_contexts_table;

    // Structure for "FIO request status" updates.
    // @abi table fioreqstss i64
    struct fioreqsts {
        uint64_t    id;             // primary key, auto-increment
        uint64_t    fioreqid;       // FIO request {fioreqctxt.fioreqid} this request status update is related to
        uint64_t   status;         // request status
        string      metadata;       // JSON formatted meta data e.g. {"obt_hash": "962167B2F99B20"}
        uint64_t    fiotime;        // FIO blockchain status update received timestamp

        uint64_t primary_key() const     { return id; }
        uint64_t by_fioreqid() const    { return fioreqid; }
        EOSLIB_SERIALIZE(fioreqsts, (id)(fioreqid)(status)(metadata)(fiotime))
    };
    // FIO requests status table
    typedef multi_index<N(fioreqstss), fioreqsts,
            indexed_by<N(byfioreqid), const_mem_fun<fioreqsts, uint64_t, &fioreqsts::by_fioreqid> >
    > fiorequest_status_table;

}
