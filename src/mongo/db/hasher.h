/* hasher.h
 *
 * Defines a simple hash function class
 */


/**
*    Copyright (C) 2012 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "../pch.h"
#include "../util/md5.hpp"

namespace mongo {

    typedef int HashSeed;
    typedef unsigned char HashDigest[16];

    class Hasher {
    public:
        //pointer to next part of input key, length in bytes to read
        virtual void addData( const void * keyData , int numBytes ) = 0;

        //finish computing the hash, put the result in the digest
        //only call this once per Hasher
        virtual void finish( HashDigest out ) = 0;

        virtual ~Hasher() { };
    };

    class MD5Hasher : public Hasher {
    private:
        virtual ~MD5Hasher() {};
        md5_state_t _st;
        HashSeed _seed;

    public:

        explicit MD5Hasher( HashSeed seed );

        void addData( const void * keyData , int numBytes );

        void finish( HashDigest out );

    };

    class HasherFactory {
    public:
        /* Eventually this may be a more sophisticated factory
         * for creating other hashers, but for now use MD5.
         */
        static Hasher* createHasher( HashSeed seed ) {
            return new MD5Hasher( seed );
        }
    };

}
