#include <ros/ros.h>
#include <std_msgs/Float32.h>
#include <std_msgs/String.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <sstream>
#include <regex>

/**
 * Executes a shell command and returns the output as a string
 */
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

/**
 * Signal strength monitor node
 */
class SignalMonitor {
private:
    ros::NodeHandle nh_;
    ros::NodeHandle private_nh_;
    ros::Publisher signal_pub_;
    ros::Publisher quality_pub_;
    ros::Publisher info_pub_;
    ros::Timer timer_;
    
    std::string wifi_interface_;
    double update_rate_;
    
    void timerCallback(const ros::TimerEvent& event) {
        double signal_level = 0.0;
        double quality_percent = 0.0;
        std::string info;
        
        try {
            // Get Wi-Fi information using iwconfig
            std::string command = "iwconfig " + wifi_interface_;
            std::string output = exec(command.c_str());
            
            // Extract signal level using regex
            std::regex signal_regex("Signal level=(-\\d+) dBm");
            std::smatch signal_match;
            if (std::regex_search(output, signal_match, signal_regex) && signal_match.size() > 1) {
                signal_level = std::stod(signal_match[1].str());
            }
            
            // Extract link quality using regex
            std::regex quality_regex("Link Quality=(\\d+)/(\\d+)");
            std::smatch quality_match;
            if (std::regex_search(output, quality_match, quality_regex) && quality_match.size() > 2) {
                double quality_num = std::stod(quality_match[1].str());
                double quality_max = std::stod(quality_match[2].str());
                quality_percent = (quality_num / quality_max) * 100.0;
            }
            
            // Get network info
            std::regex essid_regex("ESSID:\"([^\"]*)\"");
            std::smatch essid_match;
            std::string essid = "Unknown";
            if (std::regex_search(output, essid_match, essid_regex) && essid_match.size() > 1) {
                essid = essid_match[1].str();
            }
            
            std::regex rate_regex("Bit Rate=(\\d+\\.\\d+ \\w+/s)");
            std::smatch rate_match;
            std::string rate = "Unknown";
            if (std::regex_search(output, rate_match, rate_regex) && rate_match.size() > 1) {
                rate = rate_match[1].str();
            }
            
            // Format info string
            std::ostringstream oss;
            oss << "ESSID: " << essid << ", Rate: " << rate 
                << ", Signal: " << signal_level << " dBm, Quality: " << quality_percent << "%";
            info = oss.str();
            
        } catch (const std::exception& e) {
            ROS_ERROR_STREAM("Error getting signal strength: " << e.what());
            info = std::string("Error: ") + e.what();
        }
        
        // Publish data
        std_msgs::Float32 signal_msg;
        signal_msg.data = signal_level;
        signal_pub_.publish(signal_msg);
        
        std_msgs::Float32 quality_msg;
        quality_msg.data = quality_percent;
        quality_pub_.publish(quality_msg);
        
        std_msgs::String info_msg;
        info_msg.data = info;
        info_pub_.publish(info_msg);
        
        // Log data
        ROS_INFO_STREAM(info);
    }
    
public:
    SignalMonitor() : private_nh_("~") {
        // Get parameters
        private_nh_.param<std::string>("interface", wifi_interface_, "wlan0");
        private_nh_.param<double>("update_rate", update_rate_, 1.0);
        
        // Setup publishers
        signal_pub_ = nh_.advertise<std_msgs::Float32>("wifi_signal", 10);
        quality_pub_ = nh_.advertise<std_msgs::Float32>("wifi_quality", 10);
        info_pub_ = nh_.advertise<std_msgs::String>("wifi_info", 10);
        
        // Setup timer
        timer_ = nh_.createTimer(ros::Duration(1.0/update_rate_), &SignalMonitor::timerCallback, this);
        
        ROS_INFO_STREAM("Signal monitor initialized with interface: " << wifi_interface_ 
                       << " at update rate: " << update_rate_ << " Hz");
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "signal_monitor");
    
    SignalMonitor monitor;
    
    ros::spin();
    
    return 0;
} 