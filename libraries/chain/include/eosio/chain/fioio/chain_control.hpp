/** chain_control definitions file
 *  Description: Sets up the supported chain data structure
 *  @author Casey Gardiner
 *  @file fio_name_validator.hpp
 *  @copyright Dapix
 *
 *  Changes:
 */

#include <string>
#include <fc/io/json.hpp>
#include <boost/filesystem.hpp>

namespace fioio {

    using namespace std;
    namespace bs = boost::filesystem;

    bs::path JSONFILE("/config/bip44chains.json");
    vector<string> chainList;

    inline void chainControlInit( void ){
        bs::path currentpath = bs::current_path();
        string comppath = currentpath.string() + JSONFILE.string();
        currentpath = comppath;

        try {
            fc::json::from_file(currentpath).as<vector<string>>(chainList);
            ilog("chainList: ${t}",("t", chainList[0]));
        } catch (...) {
            elog ("failed to read ${f}",("f",currentpath.string()));
        }

        ilog(chainList[0]);
    }

    inline string getChainFromIndex(int index) {
        string chainName = chainList[index];

        return chainName;
    }

    inline int getIndexFromChain( string chainname ){
        vector<string>::iterator it = find(chainList.begin(), chainList.end(), chainname);
        int index = distance(chainList.begin(), it);
        int result = -1;

        if(it != chainList.end()) {
            result = distance(chainList.begin(), it);
        } else {
            //ASSERT
        }

        return result;
    }
}
