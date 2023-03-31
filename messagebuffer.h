#if !defined(MESSAGE_BUFFER_H)
#define MESSAGE_BUFFER_H

#include <atomic>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#define BUFFER_SIZE 16384 // ~16KB

class MessageBuffer
{
private:
    std::atomic<char*> m_bufferWrite;
    std::atomic<char*> m_bufferProcess;
    size_t m_head;
    size_t m_tail;
    std::thread m_parseThread;
    std::atomic<bool> m_isProcessing;
    void process();

public:
    MessageBuffer();
    void append(const char *data, size_t bytes);
    void do_process();
};

#endif // MESSAGE_BUFFER_H
