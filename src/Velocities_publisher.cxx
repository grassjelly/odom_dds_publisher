#include <stdio.h>
#include <stdlib.h>

#include "Velocities.h"
#include "VelocitiesSupport.h"
#include "ndds/ndds_cpp.h"
#include <iostream>
#include "ros/ros.h"
#include "nav_msgs/Odometry.h"

void velocityCallback(const nav_msgs::Odometry::ConstPtr& msg, VelocitiesDataWriter * Velocities_writer,  Velocities *instance, DDS_InstanceHandle_t instance_handle, DDS_ReturnCode_t retcode)
{
  //print received odom values
  ROS_INFO("Linear Velocity X: [%f]", msg->twist.twist.linear.x);
  ROS_INFO("Linear Velocity Y: [%f]", msg->twist.twist.linear.y);
  ROS_INFO("Angular Velocity Z: [%f]", msg->twist.twist.angular.z);

  //pass odom values to dds Velocities struct
  instance->linear_velocity_x = msg->twist.twist.linear.x;
  instance->linear_velocity_y = msg->twist.twist.linear.y;
  instance->angular_velocity_z =  msg->twist.twist.angular.z;

  //publish to DDS middleware
  retcode = Velocities_writer->write(*instance, instance_handle);
  
  //check if pub is successful
  if (retcode != DDS_RETCODE_OK) {
    printf("write error %d\n", retcode);
  }
}

/* Delete all entities */
static int publisher_shutdown(
    DDSDomainParticipant *participant)
{
    DDS_ReturnCode_t retcode;
    int status = 0;

    if (participant != NULL) {
        retcode = participant->delete_contained_entities();
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_contained_entities error %d\n", retcode);
            status = -1;
        }

        retcode = DDSTheParticipantFactory->delete_participant(participant);
        if (retcode != DDS_RETCODE_OK) {
            printf("delete_participant error %d\n", retcode);
            status = -1;
        }
    }

    /* RTI Connext provides finalize_instance() method on
    domain participant factory for people who want to release memory used
    by the participant factory. Uncomment the following block of code for
    clean destruction of the singleton. */
    /*

    retcode = DDSDomainParticipantFactory::finalize_instance();
    if (retcode != DDS_RETCODE_OK) {
        printf("finalize_instance error %d\n", retcode);
        status = -1;
    }
    */
    return status;
}

void publish(VelocitiesDataWriter * Velocities_writer,  Velocities *instance, DDS_InstanceHandle_t instance_handle, DDS_ReturnCode_t retcode)
{
    retcode = Velocities_writer->write(*instance, instance_handle);
    if (retcode != DDS_RETCODE_OK) {
        printf("write error %d\n", retcode);
    }
}

extern "C" int publisher_main(int domainId, int sample_count)
{
    DDSDomainParticipant *participant = NULL;
    DDSPublisher *publisher = NULL;
    DDSTopic *topic = NULL;
    DDSDataWriter *writer = NULL;
    VelocitiesDataWriter * Velocities_writer = NULL;
    Velocities *instance = NULL;
    DDS_ReturnCode_t retcode;
    DDS_InstanceHandle_t instance_handle = DDS_HANDLE_NIL;
    const char *type_name = NULL;
    int count = 0;  
    DDS_Duration_t send_period = {0,20000000};
    /* To customize participant QoS, use 
    the configuration file USER_QOS_PROFILES.xml */
    participant = DDSTheParticipantFactory->create_participant(
        domainId, DDS_PARTICIPANT_QOS_DEFAULT, 
        NULL /* listener */, DDS_STATUS_MASK_NONE);
    if (participant == NULL) {
        printf("create_participant error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize publisher QoS, use 
    the configuration file USER_QOS_PROFILES.xml */
    publisher = participant->create_publisher(
        DDS_PUBLISHER_QOS_DEFAULT, NULL /* listener */, DDS_STATUS_MASK_NONE);
    if (publisher == NULL) {
        printf("create_publisher error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* Register type before creating topic */
    type_name = VelocitiesTypeSupport::get_type_name();
    retcode = VelocitiesTypeSupport::register_type(
        participant, type_name);
    if (retcode != DDS_RETCODE_OK) {
        printf("register_type error %d\n", retcode);
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize topic QoS, use 
    the configuration file USER_QOS_PROFILES.xml */
    topic = participant->create_topic(
        "velocities",
        type_name, DDS_TOPIC_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (topic == NULL) {
        printf("create_topic error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* To customize data writer QoS, use 
    the configuration file USER_QOS_PROFILES.xml */
    writer = publisher->create_datawriter(
        topic, DDS_DATAWRITER_QOS_DEFAULT, NULL /* listener */,
        DDS_STATUS_MASK_NONE);
    if (writer == NULL) {
        printf("create_datawriter error\n");
        publisher_shutdown(participant);
        return -1;
    }
    Velocities_writer = VelocitiesDataWriter::narrow(writer);
    if (Velocities_writer == NULL) {
        printf("DataWriter narrow error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* Create data sample for writing */
    instance = VelocitiesTypeSupport::create_data();
    if (instance == NULL) {
        printf("VelocitiesTypeSupport::create_data error\n");
        publisher_shutdown(participant);
        return -1;
    }

    /* For a data type that has a key, if the same instance is going to be
    written multiple times, initialize the key here
    and register the keyed instance prior to writing */
    /*
    instance_handle = Velocities_writer->register_instance(*instance);
    */

    /* Main loop */
    
    ros::NodeHandle nh;
    ros::Subscriber sub = nh.subscribe<nav_msgs::Odometry>("odom", 1000, boost::bind(velocityCallback, _1,  Velocities_writer,  instance, instance_handle, retcode));
    ros::spin();

    /* Delete data sample */
    retcode = VelocitiesTypeSupport::delete_data(instance);

    if (retcode != DDS_RETCODE_OK) {
        printf("VelocitiesTypeSupport::delete_data error %d\n", retcode);
    }

    /* Delete all entities */
    return publisher_shutdown(participant);
}

int main(int argc, char *argv[])
{
    ros::init(argc, argv, "dds_velocity_bridge");

    int domainId = 0;
    int sample_count = 0; /* infinite loop */

    if (argc >= 2) {
        domainId = atoi(argv[1]);
    }
    if (argc >= 3) {
        sample_count = atoi(argv[2]);
    }

    /* Uncomment this to turn on additional logging
    NDDSConfigLogger::get_instance()->
    set_verbosity_by_category(NDDS_CONFIG_LOG_CATEGORY_API, 
    NDDS_CONFIG_LOG_VERBOSITY_STATUS_ALL);
    */

    return publisher_main(domainId, sample_count);
}