#!/usr/bin/env python
import rospy
from geometry_msgs.msg import PoseStamped
import random

def vicon_mock():
    rospy.init_node('vicon_mock', anonymous=True)
    pub = rospy.Publisher('/vicon/rocket/pose', PoseStamped, queue_size=10)
    rate = rospy.Rate(10)  # 10 Hz
    while not rospy.is_shutdown():
        msg = PoseStamped()
        # Simulate random pose data
        msg.pose.position.x = random.uniform(-5.0, 5.0)
        msg.pose.position.y = random.uniform(-5.0, 5.0)
        msg.pose.position.z = random.uniform(0.0, 3.0)
        pub.publish(msg)
        rate.sleep()

if __name__ == '__main__':
    vicon_mock()
