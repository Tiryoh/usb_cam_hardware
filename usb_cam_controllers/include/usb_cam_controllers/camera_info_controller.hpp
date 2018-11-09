#ifndef USB_CAM_CONTROLLERS_CAMERA_INFO_CONTROLLER
#define USB_CAM_CONTROLLERS_CAMERA_INFO_CONTROLLER

#include <string>
#include <vector>

#include <controller_interface/controller.h>
#include <realtime_tools/realtime_publisher.h>
#include <ros/duration.h>
#include <ros/node_handle.h>
#include <ros/time.h>
#include <sensor_msgs/CameraInfo.h>
#include <usb_cam_hardware_interface/packet_interface.hpp>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace usb_cam_controllers {

class CameraInfoController
    : public controller_interface::Controller< usb_cam_hardware_interface::PacketInterface > {
public:
  CameraInfoController() {}

  virtual bool init(usb_cam_hardware_interface::PacketInterface *hw, ros::NodeHandle &root_nh,
                    ros::NodeHandle &controller_nh) {
    if (!hw) {
      ROS_ERROR("Null packet interface");
      return false;
    }

    const std::vector< std::string > names(hw->getNames());
    if (names.empty()) {
      ROS_ERROR("No packet handle");
      return false;
    }
    if (names.size() > 1) {
      ROS_WARN_STREAM(names.size() << " packet handles. camera info synchronized to stamps from "
                                      "the first handle will be published.");
    }

    packet_ = hw->getHandle(names.front());
    publisher_ = boost::make_shared< Publisher >(root_nh, "camera_info", 1);
    last_stamp_ = ros::Time(0);

    return true;
  }

  virtual void starting(const ros::Time &time) {
    // nothing to do
  }

  virtual void update(const ros::Time &time, const ros::Duration &period) {
    // validate the packet
    if (!packet_.getStart()) {
      ROS_INFO("No packet. Will skip publishing camera info.");
      return;
    }
    if (packet_.getStamp() == last_stamp_) {
      ROS_INFO("Packet is not updated. Will skip publishing camera info.");
      return;
    }

    // try own the publisher
    if (!publisher_->trylock()) {
      ROS_WARN_STREAM("Cannot own the camera info publisher");
      return;
    }

    // publish the camera info
    publisher_->msg_ = sensor_msgs::CameraInfo(); // TODO: get info from CameraInfoManager
    publisher_->msg_.header.stamp = packet_.getStamp();
    publisher_->msg_.header.frame_id = ""; // TODO: set reasonable frame id
    publisher_->unlockAndPublish();
    last_stamp_ = packet_.getStamp();
  }

  virtual void stopping(const ros::Time &time) {
    // nothing to do
  }

private:
  typedef realtime_tools::RealtimePublisher< sensor_msgs::CameraInfo > Publisher;

  usb_cam_hardware_interface::PacketHandle packet_;
  boost::shared_ptr< Publisher > publisher_;
  ros::Time last_stamp_;
};

} // namespace usb_cam_controllers

#endif