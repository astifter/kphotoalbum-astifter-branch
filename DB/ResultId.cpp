#include "ResultId.h"
#include "Result.h"

DB::ResultId::ResultId(int fileId, const DB::Result& context)
    :_fileId(fileId),_context( context )
{

}
