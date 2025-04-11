#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <map>
#include <sys/stat.h>
#include <errno.h>

class ViconDataRecorder {
private:
    std::ofstream csv_file;
    std::string csv_filename;
    std::map<std::string, ros::Subscriber> subscribers;

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

    void callback(const geometry_msgs::PoseStamped::ConstPtr& msg, const std::string& object_name) {
        // Write data to CSV file
        csv_file << std::fixed << std::setprecision(6)
                << msg->header.stamp.toSec() << ","
                << object_name << ","
                << msg->pose.position.x << ","
                << msg->pose.position.y << ","
                << msg->pose.position.z << ","
                << msg->pose.orientation.x << ","
                << msg->pose.orientation.y << ","
                << msg->pose.orientation.z << ","
                << msg->pose.orientation.w << std::endl;
    }

public:
    ViconDataRecorder() {
        ros::NodeHandle nh("~");

        // Create data directory
        std::string home_dir = getenv("HOME");
        std::string data_dir = home_dir + "/vicon_data";
        
        if (!createDirectory(data_dir)) {
            ROS_ERROR("Failed to create directory %s: %s", data_dir.c_str(), strerror(errno));
            return;
        }

        // Create CSV filename with timestamp
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
        csv_filename = data_dir + "/vicon_data_" + oss.str() + ".csv";

        // Open CSV file and write header
        csv_file.open(csv_filename);
        if (!csv_file.is_open()) {
            ROS_ERROR("Failed to open CSV file %s: %s", csv_filename.c_str(), strerror(errno));
            return;
        }
        csv_file << "timestamp,object_name,x,y,z,qx,qy,qz,qw\n";

        // Get list of Vicon objects from parameter server
        std::vector<std::string> vicon_objects;
        nh.param<std::vector<std::string>>("vicon_objects", vicon_objects, std::vector<std::string>{"Test1return"});

        // Create subscribers for each Vicon object
        for (const auto& obj_name : vicon_objects) {
            std::string topic = "/vicon/" + obj_name + "/pose";
            ROS_INFO("Subscribing to %s", topic.c_str());
            
            subscribers[obj_name] = nh.subscribe<geometry_msgs::PoseStamped>(
                topic, 1,
                boost::bind(&ViconDataRecorder::callback, this, _1, obj_name)
            );
        }

        ROS_INFO("Recording Vicon data to %s", csv_filename.c_str());
        ROS_INFO("Waiting for data to start streaming...");
    }

    ~ViconDataRecorder() {
        if (csv_file.is_open()) {
            csv_file.close();
        }
    }
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "vicon_data_recorder");
    ViconDataRecorder recorder;
    ros::spin();
    return 0;
} 