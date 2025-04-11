#include <ros/ros.h>
#include <std_msgs/Float64MultiArray.h>
#include <std_msgs/String.h>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <string>
#include <boost/filesystem.hpp>
#include <pwd.h>
#include <unistd.h>

class CsvRecorder {
private:
    ros::NodeHandle nh_;
    ros::Subscriber data_sub_;
    ros::Subscriber telemetry_sub_;
    std::string output_dir_;
    std::string run_name_;
    std::string data_filename_;
    std::string telemetry_filename_;
    std::ofstream data_file_;
    std::ofstream telemetry_file_;
    bool first_data_received_;
    ros::Time start_time_;
    bool auto_timestamp_;
    
public:
    CsvRecorder() : nh_("~"), first_data_received_(false) {
        // Load parameters
        nh_.param<std::string>("output_dir", output_dir_, "~/flight_data");
        nh_.param<std::string>("run_name", run_name_, "flight_test");
        nh_.param<bool>("auto_timestamp", auto_timestamp_, true);
        
        // Create output directory if it doesn't exist
        boost::filesystem::path dir_path(expandPath(output_dir_));
        if (!boost::filesystem::exists(dir_path)) {
            ROS_INFO("Creating output directory: %s", dir_path.c_str());
            boost::filesystem::create_directories(dir_path);
        }
        
        // Generate filenames
        std::string timestamp_str;
        if (auto_timestamp_) {
            // Get current timestamp for filenames
            std::time_t t = std::time(nullptr);
            std::tm tm = *std::localtime(&t);
            std::stringstream timestamp;
            timestamp << std::put_time(&tm, "%Y%m%d_%H%M%S");
            timestamp_str = "_" + timestamp.str();
        } else {
            timestamp_str = "";
        }
        
        // Create filenames with or without timestamp
        data_filename_ = dir_path.string() + "/" + run_name_ + "_data" + timestamp_str + ".csv";
        telemetry_filename_ = dir_path.string() + "/" + run_name_ + "_telemetry" + timestamp_str + ".csv";
        
        // Open files and write headers
        data_file_.open(data_filename_);
        if (data_file_.is_open()) {
            data_file_ << "timestamp,elapsed_time,";
            // Add headers for each data value
            data_file_ << "vicon_pos_x,vicon_pos_y,vicon_pos_z,";
            data_file_ << "euler_roll,euler_pitch,euler_yaw,";
            data_file_ << "est_pos_x,est_pos_y,est_pos_z,";
            data_file_ << "est_vel_x,est_vel_y,est_vel_z,";
            data_file_ << "imu_rate_x,imu_rate_y,imu_rate_z,";
            data_file_ << "control_1,control_2,control_3";
            data_file_ << std::endl;
            ROS_INFO("Opened data CSV file: %s", data_filename_.c_str());
        } else {
            ROS_ERROR("Failed to open data CSV file: %s", data_filename_.c_str());
        }
        
        telemetry_file_.open(telemetry_filename_);
        if (telemetry_file_.is_open()) {
            telemetry_file_ << "timestamp,elapsed_time,message" << std::endl;
            ROS_INFO("Opened telemetry CSV file: %s", telemetry_filename_.c_str());
        } else {
            ROS_ERROR("Failed to open telemetry CSV file: %s", telemetry_filename_.c_str());
        }
        
        // Subscribe to data and telemetry topics
        data_sub_ = nh_.subscribe("/flight_controller_node/data", 10, &CsvRecorder::dataCallback, this);
        telemetry_sub_ = nh_.subscribe("/flight_controller_node/telemetry", 10, &CsvRecorder::telemetryCallback, this);
        
        ROS_INFO("CSV recorder initialized for run: %s", run_name_.c_str());
        ROS_INFO("Auto-timestamping: %s", auto_timestamp_ ? "enabled" : "disabled");
    }
    
    ~CsvRecorder() {
        // Close files
        if (data_file_.is_open()) {
            data_file_.close();
            ROS_INFO("Closed data CSV file: %s", data_filename_.c_str());
        }
        
        if (telemetry_file_.is_open()) {
            telemetry_file_.close();
            ROS_INFO("Closed telemetry CSV file: %s", telemetry_filename_.c_str());
        }
    }
    
    void dataCallback(const std_msgs::Float64MultiArray::ConstPtr& msg) {
        if (!data_file_.is_open()) return;
        
        ros::Time now = ros::Time::now();
        
        // Initialize start time if this is the first data point
        if (!first_data_received_) {
            start_time_ = now;
            first_data_received_ = true;
        }
        
        // Calculate elapsed time
        double elapsed = (now - start_time_).toSec();
        
        // Write timestamp and elapsed time
        data_file_ << now.toSec() << "," << elapsed << ",";
        
        // Write all data values, ensuring we don't exceed array bounds
        for (size_t i = 0; i < msg->data.size(); ++i) {
            data_file_ << msg->data[i];
            if (i < msg->data.size() - 1) {
                data_file_ << ",";
            }
        }
        
        // If we received fewer than 18 values, fill the rest with empty values
        for (size_t i = msg->data.size(); i < 18; ++i) {
            data_file_ << ",";
        }
        
        data_file_ << std::endl;
    }
    
    void telemetryCallback(const std_msgs::String::ConstPtr& msg) {
        if (!telemetry_file_.is_open()) return;
        
        ros::Time now = ros::Time::now();
        
        // Initialize start time if this is the first data point
        if (!first_data_received_) {
            start_time_ = now;
            first_data_received_ = true;
        }
        
        // Calculate elapsed time
        double elapsed = (now - start_time_).toSec();
        
        // Escape commas in the message to prevent CSV format issues
        std::string escaped_msg = msg->data;
        for (size_t i = 0; i < escaped_msg.size(); ++i) {
            if (escaped_msg[i] == ',') {
                escaped_msg.replace(i, 1, ";");
            }
        }
        
        // Write timestamp, elapsed time, and message
        telemetry_file_ << now.toSec() << "," << elapsed << "," << escaped_msg << std::endl;
    }
    
    // Expand ~ in path to home directory
    std::string expandPath(const std::string& path) {
        if (path.empty() || path[0] != '~') {
            return path;
        }
        
        const char* home = getenv("HOME");
        if (home == nullptr) {
            home = getpwuid(getuid())->pw_dir;
        }
        
        return std::string(home) + path.substr(1);
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "csv_recorder");
    
    CsvRecorder recorder;
    
    ros::spin();
    
    return 0;
} 