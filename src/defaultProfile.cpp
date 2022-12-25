#include <string>
#include <regex>

namespace lt {
namespace detail {
std::string getDefaultProfileXml(char const* i_participantName) {
    std::string xml(R"(
<?xml version="1.0" encoding="UTF-8" ?>
<dds>
<profiles xmlns="http://www.eprosima.com/XMLSchemas/fastRTPS_Profiles">
    <transport_descriptors>
        <transport_descriptor>
            <transport_id>UDP</transport_id>
            <type>UDPv4</type>
        </transport_descriptor>
        <transport_descriptor>
            <transport_id>TCP</transport_id>
            <type>TCPv4</type>
        </transport_descriptor>
    </transport_descriptors>
    <participant profile_name="paricipate" is_default_profile="true">
        <domainId>0</domainId>
        <rtps>
            <name>@LT_NAME@</name>
       </rtps>
    </participant>
    <publisher profile_name="reliable" is_default_profile="true">
        <qos>
            <durability>
                <kind>TRANSIENT_LOCAL</kind>
            </durability>
            <reliability>
                <kind>RELIABLE</kind>
            </reliability>
            <publishMode>
                <kind>ASYNCHRONOUS</kind>
            </publishMode>        
        </qos>        
        <historyMemoryPolicy>DYNAMIC_REUSABLE</historyMemoryPolicy>
    </publisher>    
    <publisher profile_name="bulk" is_default_profile="false">
        <qos>
            <durability>
                <kind>VOLATILE</kind>
            </durability>
            <reliability>
                <kind>BEST_EFFORT</kind>
            </reliability>
            <publishMode>
                <kind>ASYNCHRONOUS</kind>
            </publishMode>
        </qos>        
        <historyMemoryPolicy>DYNAMIC</historyMemoryPolicy>
    </publisher>    
    <publisher profile_name="stateful" is_default_profile="false">
        <qos>
            <durability>
                <kind>TRANSIENT_LOCAL</kind>
            </durability>
            <reliability>
                <kind>RELIABLE</kind>
            </reliability>
            <publishMode>
                <kind>ASYNCHRONOUS</kind>
            </publishMode>
            <ownership>
                <kind>EXCLUSIVE</kind>
            </ownership>        
        </qos>        
        <historyMemoryPolicy>DYNAMIC_REUSABLE</historyMemoryPolicy>
    </publisher>    
    <subscriber profile_name="reliable" is_default_profile="true">
        <qos>
            <durability>
                <kind>TRANSIENT_LOCAL</kind>
            </durability>
            <reliability>
                <kind>RELIABLE</kind>
            </reliability>        
        </qos>        
        <historyMemoryPolicy>DYNAMIC_REUSABLE</historyMemoryPolicy> 
    </subscriber>    
    <subscriber profile_name="bulk" is_default_profile="false">
        <qos>
            <durability>
                <kind>VOLATILE</kind>
            </durability>
            <reliability>
                <kind>BEST_EFFORT</kind>
            </reliability>        
        </qos>        
        <historyMemoryPolicy>DYNAMIC</historyMemoryPolicy>
    </subscriber>    
    <subscriber profile_name="stateful" is_default_profile="false">
        <qos>
            <durability>
                <kind>TRANSIENT_LOCAL</kind>
            </durability>
            <reliability>
                <kind>RELIABLE</kind>
            </reliability>
            <ownership>
                <kind>EXCLUSIVE</kind>
            </ownership>        
        </qos>        
        <historyMemoryPolicy>DYNAMIC_REUSABLE</historyMemoryPolicy>
    </subscriber>    
</profiles>
</dds>
)");
    
    std::regex findName("@LT_NAME@");
    xml = std::regex_replace(xml, findName, i_participantName);
    return xml;
    
}
}
}
