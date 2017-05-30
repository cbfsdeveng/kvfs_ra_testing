/*
 * WebController:
 * Creates a web-server and translates RESTful commands into operations on a
 * tiering agent (associated with a filesystem).
 */
#ifndef WEB_CONTROLLER_
#define WEB_CONTROLLER_


#include "eviction_agent.h"
#include "restful_web_server.h"
#include <thread>

namespace pqfs {

class TieringOutAgent;
class EvictionAgent;

class WebController {
public:
    /**
     * Normal use:
     * contruct,
     * start,
     * ...run for a while...
     * request-stop
     * join (blocks).
     */
    WebController(
            TieringOutAgent* tiering_out_agent,  // not owned.
            EvictionAgent* eviction_agent,  // not owned.
            int tcp_port);
    
    // Start the web controller.
    int Start();

    // These functions allow external control of the TOA.
    int RequestStop();

    void Join();

private:
    TieringOutAgent* tiering_out_agent;  // not owned.
    EvictionAgent* eviction_agent;  // not owned.
    std::unique_ptr<RESTfulWebServer> web_server;
    int tcp_port;

    volatile bool stop_requested;

    void HandleWebEvent(
            const std::string& event_string,
            std::string* result_string);

    // Invokes HandleWebEvent member function of "this" (handler_arg).
    static void Handler(
            const std::string& event_string,
            std::string* result_string,
            void* handler_arg);

    /**
     * put info about the xid related state of the filesystem and it's
     * tiering  agent into the specified stringstream.
     */
    void SetXidsInResult(std::ostringstream& result);
};

}  // pqfs
#endif // WEB_CONTROLLER_
