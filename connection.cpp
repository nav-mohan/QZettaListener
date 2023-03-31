#include "connection.h"

void Connection::do_read()
{
    m_timer.expires_after(std::chrono::seconds(TIMER_COMPLETION));
    m_timer.async_wait
    (
        [&](boost::system::error_code ec)
        {
            if (ec == boost::system::errc::operation_canceled)
                return;
            
            if(!ec){
                m_messageBuffer->do_process();
                // m_socket.cancel(); // should I persist the connection or should i close it?
            }
        }
    );

    auto self(shared_from_this());
    m_socket.async_read_some
    (
        boost::asio::buffer(m_data,CLIENT_BUFFER_SIZE),
        [this,self](boost::system::error_code ec, size_t bytes)
        {
            if(!ec)
            {
                self->do_store(bytes);
                this->do_read();
            }
            else
                std::cout << ec.message() << ": " << ec.what() << "\n";
        }
    );
}

void Connection::do_store(size_t bytes)
{
    m_messageBuffer->append(m_data,bytes);
}

void Connection::start()
{
    do_read();
}