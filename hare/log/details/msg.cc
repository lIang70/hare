#include "hare/base/io/local.h"
#include <hare/log/details/msg.h>

namespace hare {
namespace log {
    namespace details {

        msg::msg(const std::string* _name, LEVEL _level, source_loc _loc)
            : name_(_name)
            , level_(_level)
            , tid_(io::current_thread::get_tds().tid)
            , loc_(_loc)
        {
        }

    } // namespace details
} // namespace log
} // namespace hare