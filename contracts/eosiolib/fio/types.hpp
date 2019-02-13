/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

/**
 * @brief Public Address of an account
 * @details Public Address of an account
 */
typedef uint64_t public_address;

namespace eosio {

    /**
 *  Wraps a uint128_t to ensure it is only passed to methods that expect a Public Address and
 *  that no mathematical operations occur.  It also enables specialization of print
 *  so that it is printed as a base32 string.
     *
     *  TBD: This class needs to be developed further to map the public address to hash conversion.
 *
 *  @brief wraps a uint128_t to ensure it is only passed to methods that expect a Public Address
 *  @ingroup types
 */
    struct pub_addr {
        /**
         * Conversion Operator to convert public address to uint128_t
         *
         * @brief Conversion Operator
         * @return uint128_t - Converted result
         */
        operator uint64_t()const { return value; }

        // keep in sync with name::operator string() in eosio source code definition for name
        std::string to_string() const {
            return address_str;
        }

        /**
         * Equality Operator for public address
         *
         * @brief Equality Operator for public address
         * @param a - First data to be compared
         * @param b - Second data to be compared
         * @return true - if equal
         * @return false - if unequal
         */
        friend bool operator==( const pub_addr& a, const pub_addr& b ) { return a.value == b.value; }

        /**
         * Internal Representation of the account public address
         *
         * @brief Internal Representation of the account public address
         */
        public_address value = 0;   // public address hash
        std::string address_str;     // public address e.g 0xC8a5bA5868A5E9849962167B2F99B2040Cee2031

    private:

        EOSLIB_SERIALIZE( pub_addr, (value)(address_str) )
    };

}
