// @file index.cpp

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

#include "pch.h"

#include "mongo/bson/bsonelement.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/db/namespace.h"
#include "mongo/db/index.h"
#include "mongo/db/client.h"
#include "mongo/db/pdfile.h"

#include "d_index.h"

namespace mongo {

    /* Appends the missing fields from a source object to a
       target object and returns a new copy.
       Intended for index fields as sources, whose values
       are 1 or -1.  max/min objects as targets.
       bool min: flag to specify whether to append maxKey
                 or minKey based on the direction of the
                 source index.
    */
    static BSONObj modifyMinMax( const BSONObj& key_src,
                                      const BSONObj& key_target,
                                      bool min ) {
       BSONObjIterator isrc( key_src );
       BSONObjIterator itgt( key_target );
       BSONObjBuilder target_builder;
       while( isrc.more() ) {
            BSONElement se = isrc.next();
            BSONElement te = itgt.next();
            if ( te.eoo() ) {
                // append the remaining elements in source
                do {
                    int val = se.numberInt();
                    if ( !min ) val = -val; // descending field
                    if ( val < 0 )
                        target_builder.appendMinKey( "" );
                    else
                        target_builder.appendMaxKey( "" );
                    se = isrc.next();
                } while ( isrc.more() );
                break;
            }
            target_builder.appendAs( se , "" );

            if ( !strcmp( se.fieldName(), te.fieldName() ) == 0 ) {
                msgasserted( 16090 , "field names don't line up between keys" );
            }
       }
       return target_builder.obj();
    }

    static bool indexIsPrefix( const BSONObj& shortPattern,
                               const BSONObj& longPattern ) {
       BSONObjIterator sp( shortPattern );
       BSONObjIterator lp( longPattern );
       while( sp.more() ) {
            BSONElement se = sp.next();
            BSONElement le = lp.next();
            if ( le.eoo() )
                return false;
            if ( se.woCompare( le , true ) )
                return false;
       }
       return true;
    }

    /* This function will use the shard key to find the appropriate IndexDetails
     * Then modify the min and max objects to fit that pattern. It assumes that
     * the min and max are given with same fieldnames as the shard key
     *
     * Similar to a function in the query optimizer, but less sucky, since it does
     * NOT modify the key.
     *
     * Examples: if shardKeyPatterns is {a : 1} and there is an index {a : 1, b : -1} this
     * will choose that index, and modify the min and max by appending the extra fields
     * { b : maxKey} and {b : minKey} respectively.
     */
    IndexDetails* indexDetailsForChunkRange( const char *ns,
                                             std::string &errmsg,
                                             BSONObj &min,
                                             BSONObj &max,
                                             const BSONObj &shardKeyPattern ) {
       Client::Context ctx( ns );
       NamespaceDetails *d = nsdetails( ns );
       if ( !d ) {
            errmsg = "ns not found";
            return 0;
       }
       NamespaceDetails::IndexIterator i = d->ii();
        IndexDetails* bestIndex = 0;
       while( i.more() ) {
            IndexDetails& ii = i.next();
            BSONObj currentPattern = ii.keyPattern();
            if ( indexIsPrefix( shardKeyPattern , currentPattern ) ) {
                if ( currentPattern.nFields() == shardKeyPattern.nFields() ) {
                    // exact match -- use this index absolutely
                    bestIndex = &ii;
                    break;
                }
                // not an exact match -- remember this index and keep looping
                bestIndex = &ii;
            }
       }
        if ( !bestIndex ) {
            errmsg = "can't find index which contains the shard key" ;
            return 0;
        }

        min = modifyMinMax( min, bestIndex->keyPattern(), true );
        max = modifyMinMax( max, bestIndex->keyPattern(), false );
        return bestIndex;
    }

}
