#if !defined(CONNECTION_H)
#define CONNECTION_H

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include "messagebuffer.h"

using tcp = boost::asio::ip::tcp;

#define CLIENT_BUFFER_SIZE 1024
#define TIMER_COMPLETION 2

class Connection : public std::enable_shared_from_this<Connection>
{
private:
    void do_read();
    void do_store(size_t bytes);
    tcp::socket m_socket;
    char m_data[CLIENT_BUFFER_SIZE];
    MessageBuffer *m_messageBuffer;
    boost::asio::high_resolution_timer &m_timer;

public:
    Connection(
        tcp::socket soc, 
        boost::asio::high_resolution_timer &timer
    ) 
    : 
    m_socket(std::move(soc)), 
    m_timer(timer),
    m_messageBuffer(new MessageBuffer())
    {;}
    void start();
};


#endif // CONNECTION_H
