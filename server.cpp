#include "server.h"

Server::Server(
    boost::asio::io_context& ioContext, 
    short port, 
    boost::asio::high_resolution_timer &timer    
)
: 
m_acceptor(tcp::acceptor(ioContext,tcp::endpoint(tcp::v4(),port))), 
m_timer(timer)
{
    do_accept();
}

void Server::do_accept()
{
    m_acceptor.async_accept
    (
        [this](boost::system::error_code ec, tcp::socket socket)
        {
            if(!ec)
                std::make_shared<Connection>(
                    std::move(socket),
                    m_timer
                    )->start();
            do_accept();
        }
    );
}