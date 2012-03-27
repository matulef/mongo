/*
 * hashindex.cpp
 *
 *  Created on: Jan 24, 2012
 *      Author: matulef
 */

#include "hashindex.h"
#include "json.h"

namespace mongo {

    HashedIndexType::HashedIndexType( const IndexPlugin* plugin , const IndexSpec* spec ) :
            IndexType( plugin , spec ) {

        _keyPattern = spec->keyPattern;

        //The seed is hidden in the IndexSpec
        if ((spec->info).hasField("seed")) {
            _seed = (spec->info)["seed"].numberInt();
        } else {
            _seed = 0;
        }

        BSONObjIterator i( spec->keyPattern );
        while ( i.more() ) {
            BSONElement e = i.next();
            if ( e.type() == String && e.valuestr() == HASHEDINDEXNAME) {
                _hashedfield = e.fieldName();
            }
            else {
                _otherfields.push_back( e.fieldName() );
            }
            massert(16075,"error: currently only single-field hashed indexes supported",
                    _otherfields.size() == 0);
        }

    }


	IndexSuitability HashedIndexType::suitability( const BSONObj& query ,
	        const BSONObj& order ) const {
	    if ( isEqualityQuery( _hashedfield , query ) ) {
	        return HELPFUL;
	    }
	    return USELESS;
	}


	void HashedIndexType::getKeys( const BSONObj &obj, BSONObjSet &keys ) const {

	    BSONObjIterator i(obj);
	    while( i.more() ){
	        BSONElement e = i.next();
	        if ( e.fieldName() == _hashedfield ){
	            keys.insert( makeSingleKey( obj[_hashedfield] , _seed ) );
	        }
	        //in the future maybe get other keys here
	    }

	}

	shared_ptr<Cursor> HashedIndexType::newCursor( const BSONObj& query ,
	        const BSONObj& order , int numWanted ) const {

	    massert( 16076 , "error: hashed field is missing from query" ,
	            query.hasField( _hashedfield.c_str() ) );

	    BSONObj hashedKey = makeSingleKey( query[_hashedfield] , _seed );

	    int sortOrder = 1;
	    if ( order.hasField( _hashedfield.c_str() ) ) {
	        sortOrder = order[_hashedfield].numberInt();
	    }

	    //This makes a BtreeCursor whose start and end bounds are the same
	    const shared_ptr< BtreeCursor > c(
	            BtreeCursor::make( nsdetails( _spec->getDetails()->parentNS().c_str()),
	                    *( _spec->getDetails() ),  hashedKey , hashedKey , true , sortOrder ) );

	    //This forces a match of the query against the actual document
	    const shared_ptr< CoveredIndexMatcher > emptyMatcher(
	            new CoveredIndexMatcher( query , BSONObj() , false ) );
	    c->setMatcher( emptyMatcher );
	    return c;

	}

	class HashedIndexPlugin : public IndexPlugin {
	public:

		HashedIndexPlugin() : IndexPlugin( HASHEDINDEXNAME ) {
		}

		virtual IndexType* generate( const IndexSpec* spec ) const {
			return new HashedIndexType( this , spec );
		}

	} hashedIndexPlugin;



}

