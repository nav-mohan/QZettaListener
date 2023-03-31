#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "connection.h"

using boost::asio::ip::tcp;

class Server
{
private:
    void do_accept();
    tcp::acceptor m_acceptor;
    boost::asio::high_resolution_timer &m_timer;

public:
    Server(
        boost::asio::io_context& ioContext, short port, 
        boost::asio::high_resolution_timer &timer    
    );
    Server(const Server & srv) = delete;
    Server(const Server &&srv) = delete;
};