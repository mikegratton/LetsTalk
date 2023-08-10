#include <regex>
#include <string>

namespace lt {
/**
 * Hard-coded XML for default profiles. Profiles are:
 *
 *   - reliable (pub and sub): Default. Failed messages are resent. A limited
 *        queue of old messages is kept
 *
 *   - bulk (pub and sub): Best-effort version. Designed for use with
 *        streaming data, where it is more harmful to try to correct errors than
 *        to just keep going.
 *
 *   - stateful (pub and sub): Reliable with history. Designed for scenarios
 *        where stateful commands are sent in order. Late joiners will receive the
 *        full queue.
 */
std::string getDefaultProfileXml()
{
    std::string xml(R"(
<?xml version="1.0" encoding="UTF-8" ?>
<dds xmlns="http://www.eprosima.com/XMLSchemas/fastRTPS_Profiles">
<profiles>
    <participant profile_name="default" is_default_profile="true">
        <rtps>
            <propertiesPolicy>
                <properties>
                    <!-- Avoid local matching of this participant's endpoints -->
                    <property>
                        <name>fastdds.ignore_local_endpoints</name>
                        <value>true</value>
                    </property>
                </properties>
            </propertiesPolicy>
        </rtps>
    </participant>
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
    <data_writer profile_name="reliable" is_default_profile="true">
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
    </data_writer>    
    <data_writer profile_name="bulk" is_default_profile="false">
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
    </data_writer>    
    <data_writer profile_name="stateful" is_default_profile="false">
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
                <kind>SHARED</kind>
            </ownership>        
        </qos>        
        <historyMemoryPolicy>DYNAMIC_REUSABLE</historyMemoryPolicy>
    </data_writer>    
    <data_reader profile_name="reliable" is_default_profile="true">
        <qos>
            <durability>
                <kind>TRANSIENT_LOCAL</kind>
            </durability>
            <reliability>
                <kind>RELIABLE</kind>
            </reliability>        
        </qos>        
        <historyMemoryPolicy>DYNAMIC_REUSABLE</historyMemoryPolicy> 
    </data_reader>    
    <data_reader profile_name="bulk" is_default_profile="false">
        <qos>
            <durability>
                <kind>VOLATILE</kind>
            </durability>
            <reliability>
                <kind>BEST_EFFORT</kind>
            </reliability>        
        </qos>        
        <historyMemoryPolicy>DYNAMIC</historyMemoryPolicy>
    </data_reader>    
    <data_reader profile_name="stateful" is_default_profile="false">
        <qos>
            <durability>
                <kind>TRANSIENT_LOCAL</kind>
            </durability>
            <reliability>
                <kind>RELIABLE</kind>
            </reliability>
            <ownership>
                <kind>SHARED</kind>
            </ownership>        
        </qos>        
        <historyMemoryPolicy>DYNAMIC_REUSABLE</historyMemoryPolicy>
    </data_reader>    
</profiles>
</dds>
)");

    return xml;
}
}  // namespace lt
