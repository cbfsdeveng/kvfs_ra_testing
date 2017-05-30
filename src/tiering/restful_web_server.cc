/*
 * Based on mongoose/examples/restful_server/restful_server.c
 * (see above for html form variable code, when/if needed).
 */

#include "restful_web_server.h"

#include <glog/logging.h>


namespace pqfs {

// Static member function
void RESTfulWebServer::StaticHandleEvent(
        struct mg_connection *connection, int ev, void *ev_data)
{
    auto *rws = static_cast<RESTfulWebServer*>(connection->user_data);
    struct http_message *hm = (struct http_message *) ev_data;
    DVLOG(3) << "StaticHandleEvent: ev " << ev;
    switch (ev) {
        case MG_EV_HTTP_REQUEST:
            // TODO: NEEDSWORK: move to function.
            if (rws->handler != nullptr) {
                assert(hm->uri.len < 10000);
                std::string event_string(hm->uri.p, hm->uri.len);
                std::string result_string("");
                rws->HandleEvent(event_string, &result_string);
                LOG(INFO) << "StaticHandleEvent: result '"
                    << result_string << "'";
                /* Send headers */
                mg_printf(connection, "%s",
                          "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
                mg_printf_http_chunk(connection, "{ \"Result\": \"%s\" }\n",
                        result_string.c_str());
                /* Send empty chunk, the end of response */
                mg_send_http_chunk(connection, "", 0);
                LOG(INFO) << "StaticHandleEvent: returning";
            } else {
                LOG(FATAL) << "StaticHandleEvent: null handler HTTP_REQUEST";
            }
            break;
        default:
            // ev == 0  when our poll call times out (which we currently
            // need to do to check for stop_requested).
            LOG_FIRST_N(INFO, 1) << "StaticHandleEvent: ev=" << ev << " ignored.";
            break;
    }
}

// Set appropriate web server options, bind to the specified port.
// Returns true on success.
bool RESTfulWebServer::Init(
#ifdef RWS_ARGS
                               int argc, char *argv[]
#endif
    )
{
    bool successful = true;
    mg_mgr_init(&mgr, this);

#ifdef RWS_ARGS
    /* Process command line options to customize HTTP server */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-D") == 0 && i + 1 < argc) {
            mgr.hexdump_file = argv[++i];
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            http_opts.document_root = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            http_port = argv[++i];
        } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            http_opts.auth_domain = argv[++i];
#ifdef MG_ENABLE_JAVASCRIPT
        } else if (strcmp(argv[i], "-j") == 0 && i + 1 < argc) {
            const char *init_file = argv[++i];
            mg_enable_javascript(&mgr, v7_create(), init_file);
#endif
        } else if (strcmp(argv[i], "-P") == 0 && i + 1 < argc) {
            http_opts.global_auth_file = argv[++i];
        } else if (strcmp(argv[i], "-A") == 0 && i + 1 < argc) {
            http_opts.per_directory_auth_file = argv[++i];
        } else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            http_opts.url_rewrites = argv[++i];
#ifndef MG_DISABLE_CGI
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            http_opts.cgi_interpreter = argv[++i];
#endif
#ifdef MG_ENABLE_SSL
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            ssl_cert = argv[++i];
#endif
        } else {
            LOG(ERROR) << "Unknown option: [" << argv[i] << "]";
            successful = false;
            goto out;
        }
    }
#endif                          // RWS_ARGS

    /* Set HTTP server options */
    connection = mg_bind(&mgr, http_port.c_str(), StaticHandleEvent);
    if (connection == NULL) {
        LOG(ERROR) << "Error starting server on port " << http_port;
        successful = false;
        goto out;
    }
    // NEEDSWORK: is this allowed? Or should I use mg_bind_opt?
    connection->user_data = static_cast<void*>(this);
#ifdef MG_ENABLE_SSL
    if (ssl_cert != NULL) {
        const char *err_str = mg_set_ssl(connection, ssl_cert, NULL);
        if (err_str != NULL) {
            LOG(ERROR) << "loading SSL cert: " << err_str;
            successful = false;
            goto out;
        }
    }
#endif

    mg_set_protocol_http_websocket(connection);
    http_opts.document_root = ".";
    http_opts.enable_directory_listing = "yes";

#ifdef RWS_ARGS
    /* Use current binary directory as document root */
    if (argc > 0 && ((cp = strrchr(argv[0], '/')) != NULL ||
                     (cp = strrchr(argv[0], '/')) != NULL)) {
        *cp = '\0';
        http_opts.document_root = argv[0];
    }
#else
    // NEEDSWORK: do we want to support static documents?
    // Maybe to browse tarballs, working files, & indexes?
    http_opts.document_root = "/tmp";
#endif
  out:
    if (successful) {
        initialized = true;
    }
    return successful;
}

int RESTfulWebServer::StartPollingThread()
{
    if (polling_thread.get() != nullptr) {
        return EINVAL;
    }
    // starts running immediately.
    stop_requested = false;
    polling_thread.reset(new std::thread(&RESTfulWebServer::RunLoop, this));
    return 0;
}

void RESTfulWebServer::RequestStop()
{
    // NEEDSWORK: synchronize
    stop_requested = true;
    // NEEDSWORK: I think we could use mg_broadcase(...) to handle stop
    // requests, which would cause our mg_mgr_poll to return in our polling
    // thread. (but docs are vague, & not that important to right now).
}

void RESTfulWebServer::Join()
{
    assert(polling_thread);
    polling_thread->join();
}


void RESTfulWebServer::RunLoop()
{
    LOG(INFO) << "Starting RESTful server on port " << http_port;
    while (!stop_requested) {
        // NEEDSWORK: if StopPollingThread used 
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
}

} // namespace pqfs
