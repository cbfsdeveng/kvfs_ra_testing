// quick roll-your-own expect macro.
// TODO: use gtest.h
#include <glog/logging.h>


#define EXPECT(expr) \
    ((expr) ? static_cast<void>(0) \
    :  (static_cast<void>( LOG(ERROR) << "EXPECT failed: " << __STRING(expr) << \
        " " __FILE__ << " @ " << __LINE__ ), abort() ) )

