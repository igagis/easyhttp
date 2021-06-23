#include <tst/application.hpp>

#include "../../src/httpc/init_guard.hpp"

namespace{
class application : public tst::application{
    httpc::init_guard httpc_init_guard;
public:
    application() :
            tst::application(std::string(), std::string())
    {}
};
}

tst::application_factory app_fac([](){
    return std::make_unique<application>();
});
