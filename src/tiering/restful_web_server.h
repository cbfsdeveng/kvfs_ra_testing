/*
 * Class which implements a simple RESTful web server using Mongoose, an open
 * source embedded web server.
 * (https://github.com/cesanta/mongoose)
 */

#ifndef  TIERING_RESTFUL_SERVER_
#define  TIERING_RESTFUL_SERVER_
#include "mongoose.h"
#include <thread>

namespace pqfs {

class RESTfulWebServer {
  public:
    RESTfulWebServer(const char *http_port     // Port on which to listen for requests.
#ifdef MG_ENABLE_SSL
                     , const char *ssl_cert
#endif
            ) :
     http_port(http_port),
#ifdef MG_ENABLE_SSL
     ssl_cert(ssl_cert),
#endif
     initialized(false),
     stop_requested(false),
     polling_thread(nullptr) {
    }
    // Set appropriate web server options, bind to the specified port.
    // Returns true on success.
    bool Init();

    // Initiates a thread which will call Run, our polling loop.
    int StartPollingThread();
    // Request the polling thread to stop.
    void RequestStop();
    // Bocks to wait for the polling thread to stop.
    void Join();

    struct mg_mgr* get_mgr() { return &mgr; }
    struct mg_connection* get_connection() { return connection; }
    struct mg_serve_http_opts* get_http_opts() { return &http_opts; }

    void set_handler(
            void (*handler)(const std::string&, std::string*, void*),
            void* handler_arg) {
        this->handler = handler;
        this->handler_arg = handler_arg;
    };


  private:
    // Actual polling loop. Runs until stop is requested.
    void RunLoop();
    // Passed to mongoose (must be static or non-member).
    static void StaticHandleEvent(struct mg_connection *connection,
            int ev, void *ev_data);
    void HandleEvent(std::string& event_string, std::string* result_string) {
        (*handler)(event_string, result_string, handler_arg);
    }

    std::string http_port;
    void (*handler)(const std::string&, std::string*, void*);
    void *handler_arg;
    bool initialized;
    volatile bool stop_requested;
#ifdef MG_ENABLE_SSL
    std::strings ssl_cert;
#endif
    std::unique_ptr<std::thread> polling_thread;

    // Mongoose web server state.
    struct mg_mgr mgr;
    struct mg_connection *connection;
    struct mg_serve_http_opts http_opts;
};

}  // namespace pqfs
#endif                          // TIERING_RESTFUL_SERVER_
