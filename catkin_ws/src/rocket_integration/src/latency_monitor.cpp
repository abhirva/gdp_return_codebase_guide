#include <ros/ros.h>
#include <std_msgs/Float64.h>
#include <geometry_msgs/PoseStamped.h>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <sys/stat.h>
#include <errno.h>
#include <iomanip>
#include <sstream>

class LatencyMonitor {
private:
    ros::NodeHandle nh_;
    
    // Subscribers
    ros::Subscriber vicon_latency_sub_;
    ros::Subscriber fc_latency_sub_;
    ros::Subscriber pose_sub_;
    
    // Publishers
    ros::Publisher total_latency_pub_;
    
    // Timestamp of the last vicon pose published
    ros::Time last_vicon_timestamp_;
    
    // Latency metrics
    double vicon_processing_latency_ms_;  // Time from UDP receive to ROS publish
    double fc_processing_latency_ms_;     // Time from pose receive to serial send
    double total_latency_ms_;             // End-to-end system latency
    
    // Log file
    std::ofstream logfile_;
    bool logging_enabled_;
    std::string log_filename_;
    
    // Helper function to create directory if it doesn't exist
    bool createDirectory(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            // Directory doesn't exist, create it
            #ifdef _WIN32
                return mkdir(path.c_str()) == 0;
            #else
                return mkdir(path.c_str(), 0755) == 0;
            #endif
        }
        return true;
    }
    
public:
    LatencyMonitor() : nh_("~") {
        // Get parameters
        bool enable_logging;
        
        nh_.param<bool>("enable_logging", enable_logging, true);
        
        // Initialize subscribers
        vicon_latency_sub_ = nh_.subscribe("/vicon_node/vicon_latency", 10, 
                                &LatencyMonitor::viconLatencyCallback, this);
        fc_latency_sub_ = nh_.subscribe("/flight_controller_node/fc_latency", 10, 
                              &LatencyMonitor::fcLatencyCallback, this);
        pose_sub_ = nh_.subscribe("/vicon/Test1return/pose", 10, 
                       &LatencyMonitor::poseCallback, this);
                       
        // Initialize publishers
        total_latency_pub_ = nh_.advertise<std_msgs::Float64>("total_latency", 10);
        
        // Initialize metrics
        vicon_processing_latency_ms_ = 0.0;
        fc_processing_latency_ms_ = 0.0;
        total_latency_ms_ = 0.0;
        
        // Initialize logging
        logging_enabled_ = enable_logging;
        if (logging_enabled_) {
            // Create latency_logs directory in home directory
            std::string home_dir = getenv("HOME");
            std::string log_dir = home_dir + "/latency_logs";
            
            if (!createDirectory(log_dir)) {
                ROS_ERROR("Failed to create directory %s: %s", log_dir.c_str(), strerror(errno));
                logging_enabled_ = false;
            } else {
                // Create log filename with timestamp
                auto now = std::time(nullptr);
                auto tm = *std::localtime(&now);
                std::ostringstream oss;
                oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
                log_filename_ = log_dir + "/latency_log_" + oss.str() + ".csv";
                
                // Open log file
                logfile_.open(log_filename_);
                if (logfile_.is_open()) {
                    logfile_ << "timestamp,vicon_processing_latency_ms,fc_processing_latency_ms,total_latency_ms" << std::endl;
                    ROS_INFO("Latency logging enabled. Writing to %s", log_filename_.c_str());
                } else {
                    ROS_ERROR("Failed to open latency log file: %s", log_filename_.c_str());
                    logging_enabled_ = false;
                }
            }
        }
        
        ROS_INFO("Latency monitor initialized");
    }
    
    ~LatencyMonitor() {
        if (logfile_.is_open()) {
            logfile_.close();
        }
    }
    
    void viconLatencyCallback(const std_msgs::Float64::ConstPtr& msg) {
        vicon_processing_latency_ms_ = msg->data;
        updateTotalLatency();
    }
    
    void fcLatencyCallback(const std_msgs::Float64::ConstPtr& msg) {
        fc_processing_latency_ms_ = msg->data;
        updateTotalLatency();
    }
    
    void poseCallback(const geometry_msgs::PoseStamped::ConstPtr& msg) {
        last_vicon_timestamp_ = msg->header.stamp;
    }
    
    void updateTotalLatency() {
        // Calculate total system latency (approximation)
        total_latency_ms_ = vicon_processing_latency_ms_ + fc_processing_latency_ms_;
        
        // Publish total latency
        std_msgs::Float64 msg;
        msg.data = total_latency_ms_;
        total_latency_pub_.publish(msg);
        
        // Log if enabled
        if (logging_enabled_ && logfile_.is_open()) {
            logfile_ << std::fixed << std::setprecision(6)
                     << ros::Time::now().toSec() << ","
                     << vicon_processing_latency_ms_ << ","
                     << fc_processing_latency_ms_ << ","
                     << total_latency_ms_ << std::endl;
        }
        
        // Print debug message
        ROS_DEBUG("Latency (ms): Vicon=%.2f, FC=%.2f, Total=%.2f", 
                 vicon_processing_latency_ms_, fc_processing_latency_ms_, total_latency_ms_);
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "latency_monitor");
    
    LatencyMonitor monitor;
    
    ros::spin();
    
    return 0;
} 