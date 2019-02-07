/** FIO Request OBT header file
 *  Description: Smart contract to track requests and OBT.
 *  @author Ciju John
 *  @file fio.request.obt.hpp
 *  @copyright Dapix
 *
 *  Changes:
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
    // @abi table fioreqctexts i64
    struct fioreqctxt {        // FIO funds request context; specific to requests native to FIO platform
        uint64_t    fioreqid;       // primary key, hash of {fioreqidstr}
        string      fioreqidstr;    // fio funds request id; genetrated by FIO blockchain e.g. 2f9c49bd83632b756efc5184aacf5b41b5ae436189f45c2591ca89248e161c09
        pub_addr    fromfioaddr;   // sender FIO address e.g. john.xyz
        pub_addr    tofioaddr;     // receiver FIO address e.g. jane.xyz
        string      topubaddr;      // chain specific receiver public address e.g 0xC8a5bA5868A5E9849962167B2F99B2040Cee2031
        string      amount;         // token quantity
        string      tokencode;      // token type e.g. BLU
        string      metadata;       // JSON formatted meta data e.g. {"memo":"utility payment"}
        date        fiotime;        // FIO blockchain request received timestamp

        uint64_t primarykey() const     { return fioreqid; }
        uint64_t by_sender() const      { return fromfioaddr; }
        uint64_t by_receiver() const    { return tofioaddr; }
        uint64_t by_fiotime() const     { return fiotime; }
        EOSLIB_SERIALIZE(fioreqctxt, (fioreqid)(fioreqidstr)(tofioaddr)(topubaddr)(amount)(tokencode)(metadata)(fiotime))
    };
    // FIO requests contexts table
    typedef multi_index<N(fioreqctexts), fioreqctext,
            indexed_by<N(bysender), const_mem_fun<fioreqctext, uint64_t, &fioreqctext::by_sender> >,
    indexed_by<N(byreceiver), const_mem_fun<fioreqctext, uint64_t, &fioreqctext::by_receiver> >,
    indexed_by<N(byfiotime), const_mem_fun<fioreqctext, uint64_t, &fioreqctext::by_fiotime> >
    > fiorequest_contexts_table;

    // Structure for "FIO request status" updates.
    // @abi table fioreqstss i64
    struct fioreqsts {
        uint64_t    id;             // primary key, auto-increment
        uint64_t    fioreqid;       // FIO request {fioreqctxt.fioreqid} this request status update is related to
        trxstatus   status;         // request status
        string      metadata;       // JSON formatted meta data e.g. {"obt_hash": "962167B2F99B20"}
        date        fiotime;        // FIO blockchain status update received timestamp

        uint64_t primarykey() const     { return id; }
        uint64_t by_fioreqid() const    { return fioreqid; }
        uint64_t by_fiotime() const     { return fiotime; }
        EOSLIB_SERIALIZE(fioreqsts, (id)(fioreqid)(status)(fiotime))
    };
    // FIO requests status table
    typedef multi_index<N(fioreqstss), fioreqsts,
            indexed_by<N(byfioreqid), const_mem_fun<fioreqsts, uint64_t, &fioreqsts::by_fioreqid> >,
    indexed_by<N(byfiotime), const_mem_fun<fioreqsts, uint64_t, &fioreqsts::by_fiotime> >
    > fiorequest_status_table;

}