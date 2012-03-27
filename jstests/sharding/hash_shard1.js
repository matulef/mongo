//hash_shard1.js

/**
* test basic sharding with hashing
*/

s = new ShardingTest({name : jsTestName() , shards : 2, mongos : 1, verbose : 2} );

//for simplicity start by turning off balancer
s.stopBalancer()

db = s.getDB( "test" );

db.adminCommand( { enablesharding : "test" } );
db.adminCommand( { shardcollection : "test.foo" , key : { a : 1 } , unique : true , hashed : true } );
assert.eq( 1 , s.config.chunks.count()  , "sanity check 1" );

db.adminCommand({split : "test.foo" , middle : {a : NumberLong(-3074457345618258400)}});
db.adminCommand({split : "test.foo" , middle : {a : NumberLong(3074457345618258400)}});
assert.eq( 3 , s.config.chunks.count() , "should be 3 chunks now" );

for(i=0; i < 9000 ; i++){
    db.foo.insert({a : i}); 
}
assert.eq( db.foo.find().count(), 9000 , "wrong number of items" );

s.stop()