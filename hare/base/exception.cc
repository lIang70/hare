#include "hare/base/thread/local.h"
#include <hare/base/exception.h>

namespace hare {

Exception::Exception(std::string what)
    : what_(std::move(what))
    , stack_(current_thread::stackTrace(false))
{
}

}