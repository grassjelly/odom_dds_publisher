### Running the demo
1. Run ROSCORE

    ```sh
    $ roscore
    ```
2. Run fake odometry data (optional).

    ```sh
    $ rosrun odom_dds_publisher odomtest_pub
    ```
3. Run ROS -> DDS republisher.

    ```sh
    $ rosrun odom_dds_publisher velocities_publisher
    ```
3. Run DDS Subscriber.

    ```sh
    $ rosrun odom_dds_publisher velocities_subscriber
    ```

### Notes:
DDS Domain ID: 0
DDS Topic Name: "velocities"
Published Data Structure:
struct Velocities
{
    string<10> robot_id;
    float linear_velocity_x;
    float linear_velocity_y;
    float angular_velocity_z;
};
