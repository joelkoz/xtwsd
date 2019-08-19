#ifndef _httpclient_h
#define _httpclient_h
//
// Synchronous Http Client using Boost Beast
// This provides a Facade around the Boost Beast http library
// to simplify client side requests
//

#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>


class HttpClient {

    public:

        /**
         * Response return by a request.  Content body can be retrieved using getBody()
         * or asJson().  Individual header fields can be accessed using one of the
         * two following examples:
         * 
         *    // List all headers returned...
         *    for(auto const& field : *pResp)
         *        std::cout << field.name() << " = " << field.value() << "\n";
         * 
         *    // Access a specific field value:
         *    std::cout << "Server: " << (*pResp)[boost::beast::http::field::server] << "\n";        
         */
        class Response : public boost::beast::http::response<boost::beast::http::dynamic_body> {
            public:
               Response() : boost::beast::http::response<boost::beast::http::dynamic_body>() {
               }


               /**
                * Returns TRUE if the response result is a status 200
                */
               bool ok() { return this->result_int() == 200; }


               /**
                * Retrieves the content body of the response and returns it as a string
                */
               std::string getBody();

               /**
                * Converts the body of this response into a json object 
                */
               nlohmann::json asJson();
        };

        HttpClient(const char* host, const char* protocol = "http", const char* port = NULL);

        /**
         * Does an HTTP(S) GET on the specified path.
         */
        std::shared_ptr<Response> get(const std::string& path);


        /**
         * Does an HTTP(S) POST on the specified path, passing body in verbatim. If
         * your are using any specific encoding be sure to add the appropriate header(s) 
         * with setRequestHeader() 
         * Example: 
         *    setRequestHeader(boost::beast::http::field::content_type, "application/x-www-form-urlencoded");
         */
        std::shared_ptr<Response> post(const std::string& path, const std::string& body);



        /**
         * Does an HTTP(S) POST on the specified path, passing the json body as part of the
         * request.  The content type header is set to "application/json" before the post
         * is made.
         */
        std::shared_ptr<Response> post(const std::string& path, const nlohmann::json& body);


        /**
         * Adds the specified name/value pair as a query parameter added to the request
         * path of the next request made by this class
         */
        void setQueryParameter(const std::string&name, const std::string& value);


        /**
         * Adds the specified request key/value pair to the next request
         */
        void setRequestHeader(boost::beast::http::field key, const std::string& value);

    private:
        std::shared_ptr<Response> exec(boost::beast::http::verb action, const std::string& path, const std::string& requestBody);
        std::shared_ptr<Response> execs(boost::beast::http::verb action, const std::string& path, const std::string& requestBody);
        std::shared_ptr<boost::beast::http::request<boost::beast::http::string_body>> getRequest(boost::beast::http::verb action, const std::string& path, const std::string& requestBody);
        std::string host;
        std::string port;
        std::string protocol;

        std::map<boost::beast::http::field, std::string> requestHeaders;
        std::map<std::string, std::string> queryParams;

};


#endif