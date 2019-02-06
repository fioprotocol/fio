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


using std::string;

namespace fioio {

    using namespace eosio;

    // Transaction status
    enum class trxstatus {
        reject =    0,  // request rejected
        senttobc =  1   // request send to blockchain
    };

    // Structure for "FIO request" context.
    // @abi table fioreqctexts i64
    struct fioreqctxt {        // FIO funds request context; specific to requests native to FIO platform
        uint64_t    fioreqid;       // primary key, hash of {fioreqidstr}
        string      fioreqidstr;    // fio funds request id; genetrated by FIO blockchain e.g. 2f9c49bd83632b756efc5184aacf5b41b5ae436189f45c2591ca89248e161c09
        name        fromfioaddr;    // sender FIO address e.g. john.xyz
        name        tofioaddr;      // receiver FIO address e.g. jane.xyz
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

    // Structure for "OBT" context.
    // @abi table obtctxts i64
    struct obtctxt {        // Other Blockchain Transaction (OBT) specific data
        uint64_t    fioobtid;       // primary key, hash of {fioobtidstr}
        string      fioobtidstr;    // fio OBT id; genetrated by FIO blockchain e.g. 2f9c49bd83632b756efc5184aacf5b41b5ae436189f45c2591ca89248e161c09
        name        fromfioaddr;    // sender FIO address e.g. john.xyz
        name        tofioaddr;      // receiver FIO address e.g. jane.xyz
        string      frompubaddr;    // chain specific sender public address e.g 0xab5801a7d398351b8be11c439e05c5b3259aec9b
        string      topubaddr;      // chain specific receiver public address e.g 0xC8a5bA5868A5E9849962167B2F99B2040Cee2031
        string      amount;         // token quantity
        string      tokencode;      // token type e.g. BLU
        string      obtidstr;         // OBT id string e.g. 0xf6eaddd3851923f6f9653838d3021c02ab123a4a6e4485e83f5063b3711e000b
        uint64_t    obtid;          // hash of {obtidstr}
        string      metadata;       // JSON formatted meta data e.g. {"memo":"utility payment"}
        date        fiotime;        // FIO blockchain OBT received timestamp

        uint64_t primarykey() const     { return fioobtid; }
        uint64_t by_sender() const      { return fromfioaddr; }
        uint64_t by_receiver() const    { return tofioaddr; }
        uint64_t by_obtid() const       { return obtid; }
        uint64_t by_fiotime() const     { return fiotime; }
        EOSLIB_SERIALIZE(obtctxt, (fioobtid)(fioobtidstr)(tofioaddr)(topubaddr)(amount)(tokencode)(obtidstr)(obtid)(metadata)(fiotime))
    };
    // FIO OBT table
    typedef multi_index<N(obtctxts), obtctxt,
            indexed_by<N(bysender), const_mem_fun<obtctxt, uint64_t, &obtctxt::by_sender> >,
    indexed_by<N(byreceiver), const_mem_fun<obtctxt, uint64_t, &obtctxt::by_receiver> >,
    indexed_by<N(byfiotime), const_mem_fun<obtctxt, uint64_t, &obtctxt::by_fiotime> >
    > obt_contexts_table;

    // Structure for "OBT status" updates.
    // @abi table obtstss i64
    struct obtsts {
        uint64_t    id;             // primary key, auto-increment
        uint64_t    fioobtid;       // OBT {obtctxt.fioobtid} this OBT status update is related to
        trxstatus   status;         // OBT status
        date        fiotime;        // FIO blockchain status update received timestamp

        uint64_t primarykey() const     { return id; }
        uint64_t by_fioobtid() const     { return fioobtid; }
        uint64_t by_fiotime() const  { return statustime; }
        EOSLIB_SERIALIZE(obtsts, (id)(fioobtid)(status)(fiotime))
    };
    // OBT status table
    typedef multi_index<N(obtstss), obtsts,
            indexed_by<N(byfioobtid), const_mem_fun<obtsts, uint64_t, &obtsts::by_fioobtid> >,
    indexed_by<N(byfiotime), const_mem_fun<obtsts, uint64_t, &obtsts::by_fiotime> >
    > obt_status_table;

}