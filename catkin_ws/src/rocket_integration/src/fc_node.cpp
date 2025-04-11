#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <std_msgs/String.h>
#include <std_msgs/Float64.h>
#include <std_msgs/Float64MultiArray.h>
#include <thread>
#include <mutex>
#include <atomic>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

// For serial comm
#include <fcntl.h>      // File control
#include <termios.h>    // POSIX terminal control
#include <unistd.h>     // close()
#include <sys/select.h> // select()

int serial_fd = -1; // global or in a class
ros::Publisher debug_pub; // For debug output
ros::Publisher latency_pub; // For latency measurements
ros::Publisher fc_telemetry_pub; // For string telemetry from FC
ros::Publisher fc_data_pub; // For array data from FC

std::thread serial_read_thread;
std::atomic<bool> should_exit{false};
std::mutex serial_mutex;

// Add a flag to indicate when a command is being sent
std::atomic<bool> command_in_progress{false};
std::atomic<ros::Time> last_command_time{ros::Time(0)};

// Forward declarations
void processSerialData(const uint8_t* data, size_t length);

bool openSerial(const std::string &portName, int baudrate)
{
    // Allow for dummy port for testing
    if (portName == "/dev/null") {
        ROS_INFO("Using dummy serial port for testing");
        return true;
    }
    
    serial_fd = open(portName.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (serial_fd < 0)
    {
        ROS_ERROR("Failed to open serial port %s", portName.c_str());
        return false;
    }

    struct termios tty;
    if (tcgetattr(serial_fd, &tty) < 0) {
        ROS_ERROR("Error from tcgetattr");
        close(serial_fd);
        return false;
    }

    speed_t baud;
    switch (baudrate) {
        case 115200: baud = B115200; break;
        case 57600:  baud = B57600;  break;
        case 9600:   baud = B9600;   break;
        default:
            ROS_ERROR("Unsupported baudrate %d, defaulting to 115200", baudrate);
            baud = B115200;
            break;
    }

    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);

    // 8N1 config
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~PARENB; // no parity
    tty.c_cflag &= ~CSTOPB; // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;// no hardware flow control
    tty.c_cflag |= (CLOCAL | CREAD);

    // Non-canonical, no echo
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    tcsetattr(serial_fd, TCSANOW, &tty);
    tcflush(serial_fd, TCIOFLUSH);

    return true;
}

void poseCallback(const geometry_msgs::PoseStamped::ConstPtr &msg)
{
    // Check if a command is currently being processed - if so, skip this pose update
    if (command_in_progress) {
        ROS_DEBUG("Skipping pose update while command is being processed");
        return;
    }
    
    // Also check if we're within the "quiet period" after a command (100ms)
    ros::Time now = ros::Time::now();
    double time_since_cmd = (now - last_command_time.load()).toSec();
    if (time_since_cmd < 0.1) {  // 100ms quiet period after commands
        ROS_DEBUG("In quiet period after command (%.2f ms), skipping pose update", time_since_cmd * 1000.0);
        return;
    }
    
    // Calculate end-to-end latency from ROS timestamp to now
    double latency_ms = (now - msg->header.stamp).toSec() * 1000.0;
    
    // Publish latency for monitoring
    std_msgs::Float64 latency_msg;
    latency_msg.data = latency_ms;
    latency_pub.publish(latency_msg);
    
    // Extract position
    double px = msg->pose.position.x;
    double py = msg->pose.position.y;
    double pz = msg->pose.position.z;

    // Extract orientation
    double ox = msg->pose.orientation.x;
    double oy = msg->pose.orientation.y;
    double oz = msg->pose.orientation.z;
    double ow = msg->pose.orientation.w;

    // Example CSV: "POSE,px,py,pz,ox,oy,oz,ow\n"
    std::ostringstream oss;
    oss << "POSE," 
        << px << "," << py << "," << pz << ","
        << ox << "," << oy << "," << oz << "," << ow << "\n";
    std::string outStr = oss.str();

    // Publish debug info even if not sending to serial
    std_msgs::String debug_msg;
    debug_msg.data = outStr;
    debug_pub.publish(debug_msg);

    if (serial_fd >= 0)
    {
        // Lock access to the serial port
        std::lock_guard<std::mutex> lock(serial_mutex);
        ssize_t bytes_written = write(serial_fd, outStr.c_str(), outStr.size());
        ROS_DEBUG("Sent %zd bytes to FC: %s", bytes_written, outStr.c_str());
    }
    
    // Log latency information
    ROS_DEBUG("Vicon data latency: %.2f ms (from Vicon node to FC node)", latency_ms);
}

void cmdCallback(const std_msgs::String::ConstPtr &msg)
{
    // Set command in progress flag to block POSE updates
    command_in_progress.store(true);
    
    // Add a small delay to avoid collision with pose data
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Use original command format
    std::string cmd = "CMD," + msg->data + "\n";
    
    // Pause pose updates temporarily when sending commands
    {
        std::lock_guard<std::mutex> lock(serial_mutex);
        
        // Optionally flush any pending data before sending command
        if (serial_fd >= 0) {
            tcflush(serial_fd, TCOFLUSH);
        }
        
        // Publish debug info even if not sending to serial
        std_msgs::String debug_msg;
        debug_msg.data = cmd;
        debug_pub.publish(debug_msg);
        
        if (serial_fd >= 0)
        {
            // Send the command 3 times with short pauses between to improve reliability
            for (int i = 0; i < 3; i++) {
                ssize_t bytes_written = write(serial_fd, cmd.c_str(), cmd.size());
                ROS_INFO("Sent %zd bytes command to FC (attempt %d/3): %s", bytes_written, i+1, cmd.c_str());
                
                // Small delay between retries
                if (i < 2) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
            }
            
            // Add delay after sending to ensure it's processed
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        else
        {
            ROS_INFO("Would send to FC: %s", cmd.c_str());
        }
    }
    
    // Record the time of this command
    last_command_time.store(ros::Time::now());
    
    // Release command in progress flag
    command_in_progress.store(false);
}

// Function to continuously read from the serial port
void serialReadThread()
{
    const int BUFFER_SIZE = 1024;
    uint8_t buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    ROS_INFO("Serial read thread started");
    
    while (!should_exit)
    {
        // Skip if serial port is not open
        if (serial_fd < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        // Use select to wait for data with timeout
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serial_fd, &readfds);
        
        // Set timeout (100ms)
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        
        int ret = select(serial_fd + 1, &readfds, NULL, NULL, &tv);
        
        if (ret > 0)
        {
            // Data is available to read
            std::lock_guard<std::mutex> lock(serial_mutex);
            bytes_read = read(serial_fd, buffer, BUFFER_SIZE - 1);
            
            if (bytes_read > 0)
            {
                // Null-terminate the received data for string operations
                buffer[bytes_read] = '\0';
                
                // Process the received data
                processSerialData(buffer, bytes_read);
            }
        }
    }
    
    ROS_INFO("Serial read thread exiting");
}

// Function to parse and process incoming data from FC
void processSerialData(const uint8_t* data, size_t length)
{
    // Convert data to string for easier parsing
    std::string data_str(reinterpret_cast<const char*>(data), length);
    
    // Log minimal info to console (debug level instead of info)
    ROS_DEBUG("Received %zu bytes from FC", length);
    
    // Handle multiple messages in a single read
    // Split by newline characters
    std::istringstream stream(data_str);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Skip empty lines
        if (line.empty()) {
            continue;
        }
        
        // First, always publish raw message to telemetry topic (ensure it's always published)
        std_msgs::String msg;
        msg.data = line;
        fc_telemetry_pub.publish(msg);
        
        // Process based on the line prefix
        if (line.find("CTRL,") == 0) {
            // Parse control telemetry format: "CTRL,value1,value2,...,value18"
            std::vector<double> values;
            std::string csv = line.substr(5); // Skip "CTRL,"
            
            std::istringstream ss(csv);
            std::string token;
            
            while (std::getline(ss, token, ',')) {
                try {
                    double value = std::stod(token);
                    values.push_back(value);
                } catch (const std::exception& e) {
                    ROS_WARN("Error parsing control value: %s", token.c_str());
                }
            }
            
            // Updated to expect 18 values instead of 6
            if (values.size() == 18) {
                // Publish the parsed data array
                std_msgs::Float64MultiArray data_array;
                data_array.data = values;
                fc_data_pub.publish(data_array);
                
                // Log at debug level (shortened for readability)
                ROS_DEBUG("Processed CTRL data with 18 values: %.2f, %.2f, ... (18 total)", 
                         values[0], values[1]);
            } else {
                ROS_WARN("Received incomplete control data, expected 18 values but got %zu", values.size());
            }
        } else {
            // Just log other messages at debug level instead of info
            ROS_DEBUG("FC message: %s", line.c_str());
        }
    }
}

int main(int argc, char** argv)
{
    ros::init(argc, argv, "flight_controller_node");
    ros::NodeHandle nh("~");
    
    // Set log level to INFO by default (DEBUG messages will not show in console)
    if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Info))
    {
        ros::console::notifyLoggerLevelsChanged();
    }

    // Create publishers
    debug_pub = nh.advertise<std_msgs::String>("debug_serial_out", 10);
    latency_pub = nh.advertise<std_msgs::Float64>("fc_latency", 10);
    fc_telemetry_pub = nh.advertise<std_msgs::String>("telemetry", 10);
    fc_data_pub = nh.advertise<std_msgs::Float64MultiArray>("data", 10);

    // Load params
    std::string port;
    nh.param<std::string>("serial_port", port, "/dev/ttyTHS1");
    int baud;
    nh.param<int>("baudrate", baud, 115200);
    
    // Add a configurable parameter for the Vicon topic name
    std::string vicon_topic;
    nh.param<std::string>("vicon_topic", vicon_topic, "/vicon/rocket/pose");

    // Open serial
    if(!openSerial(port, baud))
    {
        ROS_ERROR("Could not open serial port: %s", port.c_str());
        return 1;
    }
    ROS_INFO("Opened serial port: %s at %d baud", port.c_str(), baud);
    ROS_INFO("Subscribing to Vicon topic: %s", vicon_topic.c_str());

    // Start the serial reading thread
    should_exit = false;
    serial_read_thread = std::thread(serialReadThread);

    // Subscribers
    ros::Subscriber pose_sub = nh.subscribe(vicon_topic, 10, poseCallback);
    ros::Subscriber cmd_sub  = nh.subscribe("/fc_command", 10, cmdCallback);

    ros::spin();

    // Cleanup
    should_exit = true;
    if (serial_read_thread.joinable()) {
        serial_read_thread.join();
    }
    
    if (serial_fd >= 0)
    {
        close(serial_fd);
        serial_fd = -1;
    }
    return 0;
}