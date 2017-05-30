/*
 * Implementation of WebController.
 * Creates a restful web server, and handles requests by translating them to
 * api calls on a tiering out agent.
 */
#include "eviction_agent.h"
#include "index_manager.h"
#include "segment.h"
#include "tiering_out_agent.h"
#include "web_controller.h"

#include <glog/logging.h>
#include <google/protobuf/text_format.h>


#include <assert.h>
#include <stdio.h>

#include <thread>

// TODO(joe): IWYU

using google::protobuf::TextFormat;

namespace pqfs {

// TODO(): what does web controller need access to?
// what about the change log? key-value-store?
WebController::WebController(
        TieringOutAgent *tiering_out_agent,
        EvictionAgent* eviction_agent,  // not owned.
        int tcp_port) {
    this->tiering_out_agent = tiering_out_agent;
    this->eviction_agent = eviction_agent;
    this->tcp_port = tcp_port;
}

int WebController::Start() {
    LOG(INFO) << "WebController::Start tcp_port " << tcp_port;
    web_server.reset(new RESTfulWebServer(std::to_string(tcp_port).c_str()));
    web_server->set_handler(&Handler, static_cast<void*>(this));
    web_server->Init();
    web_server->StartPollingThread();
    LOG(INFO) << "waiting for commands on web api (localhost:" << tcp_port << ")";
    return 0;
}

int WebController::RequestStop() {
    // NEEDSWORK: synchronize
    stop_requested = true;
    if (web_server.get() == nullptr) {
        LOG(ERROR) << "RequestStop: not started?";
        return EINVAL;  // not Started?
    }
    web_server->RequestStop();
    return 0;
}

void WebController::Join() {
    assert(web_server.get() != nullptr);
    web_server->Join();
}

// static class member function
void WebController::Handler(
        const std::string& event_string,
        std::string* result_string,
        void* handler_arg) {    // handler_arg is the controller requested
                                //(presumably determined by port #).
    WebController* controller = static_cast<WebController*>(handler_arg);
    controller->HandleWebEvent(event_string, result_string);
}

void WebController::SetXidsInResult(std::ostringstream& result) {
    auto* fs_change_log = tiering_out_agent->get_fs_change_log();

    result << " tiered_out_xid=" << fs_change_log->get_tiered_out_xid()
        << " log_last_xid=" << fs_change_log->get_last_xid()
        << " log_next_xid=" << fs_change_log->get_next_xid()
        << " eviction_next_xid=" << fs_change_log->get_eviction_next_xid();
}

// TODO(joe): make this less hand-crafted (wants to return a set of values, so
// maybe a map is produced and translated into json on return?
void WebController::HandleWebEvent(
        const std::string& event_string,  // The path part of the url (for now)
        std::string* result_string) {
    std::ostringstream result;
    result << "event_string=" << event_string;
    LOG(INFO) << "HandleWebEvent: " << event_string;
    if (event_string == "/STOP") {
        tiering_out_agent->RequestStop();
        result << "Stop requested";
    } else if (event_string == "/SNAP") {
        tiering_out_agent->RequestSnapshot();
        SetXidsInResult(result);
    } else if (event_string == "/STAT") {
        SetXidsInResult(result);
    } else if (event_string == "/EVICT") {
        SetXidsInResult(result);
        eviction_agent->RequestEviction();
    } else if (event_string == "/DUMP") {
        tiering_out_agent->DumpKVStore(result);
    } else if (event_string == "/DBSTATS") {
        tiering_out_agent->DBStats(result);
    } else {
        result << "Unknown command: '" + event_string + "'";
    }
    *result_string = result.str();
    VLOG(2) << "HandleWebEvent: result for " << event_string
            << "\n" << result.str();
}

} // namespace pqfs
