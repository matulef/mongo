/*
 * hashindex.cpp
 *
 *  Created on: Jan 24, 2012
 *      Author: matulef
 */

#include "namespace-inl.h"
#include "pdfile.h"
#include "index.h"
#include "btree.h"
#include "hasher.h"
#include "matcher.h"

namespace mongo {

	const string HASHEDINDEXNAME = "hashed";

    /* This is an index where the values are hashes of a given field.
     * This can be created using ensureIndex({a: "hashed"})
     *
     * LIMITATION: currently only works with a single field. If you try
     * to do ensureIndex({a : "hashed", b : 1}) an error will be thrown.
     */
	class HashedIndexType : public IndexType{
	public:

	    HashedIndexType( const IndexPlugin* plugin , const IndexSpec* spec );
	    virtual ~HashedIndexType(){ }

		/* Currently this index is only considered "HELPFUL" if it's a simple equality query.
		 * Namely, if it's a query of the form { a : <value> } where <value> is not a sub-object
		 * whose first field starts with '$'.  Otherwise, it's considered "USELESS".
		 * Known limitations:
		 *  - $in queries will not use this index, even if all the subqueries are equality
		 *        queries (i.e. {a : {$in : [1,2]}} should theoretically be able to use
		 *        the index, but will not).
		 */
		IndexSuitability suitability( const BSONObj& query , const BSONObj& order ) const;

		/* The input is "obj" which should have a field corresponding to the hashedfield.
		 * The output is a BSONObj with a single BSONElement whose value is the hash
		 * Eg if this is an index on "a" we have
		 *   obj is {a : 45} --> key becomes {"" : hash(45) }
		 *
		 * Known Limitations:
		 *  - if the value is an array, it will *not* generate a set of keys, instead
		 *    it will just generate a single key by hashing the array as a whole.  It's
		 *    arguable whether this is actually the desired behavior.
		 */

		void getKeys( const BSONObj &obj, BSONObjSet &keys ) const;


		/* The newCursor method assumes the query is suitable, i.e that it's an equality query.
		 * It works by generating a BTreeCursor with min-max bounds exactly equal to the hashed
		 * value then setting the Matcher in the BTreeCursor to see if the whole doc matches.
		 */

		shared_ptr<Cursor> newCursor( const BSONObj& query ,
		        const BSONObj& order , int numWanted ) const;

		/* A helper method used in a couple places.
		 * Takes a BSONElement and seed, and generates an object with a blank field name
		 * and hashed value as value.  E.g. if the element is {a : 3} this outputs
		 * { "" : hash(3) }
		 *
		 * */
        static BSONObj makeSingleKey( const BSONElement e , HashSeed seed )  {
            BSONObjBuilder keybuilder;
            keybuilder.appendNumber("", e.hash64(seed) );
            return keybuilder.obj();
        }

        /* Checks whether a given query is an equality query on a given field (as
         * opposed to something more fancy like a range query, etc.)
         */
        static bool isEqualityQuery(const string& fieldname ,  const BSONObj& query ) {
            if ( query.hasField( fieldname.c_str() ) ){
                BSONElement usefulElt = query[fieldname];
                if ( ! usefulElt.isABSONObj() ) {
                    return true;
                } else {
                    BSONObj embeddedObj = usefulElt.Obj();
                    if ( embeddedObj.firstElementFieldName()[0] != '$' )
                        return true;
                }
            }
            return false;
        }

        bool scanAndOrderRequired( const BSONObj& query , const BSONObj& order ) const {
            return false;
        }


	protected:
		string _hashedfield;
		BSONObj _keyPattern;
		vector<string> _otherfields;
		int _seed;

	};


}

