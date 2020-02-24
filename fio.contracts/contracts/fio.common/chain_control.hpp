/** chain_control definitions file
 *  Description: Sets up the supported chain data structure
 *  @author Casey Gardiner
 *  @file chain_control.hpp
 *  @license FIO Foundation ( https://github.com/fioprotocol/fio/blob/master/LICENSE ) Dapix
 *
 *  Changes:
 */

#include <string>

namespace fioio {

    using namespace std;

    struct clentry {
        uint32_t index;
        string chain;
    };

    struct clist {
        vector <clentry> chains;
    };

    struct chainControl {
        clist chainList;

        inline string getChainFromIndex(int index) {
            for (int it = 0; it < chainList.chains.size(); it++) {
                if (index == chainList.chains[it].index) {
                    return chainList.chains[it].chain;
                }
            }
            return "";
        }

        inline int getIndexFromChain(string chainname) {
            for (int it = 0; it < chainList.chains.size(); it++) {
                if (chainname == chainList.chains[it].chain) {
                    return chainList.chains[it].index;
                }
            }
            return -1;
        }

        inline int getVectorIndex(int chainIndex) {
            for (int it = 0; it < chainList.chains.size(); it++) {
                if (chainIndex == chainList.chains[it].index) {
                    return it;
                }
            }
            return -1;
        }
    };
}