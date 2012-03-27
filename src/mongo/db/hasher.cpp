/* hasher.cpp
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

#include "hasher.h"

namespace mongo {

    MD5Hasher::MD5Hasher( HashSeed seed ) : _seed( seed ) {
        md5_init( &_st );
        md5_append( &_st , reinterpret_cast< const md5_byte_t * >( & _seed ) , sizeof( HashSeed ) );
    }

    void MD5Hasher::addData( const void * keyData , int numBytes ) {
        md5_append( &_st , static_cast< const md5_byte_t * >( keyData ), numBytes );
    }

    void MD5Hasher::finish( HashDigest out ) {
        md5_finish( &_st , out );
    }

}
