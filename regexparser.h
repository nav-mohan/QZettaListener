#if !defined(REGEX_PARSER_H)
#define REGEX_PARSER_H

#include <iostream>
#include <string>
#include <vector>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>


struct LogEventStruct {
    uint64_t LogEventID;
    std::string AirStarttime;
    std::string AirStoptime;
    std::string LogEventType;

    std::string AssetID;
    uint32_t AssetTypeID;
    std::string AssetTypeName;
    std::string AssetFileName;

    std::string Title;
    std::string Comment;

    std::string ArtistName;
    uint32_t ArtistID;
    
    std::string AlbumName;
    uint32_t AlbumID;
    
    std::string ParticipantName;
    uint32_t ParticipantID;

    std::string ProductName;
    uint32_t ProductID;
    
    std::string SponsorName;
    uint32_t SponsorID;

    std::string RwGenre;
    std::string RwReleaseDate;
    uint8_t RwLocal = 0;
    uint8_t RwCanCon = 0;
    uint8_t RwHit = 0;
    uint8_t RwExplicit = 0;
    uint8_t RwFemale = 0;
    uint8_t RwIndigenous = 0;
};

boost::regex pattern("<LogEvent\\s.*?<\\/LogEvent>");

void findMatch(char *text, std::vector<std::string>* regexMatches)
{
    std::string s(text);
    boost::sregex_iterator iter(s.begin(), s.end(), pattern);
    boost::sregex_iterator end;
    while (iter != end)
    {
        regexMatches->emplace_back(std::move(iter->str()));
        ++iter;
    }
}

bool parseXmlString(std::string xmlString, boost::property_tree::ptree *tree, LogEventStruct *les)
{
    bool isAsset = false;
    bool isTask = false;
    bool isPrerecAsset = false;
    bool isWeirdAsset = false;
    bool isLastStarted = false;

    std::stringstream ss(xmlString);
    boost::property_tree::read_xml(ss,*tree);

    // obtaininig <LogEvent> attributes
    auto& LogEvent = tree->get_child("LogEvent");
    les->LogEventID = LogEvent.get<int>("<xmlattr>.LogEventID");
    les->AirStarttime = LogEvent.get<std::string>("<xmlattr>.AirStarttime");
    les->AirStoptime = LogEvent.get<std::string>("<xmlattr>.AirStoptime",les->AirStarttime);

    boost::property_tree::basic_ptree<std::string, std::string> AssetEvent;
    boost::property_tree::basic_ptree<std::string, std::string> Task;

    try
    {
        AssetEvent = LogEvent.get_child("AssetEvent");
        isAsset = true;
        if(les->LogEventID > 0)
        {
            isLastStarted = LogEvent.get<bool>("<xmlattr>.LastStarted",false);
            isPrerecAsset = true;
        } 
        else isWeirdAsset = true;
    }
    catch(boost::wrapexcept<boost::property_tree::ptree_bad_path>)
    {
        try
        {
            Task = LogEvent.get_child("Task");
            isTask = true;
        }
        catch(boost::wrapexcept<boost::property_tree::ptree_bad_path>)
        {
            std::cerr << "Unknwon XML type\n";
            return false;
        }
    }

    if(isPrerecAsset && !isLastStarted) // Asset is stopping
        return false;
    
    if(isWeirdAsset)
    {
        les->LogEventID = std::time(0);
        auto& Asset = AssetEvent.get_child("Asset");
        les->Title = Asset.get<std::string>("<xmlattr>.Title");
        les->ArtistName = Asset.get_child("Artist").get<std::string>("<xmlattr>.Name");
        return true;
    }

    if(isTask)
    {
        les->LogEventID = std::time(0);
        les->Comment = Task.get<std::string>("<xmlattr>.Comment");
        if(!strncmp(les->Comment.c_str(),"LiveMetadata.Send",17))
        {
            les->LogEventType = "LiveMetadata.Send";
            size_t i1 = les->Comment.find("Title: ");
            size_t i2 = les->Comment.find("Artist: ");
            size_t i3 = les->Comment.find("Composer: ");
            size_t i4 = les->Comment.find(" ]");
            if(i1 != -1 && i2 != -1)
                les->Title = les->Comment.substr(i1+7,i2-i1-9);
            if(i2 != -1 && i3 != -1)
                les->ArtistName = les->Comment.substr(i2+8,i3-i2-10);
            if(i3 != -1 && i4 != -1)
                les->AssetTypeName = les->Comment.substr(i3+10,i4-i3-10);

        }
        else if(!strncmp(les->Comment.c_str(),"Sequencer.SetMode",17))
        {
            les->LogEventType = "Sequencer.SetMode";
            if(les->Comment.find("Live Assist") != std::string::npos)
                les->AssetTypeName = "Live Assist";
            if(les->Comment.find("Auto") != std::string::npos)
                les->AssetTypeName = "Auto";
        }
        return true;
    }
    if(isPrerecAsset)
    {
        les->LogEventType = "Prerec. Asset";
        std::cout << "Finding Asset Commenct\n";

        auto& Asset = AssetEvent.get_child("Asset");
        les->Comment = Asset.get<std::string>("<xmlattr>.Comment");
        les->AssetID = Asset.get<uint32_t>("<xmlattr>.AssetID");
        les->AssetTypeID = Asset.get<uint32_t>("<xmlattr>.AssetTypeID");
        les->Title = Asset.get<std::string>("<xmlattr>.Title");
        les->AssetTypeName = Asset.get<std::string>("<xmlattr>.AssetTypeName");

        auto& Resource = Asset.get_child("Resource");
        les->AssetFileName = Resource.get<std::string>("<xmlattr>.ResourceFile");

        for(auto& [name,child] : Asset){
            if(name == "Artist"){
                les->ArtistName = child.get<std::string>("<xmlattr>.Name");
                les->ArtistID = child.get<uint32_t>("<xmlattr>.ArtistID");
            }
            if(name == "Album"){
                les->AlbumName = child.get<std::string>("<xmlattr>.Name");
                les->AlbumID = child.get<uint32_t>("<xmlattr>.AlbumID");
            }
            if(name == "Sponsor"){
                les->SponsorName = child.get<std::string>("<xmlattr>.Name");
                les->SponsorID = child.get<uint32_t>("<xmlattr>.SponsorID");
            }
            if(name == "Product"){
                les->ProductName = child.get<std::string>("<xmlattr>.Name");
                les->ProductID = child.get<uint32_t>("<xmlattr>.ProductID");
            }
            if(name == "Participant"){
                les->ParticipantName = child.get<std::string>("<xmlattr>.Name");
                les->ParticipantID = child.get<uint32_t>("<xmlattr>.ParticipantID");
            }

            if(name=="AssetAttribute"){
                std::string AttributeTypeName = child.get<std::string>("<xmlattr>.AttributeTypeName");
                if(!strncmp("Genre",AttributeTypeName.c_str(),5))
                    les->RwGenre = child.get<std::string>("<xmlattr>.AttributeValueName");
                if(!strncmp("RW Release Date",AttributeTypeName.c_str(),15))
                    les->RwReleaseDate = child.get<std::string>("<xmlattr>.AttributeValueName");
                if(!strncmp("Local",AttributeTypeName.c_str(),5))
                    les->RwLocal = child.get<uint8_t>("<xmlattr>.AttributeValueName");
                if(!strncmp("CanCon",AttributeTypeName.c_str(),6))
                    les->RwCanCon = child.get<uint8_t>("<xmlattr>.AttributeValueName");
                if(!strncmp("Hit",AttributeTypeName.c_str(),3))
                    les->RwHit = child.get<uint8_t>("<xmlattr>.AttributeValueName");
                if(!strncmp("Explicit",AttributeTypeName.c_str(),8))
                    les->RwExplicit = child.get<uint8_t>("<xmlattr>.AttributeValueName");
                if(!strncmp("Female",AttributeTypeName.c_str(),6))
                    les->RwFemale = child.get<uint8_t>("<xmlattr>.AttributeValueName");
                if(!strncmp("Indigenous",AttributeTypeName.c_str(),10))
                    les->RwIndigenous = child.get<uint8_t>("<xmlattr>.AttributeValueName");
            }
        }
    }

    return true;
}

#endif // REGEX_PARSER_H
