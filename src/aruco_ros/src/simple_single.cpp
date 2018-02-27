/*****************************
Copyright 2011 Rafael Muñoz Salinas. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY Rafael Muñoz Salinas ''AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Rafael Muñoz Salinas OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Rafael Muñoz Salinas.
********************************/
/**
* @file simple_single.cpp
* @author Bence Magyar
* @date June 2012
* @version 0.1
* @brief ROS version of the example named "simple" in the Aruco software package.
*/

#include <iostream>
#include <aruco/aruco.h>
#include <aruco/cvdrawingutils.h>

#include <opencv2/core/core.hpp>
#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <aruco_ros/aruco_ros_utils.h>
#include <tf/transform_broadcaster.h>
#include <tf/transform_listener.h>
#include <visualization_msgs/Marker.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/Vector3.h>
#include "sensor_msgs/Range.h"
#include "std_msgs/Empty.h"
#include <dynamic_reconfigure/server.h>
#include <aruco_ros/ArucoThresholdConfig.h>
#include <ardrone_autonomy/Navdata.h>
using namespace aruco;

class ArucoSimple
{
private:
  cv::Mat inImage;
  aruco::CameraParameters camParam;
  tf::StampedTransform rightToLeft;
  bool useRectifiedImages;
  MarkerDetector mDetector;
  vector<Marker> markers;
  ros::Subscriber cam_info_sub;
  ros::Subscriber sonar_height_sub;
  ros::Subscriber navdata_sub;
  bool cam_info_received;
  image_transport::Publisher image_pub;
  image_transport::Publisher debug_pub;
  ros::Publisher pose_pub;
  ros::Publisher transform_pub; 
  ros::Publisher position_pub;
  ros::Publisher ardrone_cmd_vel_pub;
  ros::Publisher marker_pub; //rviz visualization marker
  ros::Publisher pixel_pub;
  ros::Publisher land_pub;

  std::string marker_frame;
  std::string camera_frame;
  std::string reference_frame;
  //std::string state;

  double marker_size;
  int marker_id;
  //My additions
  int count;
  float z_rot_count;
  float z_rot_coeff;
  float sonar_height;
  float sonar_height_error;
  float xy_error;
  float err_eps;
  // state and land
  bool land_begin;
  std::string state;
  

  geometry_msgs::Twist cmdVelMsg;
  geometry_msgs::Vector3 navdataMsg;
  std_msgs::Empty emptyMsg;
  ros::NodeHandle nh;
  image_transport::ImageTransport it;
  image_transport::Subscriber image_sub;

  tf::TransformListener _tfListener;

public:

  ArucoSimple()
    : cam_info_received(false),
      nh("~"),
      it(nh)
  {
//initialize
    //states
    land_begin=false;
    state="search";
    err_eps=0.005;
    count=0;
    sonar_height_error=100;
    xy_error=100;
    z_rot_count=0.4;
    std::string refinementMethod;
    nh.param<std::string>("corner_refinement", refinementMethod, "LINES");
    if ( refinementMethod == "SUBPIX" )
      mDetector.setCornerRefinementMethod(aruco::MarkerDetector::SUBPIX);
    else if ( refinementMethod == "HARRIS" )
      mDetector.setCornerRefinementMethod(aruco::MarkerDetector::HARRIS);
    else if ( refinementMethod == "NONE" )
      mDetector.setCornerRefinementMethod(aruco::MarkerDetector::NONE); 
    else      
      mDetector.setCornerRefinementMethod(aruco::MarkerDetector::LINES); 

    //Print parameters of aruco marker detector:
    ROS_INFO_STREAM("Corner refinement method: " << mDetector.getCornerRefinementMethod());
    ROS_INFO_STREAM("Threshold method: " << mDetector.getThresholdMethod());
    double th1, th2;
    mDetector.getThresholdParams(th1, th2);
    ROS_INFO_STREAM("Threshold method: " << " th1: " << th1 << " th2: " << th2);
    float mins, maxs;
    mDetector.getMinMaxSize(mins, maxs);
    ROS_INFO_STREAM("Marker size min: " << mins << "  max: " << maxs);
    ROS_INFO_STREAM("Desired speed: " << mDetector.getDesiredSpeed());

    image_sub = it.subscribe("/image", 1, &ArucoSimple::image_callback, this);
    cam_info_sub = nh.subscribe("/camera_info", 1, &ArucoSimple::cam_info_callback, this);
    sonar_height_sub = nh.subscribe("/sonar_height", 10, &ArucoSimple::sonar_height_callback, this);
    navdata_sub = nh.subscribe("/ardrone/navdata", 10, &ArucoSimple::navdata_callback, this);

    image_pub = it.advertise("result", 1);
    debug_pub = it.advertise("debug", 1);
    pose_pub = nh.advertise<geometry_msgs::PoseStamped>("pose", 100);
    transform_pub = nh.advertise<geometry_msgs::TransformStamped>("transform", 100);
    position_pub = nh.advertise<geometry_msgs::Vector3Stamped>("position", 100);
    marker_pub = nh.advertise<visualization_msgs::Marker>("marker", 10);
    pixel_pub = nh.advertise<geometry_msgs::PointStamped>("pixel", 10);
    ardrone_cmd_vel_pub = nh.advertise<geometry_msgs::Twist>("/cmd_vel", 1);
    land_pub = nh.advertise<std_msgs::Empty>("/ardrone/land", 100);

    nh.param<double>("marker_size", marker_size, 0.05);
    nh.param<int>("marker_id", marker_id, 300);
    nh.param<std::string>("reference_frame", reference_frame, "");
    nh.param<std::string>("camera_frame", camera_frame, "");
    nh.param<std::string>("marker_frame", marker_frame, "");
    nh.param<bool>("image_is_rectified", useRectifiedImages, true);

    ROS_ASSERT(camera_frame != "" && marker_frame != "");

    if ( reference_frame.empty() )
      reference_frame = camera_frame;

    ROS_INFO("Aruco node started with marker size of %f m and marker id to track: %d",
             marker_size, marker_id);
    ROS_INFO("Aruco node will publish pose to TF with %s as parent and %s as child.",
             reference_frame.c_str(), marker_frame.c_str());
  }

  bool getTransform(const std::string& refFrame, const std::string& childFrame, tf::StampedTransform& transform)
  {
    std::string errMsg;

    if ( !_tfListener.waitForTransform(refFrame,childFrame, ros::Time(0), ros::Duration(0.5), ros::Duration(0.01), &errMsg) )
    {
      ROS_ERROR_STREAM("Unable to get pose from TF: " << errMsg);
      return false;
    }
    else
    {
      try
      {
        _tfListener.lookupTransform( refFrame, childFrame,
                                     ros::Time(0),                  //get latest available
                                     transform);
      }
      catch ( const tf::TransformException& e)
      {
        ROS_ERROR_STREAM("Error in lookupTransform of " << childFrame << " in " << refFrame);
        return false;
      }
    }
    return true;
  }

  void set_cmd_vel(float x_l,float y_l,float z_l,float x_r,float y_r,float z_r){
      cmdVelMsg.linear.x=x_l;
      cmdVelMsg.linear.y=y_l;
      cmdVelMsg.linear.z=z_l;
      cmdVelMsg.angular.x=x_r;
      cmdVelMsg.angular.y=y_r;
      cmdVelMsg.angular.z=z_r;
      ardrone_cmd_vel_pub.publish(cmdVelMsg);
  }
  void marker_follower(geometry_msgs::Vector3Stamped positionMsg){
            //PD controller
            //works for simulation
            float Kp=0.2;
            float Kd=0.0005;
            xy_error= positionMsg.vector.x*positionMsg.vector.x + positionMsg.vector.y*positionMsg.vector.y;
//            set_cmd_vel(-Kp*(-positionMsg.vector.x) + Kd*(-navdataMsg.x), -Kp*(-positionMsg.vector.y) + Kd*(-navdataMsg.y), 0, 0, 0, 0);
            
            //works for real 
            // realde dikey x ekseni ve yukarisi negatif oluyor nedense. Kordinat farkli yani
//            ROS_INFO("Apply x: %f", Kp*(-positionMsg.vector.x) );
//            ROS_INFO("Apply y: %f", Kp*(-positionMsg.vector.y) );
            
            /* // below a certain vveloicty real quadrotor begin to make wierd movements
            float x_vel = Kp*(-positionMsg.vector.x);
            float y_vel = Kp*(-positionMsg.vector.y);

            if(x_vel<0.005 && x_vel>-0.005 )
              x_vel=0;
            if(y_vel<0.005 && y_vel>-0.005 )
              y_vel=0;              
            set_cmd_vel(x_vel, y_vel, 0, 0, 0, 0);
            */
            set_cmd_vel(Kp*(-positionMsg.vector.x), Kp*(-positionMsg.vector.y), 0, 0, 0, 0);
            //ROS_INFO("%f",positionMsg.vector.x);
  }

  
  //Reads the platform GPS value
  geometry_msgs::PoseStamped marker_position_callback(){
    geometry_msgs::PoseStamped marker_GPS_position;
    marker_GPS_position.pose.position.x=2;
    marker_GPS_position.pose.position.y=2;
    marker_GPS_position.pose.position.z=0;
    return marker_GPS_position;
  }


  void marker_search(){
      set_cmd_vel(0.2, 0.18, 0, 0, 0, 0);
  }

  void sonar_height_callback(const sensor_msgs::Range::ConstPtr& msg){
    sonar_height=msg->range;
  }

  void navdata_callback(ardrone_autonomy::Navdata msg){
    navdataMsg.x=msg.vx;
    navdataMsg.y=msg.vy;
    navdataMsg.z=msg.vz;
  }

  void heigh_contoller(float desired_heigt){
    //PD controller
      float Kp=10.0;
      float Kd=0.001;
      sonar_height_error=desired_heigt- sonar_height;
      //ROS_INFO("%f",sonar_height_error);
      set_cmd_vel(0, 0, Kp*(sonar_height_error) + Kd*(-navdataMsg.z), 0, 0, 0);
  }

  void image_callback(const sensor_msgs::ImageConstPtr& msg)
  {
    static tf::TransformBroadcaster br;
    if(cam_info_received)
    {
      ros::Time curr_stamp(ros::Time::now());
      cv_bridge::CvImagePtr cv_ptr;
      //ROS_INFO("x %f, y %f, z %f", navdataMsg.x, navdataMsg.y, navdataMsg.z);
      try
      {
        cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::RGB8);
        inImage = cv_ptr->image;

        //detection results will go into "markers"
        markers.clear();
        //Ok, let's detect
        mDetector.detect(inImage, markers, camParam, marker_size, false);
        //for each marker, draw info and its boundaries in the image

/*        //if the marker is lost
        if(markers.size()==0 && !(land_begin))
        {
          state="search";
        }
        
        //implement the states
        if(state == "search")
        {
          //ROS_INFO("SEARCH");
          //adjust the height
          if(sonar_height_error>err_eps) //mutlak deger yapmak lazım
          {
            heigh_contoller(2.0);
          }
          //if not found search, if found change the state
          else if(sonar_height_error<err_eps)
          {
            if(markers.size()==1)
            {
              state="follow";
            }
            else
            {            
              geometry_msgs::PoseStamped marker_GPS_position;
              marker_GPS_position=marker_position_callback();
              marker_search();
            }
          }
          else
          { //set vel diye bir function yazabilirim!
              set_cmd_vel(0, 0, 0, 0, 0, 0);
              //ROS_INFO("Reached!!");
          }


        }
        else if(state=="follow")
        {
          //ROS_INFO("FOLLOW");
*/        

/*
          if(markers.size()==0){
            set_cmd_vel(0,0,0,0,0,0);
          }
*/          
          for(size_t i=0; i<markers.size(); ++i)
          {
            // only publishing the selected marker
            //bunun elseinde ise search algoritmasi calismasi lazim!!!
            if(markers[i].id == marker_id)
            {
              tf::Transform transform = aruco_ros::arucoMarker2Tf(markers[i]);
              //ROS_INFO("%f %f %f mete", markers[i].Rvec.at<float>(0,0), markers[i].Rvec.at<float>(1,0),markers[i].Rvec.at<float>(2,0));
              tf::StampedTransform cameraToReference;
              cameraToReference.setIdentity();
              
              if ( reference_frame != camera_frame )
              {
                getTransform(reference_frame,
                            camera_frame,
                            cameraToReference);
              }
              
              
              transform = static_cast<tf::Transform>(cameraToReference) * static_cast<tf::Transform>(rightToLeft) * transform;
              tf::StampedTransform stampedTransform(transform, curr_stamp,reference_frame, marker_frame);
              br.sendTransform(stampedTransform);
              geometry_msgs::PoseStamped poseMsg;
              tf::poseTFToMsg(transform, poseMsg.pose);
              
              //ROS_INFO("%f",poseMsg.pose.position.x);
              //ROS_INFO("%f",poseMsg.pose.position.y);
              //ROS_INFO("%f",poseMsg.pose.position.z);

              poseMsg.header.frame_id = reference_frame;
              poseMsg.header.stamp = curr_stamp;
              pose_pub.publish(poseMsg);

              geometry_msgs::TransformStamped transformMsg;
              tf::transformStampedTFToMsg(stampedTransform, transformMsg);
              transform_pub.publish(transformMsg);

              geometry_msgs::Vector3Stamped positionMsg;
              positionMsg.header = transformMsg.header;
              positionMsg.vector = transformMsg.transform.translation;
              position_pub.publish(positionMsg);
/*
              //tracking control
              if(xy_error < xy_error_treshold){
                state="land";
                land_begin=true;
                break;
              }
*/              marker_follower(positionMsg);

  
              geometry_msgs::PointStamped pixelMsg;
              pixelMsg.header = transformMsg.header;
              pixelMsg.point.x = markers[i].getCenter().x;
              pixelMsg.point.y = markers[i].getCenter().y;
              pixelMsg.point.z = 0;
              pixel_pub.publish(pixelMsg);

              //Publish rviz marker representing the ArUco marker patch
              visualization_msgs::Marker visMarker;
              visMarker.header = transformMsg.header;
              visMarker.pose = poseMsg.pose;
              visMarker.id = 1;
              visMarker.type   = visualization_msgs::Marker::CUBE;
              visMarker.action = visualization_msgs::Marker::ADD;
              visMarker.pose = poseMsg.pose;
              visMarker.scale.x = marker_size;
              visMarker.scale.y = 0.001;
              visMarker.scale.z = marker_size;
              visMarker.color.r = 1.0;
              visMarker.color.g = 0;
              visMarker.color.b = 0;
              visMarker.color.a = 1.0;
              visMarker.lifetime = ros::Duration(3.0);
              marker_pub.publish(visMarker);
            }
            // but drawing all the detected markers
            markers[i].draw(inImage,cv::Scalar(0,0,255),2);
          }
/*        }
        else if(state=="land")
        {
          //ROS_INFO("LAND");          
          heigh_contoller(0.8);
          if(sonar_height_error<err_eps)
          {
              //set_cmd_vel(0,0,0,0,0,0);
              land_pub.publish(emptyMsg);
          }
            //komple land yap
          //if(xy_error>xy_error_treshold) state="follow" xy_error bulucu yazmam lazım
        }
        else{
          set_cmd_vel(0,0,0,0,0,0);
        }     
        
*/
        //draw a 3d cube in each marker if there is 3d info
        if(camParam.isValid() && marker_size!=-1)
        {
          for(size_t i=0; i<markers.size(); ++i)
          {
            CvDrawingUtils::draw3dAxis(inImage, markers[i], camParam);
          }
        }

        if(image_pub.getNumSubscribers() > 0)
        {
          //show input with augmented information
          cv_bridge::CvImage out_msg;
          out_msg.header.stamp = curr_stamp;
          out_msg.encoding = sensor_msgs::image_encodings::RGB8;
          out_msg.image = inImage;
          image_pub.publish(out_msg.toImageMsg());
        }

        if(debug_pub.getNumSubscribers() > 0)
        {
          //show also the internal image resulting from the threshold operation
          cv_bridge::CvImage debug_msg;
          debug_msg.header.stamp = curr_stamp;
          debug_msg.encoding = sensor_msgs::image_encodings::MONO8;
          debug_msg.image = mDetector.getThresholdedImage();
          debug_pub.publish(debug_msg.toImageMsg());
        }
      }
      catch (cv_bridge::Exception& e)
      {
        ROS_ERROR("cv_bridge exception: %s", e.what());
        return;
      }
    }
  }
  // wait for one camerainfo, then shut down that subscriber
  void cam_info_callback(const sensor_msgs::CameraInfo &msg)
  {
    camParam = aruco_ros::rosCameraInfo2ArucoCamParams(msg, useRectifiedImages);
    // handle cartesian offset between stereo pairs
    // see the sensor_msgs/CamaraInfo documentation for details
    rightToLeft.setIdentity();
    rightToLeft.setOrigin(
        tf::Vector3(
            -msg.P[3]/msg.P[0],
            -msg.P[7]/msg.P[5],
            0.0));

    cam_info_received = true;
    cam_info_sub.shutdown();
  }


  void reconf_callback(aruco_ros::ArucoThresholdConfig &config, uint32_t level)
  {
    mDetector.setThresholdParams(config.param1,config.param2);
    if (config.normalizeImage)
    {
      ROS_WARN("normalizeImageIllumination is unimplemented!");
    }
  }
};

int main(int argc,char **argv)
{
  ros::init(argc, argv, "aruco_simple");
  ArucoSimple node;
  ros::spin();
}