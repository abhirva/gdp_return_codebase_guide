// File: vicon_udp_node.cpp

#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <tf/tf.h> // For quaternion from euler if needed
#include <std_msgs/Float64.h>

// A small struct to hold parsed data for each object
struct ViconObject
{
  int frameNumber;
  std::string name;
  double transX;
  double transY;
  double transZ;
  double rotX;
  double rotY;
  double rotZ;
  // Add a timestamp for when the UDP packet was received
  ros::Time receiveTime;
};

// Parse the Vicon UDP packet, returning a list of objects (one entry per item)
std::vector<ViconObject> parseViconPacket(const uint8_t* data, size_t length)
{
  std::vector<ViconObject> results;

  if (length < 5)
  {
    // Not enough bytes for even frameNumber + itemsInBlock
    return results;
  }

  // 0-3: Frame number (int32, little-endian)
  int frameNumber = 0;
  // memcpy approach (since struct might cause alignment issues)
  std::memcpy(&frameNumber, data + 0, 4);

  // 4: ItemsInBlock (uint8)
  uint8_t itemsInBlock = data[4];

  // offset after reading 5 bytes
  size_t offset = 5;

  for (int i = 0; i < itemsInBlock; i++)
  {
    // First, check we have at least 3 more bytes for itemID + itemDataSize
    if (offset + 3 > length) break;

    uint8_t itemID = data[offset];
    offset += 1;

    uint16_t itemDataSize = 0;
    std::memcpy(&itemDataSize, data + offset, 2);
    offset += 2;

    // Ensure we have itemDataSize bytes remaining
    if (offset + itemDataSize > length)
    {
      // Packet might be truncated
      break;
    }

    // According to standard doc: 
    //   - First 24 bytes: object name (zero-padded ASCII)
    //   - Next 6 doubles: translation (X,Y,Z) & rotation (X,Y,Z)
    // Typically that's 72 bytes total after the name if itemDataSize=72 or 80 depending on alignment.

    // Object name (24 bytes)
    char nameBuf[25];
    std::memset(nameBuf, 0, 25);
    std::memcpy(nameBuf, data + offset, 24);
    // remove trailing zeroes for a clean std::string
    std::string objName(nameBuf);
    // we can trim at first null char if needed but this is typically okay

    // Next are doubles for transX, transY, transZ, rotX, rotY, rotZ
    double tX, tY, tZ, rX, rY, rZ;
    // positions start at offset+24
    std::memcpy(&tX, data + offset + 24, 8);
    std::memcpy(&tY, data + offset + 32, 8);
    std::memcpy(&tZ, data + offset + 40, 8);
    // rotations start at offset+48
    std::memcpy(&rX, data + offset + 48, 8);
    std::memcpy(&rY, data + offset + 56, 8);
    std::memcpy(&rZ, data + offset + 64, 8);

    ViconObject vo;
    vo.frameNumber = frameNumber;
    vo.name        = objName;
    vo.transX      = tX;
    vo.transY      = tY;
    vo.transZ      = tZ;
    vo.rotX        = rX;
    vo.rotY        = rY;
    vo.rotZ        = rZ;

    results.push_back(vo);

    offset += itemDataSize; // Move to next item
  }

  return results;
}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "vicon_udp_node");
  ros::NodeHandle nh("~");

  // Param for port
  int udpPort;
  nh.param<int>("udp_port", udpPort,57273); 
  
  // Add a diagnostic publisher for latency
  ros::Publisher latency_pub = nh.advertise<std_msgs::Float64>("vicon_latency", 10);

  // Create socket
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    ROS_ERROR("Failed to create UDP socket: %s", std::strerror(errno));
    return 1;
  }

  // Bind
  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
  addr.sin_port = htons(udpPort);

  if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
  {
    ROS_ERROR("Bind failed on port %d: %s", udpPort, std::strerror(errno));
    close(sockfd);
    return 1;
  }

  ROS_INFO("Listening for Vicon UDP on port %d", udpPort);

  // We will do a non-blocking approach with select or simply use a short recv timeout
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 5000; // 5ms
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  ros::Rate loop_rate(200); // up to 200 Hz
  uint8_t buffer[2048];

  // We'll create publishers on the fly if we see new object names
  // Or we can create a single pub. For minimal latency, consider
  // re-using a map of pub handles.
  std::map<std::string, ros::Publisher> pubMap;

  while (ros::ok())
  {
    sockaddr_in senderAddr;
    socklen_t senderLen = sizeof(senderAddr);
    std::memset(buffer, 0, sizeof(buffer));

    int received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                            reinterpret_cast<sockaddr*>(&senderAddr),
                            &senderLen);
    if (received > 0)
    {
      // Record the time when we received the packet
      ros::Time receiveTime = ros::Time::now();
      
      // parse
      std::vector<ViconObject> objs = parseViconPacket(buffer, received);

      for (auto& obj : objs)
      {
        // Store the receive time
        obj.receiveTime = receiveTime;
        
        // Convert mm->m if needed
        double xm = obj.transX / 1000.0;
        double ym = obj.transY / 1000.0;
        double zm = obj.transZ / 1000.0;

        // Convert Euler angles to quaternion
        // doc says rotationX, rotationY, rotationZ are EulerXYZ in radians
        // We'll use tf::createQuaternionFromRPY(roll, pitch, yaw) with the r->x, p->y, y->z approach
        // but the exact order can matter. Let's do a small check:
        tf::Quaternion quat = tf::createQuaternionFromRPY(obj.rotX, obj.rotY, obj.rotZ);

        // If we haven't created a publisher for this objectName yet, do so
        if (pubMap.find(obj.name) == pubMap.end())
        {
          std::string topic = "/vicon/" + obj.name + "/pose";
          pubMap[obj.name] = nh.advertise<geometry_msgs::PoseStamped>(topic, 1);
          ROS_INFO("Creating publisher for object: %s -> %s",
                   obj.name.c_str(), topic.c_str());
        }
        ros::Publisher& thisPub = pubMap[obj.name];

        geometry_msgs::PoseStamped ps;
        // Use receive time as the timestamp
        ps.header.stamp = receiveTime;
        ps.header.frame_id = "vicon_world";
        ps.pose.position.x = xm;
        ps.pose.position.y = ym;
        ps.pose.position.z = zm;
        ps.pose.orientation.x = quat.x();
        ps.pose.orientation.y = quat.y();
        ps.pose.orientation.z = quat.z();
        ps.pose.orientation.w = quat.w();

        thisPub.publish(ps);
        
        // Calculate and publish the ROS-side latency
        std_msgs::Float64 latency_msg;
        latency_msg.data = (ros::Time::now() - receiveTime).toSec() * 1000.0; // in milliseconds
        latency_pub.publish(latency_msg);
      }
    }

    ros::spinOnce();
    loop_rate.sleep();
  }

  close(sockfd);
  return 0;
}
