
#include <boost/beast.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>


using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

int main_http_client(int argc, char** argv)
{
    try
    {
        if (argc != 4 && argc != 5)
        {
            std::cerr <<
            "Usage: http-client-sync <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
            "Example:\n" <<
            "    http-client-sync www.example.com 80 /\n" <<
            "    http-client-sync www.example.com 80 / 1.0\n";
            return EXIT_FAILURE;
        }
        auto const host = argv[1];
        auto const port = argv[2];
        auto const target = argv[3];
        int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;
        
        boost::asio::io_context ioc;
        
        tcp::resolver resolver{ioc};
        tcp::socket socket{ioc};
        
        auto const results = resolver.resolve(host, port);
        boost::asio::connect(socket, results.begin(), results.end());
        
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, "Macintosh;Intel Mac OS X");
        
        http::write(socket, req);
        
        boost::beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(socket, buffer, res);
        
        std::cout << res << std::endl;
        
        boost::system::error_code ec;
        socket.shutdown(tcp::socket::shutdown_both, ec);
        
        if (ec && ec != boost::system::errc::not_connected) {
            throw boost::system::system_error{ec};
        }
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

namespace websocket = boost::beast::websocket;

int main_websocket(int argc, char** argv)
{
    try {
        if(argc != 4)
        {
            std::cerr <<
            "Usage: websocket-client-sync <host> <port> <text>\n" <<
            "Example:\n" <<
            "    websocket-client-sync echo.websocket.org 80 \"Hello, world!\"\n";
            return EXIT_FAILURE;
        }
        
        auto const host = argv[1];
        auto const port = argv[2];
        auto const text = argv[3];
        
        boost::asio::io_context ioc;
        tcp::resolver resolver{ioc};
        websocket::stream<tcp::socket> ws{ioc};
        
        auto const results = resolver.resolve(host, port);
        
        boost::asio::connect(ws.next_layer(), results.begin(), results.end());
        
        ws.handshake(host, "/");
        
        ws.write(boost::asio::buffer(std::string(text)));
        boost::beast::multi_buffer buffer;
        ws.read(buffer);
        
        ws.close(websocket::close_code::normal);
        std::cout << boost::beast::buffers(buffer.data()) << std::endl;
        
        
    } catch (std::exception const& e) {
        std::cerr << "Error:  " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}


int main(int argv, char **argc)
{
//    int num = 4;
//    char* str[] = {"", "www.cpglive.com/", "80", "1.0"};
//    return main_http_client(num, str);
    
    return main_websocket(argv, argc);
//    return 0;
}






















