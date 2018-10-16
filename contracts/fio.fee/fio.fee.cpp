
#include "fio.fee.hpp"
#include <fio.common/fio.common.hpp>

namespace fioio {

    class FioFee : public contract {
    private:
        trxfees_singleton trxfees;

        void initialize() {
            auto fees = trxfees.get_or_default(trxfee());
            fees.id = 1;     // default voting cycle is 1
            fees.expiration = now() + 86400;     // default expiration is 24 hrs
            fees.domregiter = asset(140000, S(4, FIO));   // 14 FIO
            fees.nameregister = asset(10000, S(4, FIO));  // 1 FIO
            fees.domtransfer = asset(140000, S(4, FIO));  // 14 FIO
            fees.nametransfer = asset(1000, S(4, FIO));  // 0.1 FIO
            fees.namelookup = asset(1000, S(4, FIO));    // 0.1 FIO
            fees.upaddress = asset(1000, S(4, FIO));     // 0.1 FIO
            fees.transfer = asset(1000, S(4, FIO));      // 0.1 FIO
            fees.metadata = asset(1000, S(4, FIO));      // 0.1 FIO
            trxfees.set(fees, _self);
        }

        // TODO: timer for driving elections and updating fees.

    public:
        FioFee(account_name self)
        : contract(self), trxfees(self, self) {
            initialize();
        }




    }; // class FioFee
} // namespace fioio
