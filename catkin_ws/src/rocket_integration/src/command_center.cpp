#include <ros/ros.h>
#include <std_msgs/String.h>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    ros::init(argc, argv, "command_center");
    ros::NodeHandle nh;
    
    // Publisher for flight controller commands
    ros::Publisher pub = nh.advertise<std_msgs::String>("/fc_command", 10);
    
    // Print menu
    std::cout << "\n=== Rocket Flight Command Center ===" << std::endl;
    std::cout << "Available commands:" << std::endl;
    std::cout << "  START - Start the mission" << std::endl;
    std::cout << "  STOP  - Stop the mission" << std::endl;
    std::cout << "  FTS   - Flight Termination System (Emergency Stop)" << std::endl;
    std::cout << "  q     - Quit this command center" << std::endl;
    std::cout << "==================================\n" << std::endl;
    
    ros::Rate rate(10); // 10 Hz
    std::string input;
    
    while (ros::ok())
    {
        // Prompt and get user input
        std::cout << "Command > ";
        std::getline(std::cin, input);
        
        // Check for quit command
        if (input == "q" || input == "Q")
        {
            std::cout << "Exiting command center..." << std::endl;
            break;
        }
        
        // If we have input, publish it
        if (!input.empty())
        {
            // Convert to uppercase for consistency
            for (auto & c: input) c = toupper(c);
            
            std_msgs::String msg;
            msg.data = input;
            pub.publish(msg);
            
            ROS_INFO("Published command: %s", input.c_str());
        }
        
        ros::spinOnce();
        rate.sleep();
    }
    
    std::cout << "Command center terminated" << std::endl;
    return 0;
} 