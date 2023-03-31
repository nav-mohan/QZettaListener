#include "messagebuffer.h"
#include "regexparser.h"

MessageBuffer::MessageBuffer()
{
    m_bufferWrite = (char*)malloc(BUFFER_SIZE);
    m_bufferProcess = (char*)malloc(BUFFER_SIZE);
    m_head = 0;
    m_tail = 0; // this is usefule when parser needs to parse from mid of bufferA and start of bufferB. Not sure how I'd move the remnants though
    m_isProcessing = false;
}

void MessageBuffer::append(const char* data, size_t bytes)
{
    if(bytes+m_head > BUFFER_SIZE)
    {
        std::cout << "append()->do_process()\n";
        do_process();
        m_head = 0;
    }
    memcpy(m_bufferWrite+m_head,data,bytes);
    m_head += bytes;
}

void MessageBuffer::do_process()
{
    std::cout << "do_process\n";
    if(!m_isProcessing.load())
    {
        m_bufferWrite = m_bufferProcess.exchange(m_bufferWrite);
        m_parseThread = std::move(std::thread([this] {this->process(); }));
        m_parseThread.detach();
    }
}

void MessageBuffer::process()
{
    m_isProcessing.store(true); // not available for swap with WB
    std::vector<std::string> results;
    findMatch(m_bufferProcess,&results);
    for(auto str: results)
    {
        boost::property_tree::ptree tree;
        LogEventStruct les;
        if(parseXmlString(str, &tree, &les))
            std::cout << les.LogEventID << "\n";
            std::cout << "\n------------\n";
    }
    m_isProcessing.store(false); // now available for swap with WB
}
