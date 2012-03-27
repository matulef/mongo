
#include "mongo/bson/bsonobj.h"
#include <string>

namespace mongo {
    class IndexDetails;

    IndexDetails *indexDetailsForChunkRange( const char *ns, std::string &errmsg,
            BSONObj &min, BSONObj &max, const BSONObj &shardKeyPattern );

}
