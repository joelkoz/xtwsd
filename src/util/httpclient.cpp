#include "httpclient.h"
#include <map>
#include <memory>
#include <string>
#include <iostream>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <iomanip>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include "root_certificates.hpp"

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
namespace ssl = net::ssl;

using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>
using namespace std;

// found at https://stackoverflow.com/questions/154536/encode-decode-urls-in-c
string url_encode(const string &value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char) c);
        escaped << nouppercase;
    }

    return escaped.str();
}


string HttpClient::Response::getBody() {
    std::string body { boost::asio::buffers_begin(this->body().data()),
                       boost::asio::buffers_end(this->body().data()) };
    return body;
}


nlohmann::json HttpClient::Response::asJson() {
   return nlohmann::json::parse(this->getBody());
}


HttpClient::HttpClient(const char* host, const char* protocol, const char* port) :
    host{host}, protocol{protocol} {

    if (port == NULL) {
        if (strcmp(protocol, "https") == 0) {
            this->port = "443";
        }
        else {
            this->port = "80";
        }
    }
    else {
        this->port = port;
    }

}


std::shared_ptr<HttpClient::Response> HttpClient::get(const string& path) {
    string body;
    if (protocol == "https") {
        return execs(http::verb::get, path, body);
    }
    else {
        return exec(http::verb::get, path, body);
    }
}


std::shared_ptr<HttpClient::Response> HttpClient::post(const string& path, const string& body) {
    if (protocol == "https") {
        return execs(http::verb::post, path, body);
    }
    else {
        return exec(http::verb::post, path, body);
    }
}


std::shared_ptr<HttpClient::Response> HttpClient::post(const string& path, const nlohmann::json& body) {
    setRequestHeader(http::field::content_type, "application/json");
    return post(path, body.dump());
}



void HttpClient::setQueryParameter(const string&name, const string& value) {
    queryParams.emplace(name, value);
}



void HttpClient::setRequestHeader(http::field field, const string& value) {
    requestHeaders.emplace(field, value);
}



shared_ptr<http::request<http::string_body>> HttpClient::getRequest(http::verb action, const string& path, const string& requestBody) {

       string fullPath = path;

        if (queryParams.size() > 0) {
            fullPath += "?";
            bool firstOut = false;
            for (auto entry : queryParams) {
                auto key = entry.first;
                auto value = entry.second;
                if (firstOut) {
                    fullPath += "&";
                }
                else {
                    firstOut = true;
                }
                fullPath += key;
                fullPath += "=";
                fullPath += url_encode(value);
            }
            queryParams.clear();
        }

        shared_ptr<http::request<http::string_body>> pReq(new http::request<http::string_body>{action, fullPath, 11});
        pReq->set(http::field::host, host);
        pReq->set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Output the request headers...
        auto it = requestHeaders.begin();
        for (auto entry : requestHeaders) {
            auto key = entry.first;
            auto value = entry.second;
            pReq->set(key, value);
        }

        // Clear the headers for the next time around...
        requestHeaders.clear();

        pReq->body() = requestBody;
        pReq->prepare_payload();

        return pReq;
}


// Modified version of https://www.boost.org/doc/libs/develop/libs/beast/example/http/client/sync/http_client_sync.cpp
std::shared_ptr<HttpClient::Response> HttpClient::exec(http::verb action, const string& path, const string& requestBody) {

        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        stream.connect(results);

        // Set up the request message
        shared_ptr<http::request<http::string_body>> pReq = getRequest(action, path, requestBody);

        // Send the HTTP request to the remote host
        http::write(stream, *pReq);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        std::shared_ptr<HttpClient::Response> pResp(new HttpClient::Response());

        // Receive the HTTP response
        http::read(stream, buffer, *pResp);

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        //
        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};

        return pResp;

}


// Modified version of https://www.boost.org/doc/libs/develop/libs/beast/example/http/client/sync-ssl/http_client_sync_ssl.cpp
std::shared_ptr<HttpClient::Response> HttpClient::execs(http::verb action, const string& path, const string& requestBody) {

        printf("   %s: %s\n", action == http::verb::post ? "POST" : "GET", path.c_str());

        // The SSL context is required, and holds certificates
        ssl::context ctx(ssl::context::tlsv12_client);

        // This holds the root certificate used for verification
        load_root_certificates(ctx);

        // Verify the remote server's certificate
        ctx.set_verify_mode(ssl::verify_peer);


        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);


        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream).connect(results);

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);

        // Set up the request message
        shared_ptr<http::request<http::string_body>> pReq = getRequest(action, path, requestBody);

        // Send the HTTP request to the remote host
        http::write(stream, *pReq);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        std::shared_ptr<HttpClient::Response> pResp(new HttpClient::Response());

        // Receive the HTTP response
        http::read(stream, buffer, *pResp);

        // Gracefully close the stream
        beast::error_code ec;
        stream.shutdown(ec);
        if(ec == net::error::eof)
        {
            ec = {};
        }
        if(ec)
            throw beast::system_error{ec};

        return pResp;
}


