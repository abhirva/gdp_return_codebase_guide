#include <ros/ros.h>
#include <string>

int main(int argc, char** argv)
{
    ros::init(argc, argv, "echo_data_reference");
    ros::NodeHandle nh("~");
    
    std::string summary;
    nh.param<std::string>("summary", summary, "No data reference provided");
    
    // Print the reference information
    ROS_INFO_STREAM("\n" << summary);
    
    // Run once then exit (no need to spin)
    return 0;
} 