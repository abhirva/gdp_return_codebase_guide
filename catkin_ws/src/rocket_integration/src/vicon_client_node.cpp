/***************************************************************************
 * vicon_client_node.cpp
 * 
 * A ROS node that connects to a Vicon DataStream server, retrieves
 * 6-DOF pose data for a given subject and publishes it as a PoseStamped.
 *
 * Requirements:
 *  - DataStreamClient.h and libViconDataStreamSDK_CPP.so from the official Vicon SDK
 *  - ROS (Melodic, Noetic, etc.) with geometry_msgs, tf2
 ***************************************************************************/

#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <tf2/LinearMath/Quaternion.h>

// Vicon DataStream SDK
#include "DataStreamClient.h"

// Use the ViconDataStreamSDK::CPP namespace to simplify calls
using namespace ViconDataStreamSDK::CPP;

int main(int argc, char** argv)
{
  // Initialize the ROS node
  ros::init(argc, argv, "vicon_client_node");
  ros::NodeHandle nh("~");  // private node handle

  // Retrieve parameters or set defaults
  // e.g. rosrun rocket_integration vicon_client_node _vicon_host:=192.168.10.1:801
  std::string vicon_host;
  nh.param<std::string>("vicon_host", vicon_host, "localhost:801");
  
  // For your rocket, you might have a subject name in Vicon called "Rocket"
  // or "Object1". We'll param as well:
  std::string vicon_subject;
  nh.param<std::string>("vicon_subject", vicon_subject, "Rocket");

  // If there's only one segment or you specifically want "Root" or "base_link"
  // in Vicon, param it or leave as default
  std::string vicon_segment;
  nh.param<std::string>("vicon_segment", vicon_segment, "Root");

  // Advertise a ROS topic for the pose data
  ros::Publisher pose_pub =
      nh.advertise<geometry_msgs::PoseStamped>("vicon_pose", 10);

  // Create Vicon DataStream Client
  Client viconClient;

  // Connect to the Vicon server
  ROS_INFO_STREAM("Connecting to Vicon at " << vicon_host << " ...");
  Output_Connect connectResult = viconClient.Connect(vicon_host);
  if (connectResult.Result != Result::Success)
  {
    ROS_ERROR_STREAM("Failed to connect to Vicon server: " << vicon_host 
                      << ", result=" << connectResult.Result);
    return 1;
  }
  ROS_INFO_STREAM("Connected to Vicon server at " << vicon_host);

  // Enable segment data so we can get position + orientation
  viconClient.EnableSegmentData();
  // Optionally: viconClient.EnableMarkerData();
  // Optionally: viconClient.EnableUnlabeledMarkerData();
  // etc.

  // Set the streaming mode. You can choose ServerPush or ClientPullPreFetch
  viconClient.SetStreamMode(StreamMode::ClientPull);

  // Optionally, set axis mapping if needed (Y up, Z up, etc.). By default:
  // viconClient.SetAxisMapping(Direction::Forward, Direction::Left, Direction::Up);

  // We'll run at 100 Hz or so:
  ros::Rate loop_rate(100);

  while (ros::ok())
  {
    // Request the latest frame from Vicon
    Output_GetFrame oframe = viconClient.GetFrame();
    if (oframe.Result == Result::Success)
    {
      // We have a new frame, let's fetch subject's global translation
      auto transRes = viconClient.GetSegmentGlobalTranslation(vicon_subject, vicon_segment);
      if (transRes.Result == Result::Success && !transRes.Occluded)
      {
        double x_mm = transRes.Translation[0]; // in mm
        double y_mm = transRes.Translation[1];
        double z_mm = transRes.Translation[2];
        
        // Convert mm -> meter if you prefer
        double x = x_mm / 1000.0;
        double y = y_mm / 1000.0;
        double z = z_mm / 1000.0;

        // Next, fetch Euler angles (X, Y, Z rotation in radians)
        auto rotRes = viconClient.GetSegmentGlobalRotationEulerXYZ(vicon_subject, vicon_segment);
        double rx = 0.0, ry = 0.0, rz = 0.0;  // in radians
        bool haveRotation = false;

        if (rotRes.Result == Result::Success && !rotRes.Occluded)
        {
          rx = rotRes.Rotation[0];  // rotation about X
          ry = rotRes.Rotation[1];  // rotation about Y
          rz = rotRes.Rotation[2];  // rotation about Z
          haveRotation = true;
        }

        // Convert Euler angles to a quaternion
        tf2::Quaternion q;
        // setRPY takes arguments as (roll, pitch, yaw)
        // If Vicon's "EulerXYZ" means rotation about X, then Y, then Z in that order,
        // you might interpret them as roll=rx, pitch=ry, yaw=rz.
        q.setRPY(rx, ry, rz);

        // Prepare a PoseStamped
        geometry_msgs::PoseStamped pose_msg;
        pose_msg.header.stamp = ros::Time::now();
        pose_msg.header.frame_id = "vicon_world"; // or "map", "odom", etc.

        pose_msg.pose.position.x = x;
        pose_msg.pose.position.y = y;
        pose_msg.pose.position.z = z;

        if (haveRotation)
        {
          pose_msg.pose.orientation.x = q.x();
          pose_msg.pose.orientation.y = q.y();
          pose_msg.pose.orientation.z = q.z();
          pose_msg.pose.orientation.w = q.w();
        }
        else
        {
          // If rotation is occluded, we could default to no rotation
          pose_msg.pose.orientation.x = 0;
          pose_msg.pose.orientation.y = 0;
          pose_msg.pose.orientation.z = 0;
          pose_msg.pose.orientation.w = 1;
        }

        // Publish on the topic
        pose_pub.publish(pose_msg);
      }
      else
      {
        // No translation data or the segment is occluded
        ROS_WARN_ONCE("Segment translation occluded or not found for subject '%s' segment '%s'.",
                      vicon_subject.c_str(), vicon_segment.c_str());
      }

    }
    else
    {
      // Could not get a frame
      ROS_WARN_STREAM_THROTTLE(1.0, "Failed to get Vicon frame: result=" << oframe.Result);
    }

    ros::spinOnce();
    loop_rate.sleep();
  }

  // Disconnect cleanly
  viconClient.Disconnect();
  ROS_INFO("Vicon node shutting down.");

  return 0;
}