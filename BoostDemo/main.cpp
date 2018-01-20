
#include <boost/beast.hpp>
#include <boost/asio.hpp>
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

template<class AsyncStream, class CompletionToken>
BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, void(boost::beast::error_code))
async_echo(AsyncStream& stream, CompletionToken&& token);

template<class AsyncStream, class Handler>
class echo_op;

template <class AsyncStream, class CompletionToken>
BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, void(boost::beast::error_code))
async_echo(AsyncStream& stream, CompletionToken&& token)
{
    static_assert(boost::beast::is_async_stream<AsyncStream>::value, "AsyncStream requirements not met");
    boost::asio::async_completion<CompletionToken, void(boost::beast::error_code)> init{token};
    
    echo_op<AsyncStream, BOOST_ASIO_HANDLER_TYPE(CompletionToken, void(boost::beast::error_code))>
    {stream, init.completion_handler}
    (boost::beast::error_code{}, 0);
    
    return init.result.get();
}

template<class AsyncStream, class Handler>
class echo_op
{
    struct state
    {
        AsyncStream& stream;
        int step = 0;
        
        boost::asio::basic_streambuf<typename std::allocator_traits<
        boost::asio::associated_allocator_t<Handler>>:: template rebind_alloc<char>> buffer;
        
        explicit state(Handler& handler, AsyncStream& stream_)
        :stream(stream_),
        buffer((std::numeric_limits<std::size_t>::max)(),
               boost::asio::get_associated_allocator(handler))
        {
            
        }
    };
    
    boost::beast::handler_ptr<state, Handler> p_;
    
public:
    
    echo_op(echo_op&&) = default;
    echo_op(echo_op const&) = default;
    
    template<class DeducedHandler, class... Args>
    echo_op(AsyncStream& stream, DeducedHandler&& handler)
    :p_(std::forward<DeducedHandler>(handler), stream)
    {
        
    }
    
    using allocator_type = boost::asio::associated_allocator_t<Handler>;
    
    allocator_type get_allocator() const noexcept
    {
        return boost::asio::get_associated_allocator(p_.handler());
    }
    
    using executor_type = boost::asio::associated_executor_t<
    Handler, decltype(std::declval<AsyncStream&>().get_executor())>;
    
    executor_type get_executor() const noexcept
    {
        return boost::asio::get_associated_executor(p_.handler(), p_.stream.get_executor());
    }
    
    void operator()(boost::beast::error_code ec, std::size_t bytes_transferred);
};

template<class AsyncStream, class Handler>
void echo_op<AsyncStream, Handler>::
operator()(boost::beast::error_code ec, std::size_t bytes_transferred)
{
    auto& p = *p_;
    
    switch (ec ? 2 : p.step)
    {
        case 0:
            p.step = 1;
            return boost::asio::async_read_until(p.stream, p.buffer, "\r", std::move(*this));
        case 1:
            p.step = 2;
            return boost::asio::async_write(p.stream, boost::beast::buffers_prefix(bytes_transferred, p.buffer.data()), std::move(*this));
        case 2:
            p.buffer.consume(bytes_transferred);
            break;
    }
    
    p_.invoke(ec);
    return;
}

int main(int argv, char **argc)
{
//    int num = 4;
//    char* str[] = {"", "www.cpglive.com/", "80", "1.0"};
//    return main_http_client(num, str);
    
    return main_websocket(argv, argc);
//    return 0;
}






















