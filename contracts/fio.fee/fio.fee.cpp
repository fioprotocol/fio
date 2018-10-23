
#include "fio.fee.hpp"
#include <fio.common/fio.common.hpp>

namespace fioio {

    /***
     * This contract maintains fee related actions and data. Most importantly its the host of the transaction fee
     * structure that is used by sister contracts to 
     */
    class FioFee : public contract {
    private:
        trxfees_singleton trxfees;

        void initialize() {
            print("Setting default fees.");
            auto fees = trxfees.get_or_default(trxfee());
            fees.id = 1;     // default voting cycle is 1
            fees.expiration = now() + 86400;     // default expiration is 24 hrs
            fees.domregiter = asset(140000, S(4, FIO));     // 14 FIO domain registration fee
            fees.nameregister = asset(10000, S(4, FIO));    // 1 FIO name registration fee
            fees.domtransfer = asset(140000, S(4, FIO));    // 14 FIO domain transfer fee
            fees.nametransfer = asset(1000, S(4, FIO));     // 0.1 FIO name transfer fee
            fees.namelookup = asset(1000, S(4, FIO));       // 0.1 FIO name lookup fee
            fees.upaddress = asset(1000, S(4, FIO));        // 0.1 FIO update/add address fee
            fees.transfer = asset(1000, S(4, FIO));         // 0.1 FIO asset transfer fee
            fees.metadata = asset(1000, S(4, FIO));         // 0.1 FIO metadata recording fee
            trxfees.set(fees, _self);
        }

        // TODO: timer for driving elections and updating fees.

    public:
        FioFee(account_name self)
        : contract(self), trxfees(self, self) {
            initialize();
        }

    }; // class FioFee

    EOSIO_ABI( FioFee,  )
} // namespace fioio
