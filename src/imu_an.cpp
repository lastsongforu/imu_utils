#define BACKWARD_HAS_DW 1
#include "backward.hpp"
namespace backward
{
backward::SignalHandling sh;
}

#include "acc_lib/allan_acc.h"
#include "acc_lib/fitallan_acc.h"
#include "gyr_lib/allan_gyr.h"
#include "gyr_lib/fitallan_gyr.h"
#include <code_utils/ros_utils.h>
#include <geometry_msgs/Vector3Stamped.h>
#include <iostream>
#include <mutex>
#include <queue>
#include <ros/ros.h>
#include <sensor_msgs/Imu.h>

std::mutex m_buf;

std::queue< sensor_msgs::ImuConstPtr > imu_buf;
imu::AllanGyr* gyr_x;
imu::AllanGyr* gyr_y;
imu::AllanGyr* gyr_z;
imu::AllanAcc* acc_x;
imu::AllanAcc* acc_y;
imu::AllanAcc* acc_z;
double start_t;
bool start       = true;
bool end         = false;
int max_time_min = 10;
std::string data_save_path;

void
imu_callback( const sensor_msgs::ImuConstPtr& imu_msg )
{
    //    m_buf.lock( );
    //    imu_buf.push( imu_msg );
    //    m_buf.unlock( );
    double time = imu_msg->header.stamp.toSec( );
    gyr_x->pushRadPerSec( imu_msg->angular_velocity.x, time );
    gyr_y->pushRadPerSec( imu_msg->angular_velocity.y, time );
    gyr_z->pushRadPerSec( imu_msg->angular_velocity.z, time );
    acc_x->pushMPerSec2( imu_msg->linear_acceleration.x, time );
    acc_y->pushMPerSec2( imu_msg->linear_acceleration.y, time );
    acc_z->pushMPerSec2( imu_msg->linear_acceleration.z, time );

    if ( start )
    {
        start_t = time;
        start   = false;
    }
    else
    {
        double time_min = ( time - start_t ) / 60;
        if ( time_min > max_time_min )
            end = true;
    }
}

void
writeData1( const std::string sensor_name, //
            const std::vector< double >& gyro_ts_x,
            const std::vector< double >& gyro_d )
{
    std::ofstream out_t;
    std::ofstream out_x;
    out_t.open( data_save_path + "data_" + sensor_name + "_t.txt", std::ios::trunc );
    out_x.open( data_save_path + "data_" + sensor_name + "_x.txt", std::ios::trunc );
    out_t << std::setprecision( 10 );
    out_x << std::setprecision( 10 );
    for ( int index = 0; index < gyro_ts_x.size( ); ++index )
    {
        out_t << gyro_ts_x[index] << '\n';
        out_x << gyro_d[index] << '\n';
    }
    out_t.close( );
    out_x.close( );
}

void
writeData3( const std::string sensor_name,
            const std::vector< double >& gyro_ts_x,
            const std::vector< double >& gyro_d_x,
            const std::vector< double >& gyro_d_y,
            const std::vector< double >& gyro_d_z )
{
    std::ofstream out_t;
    std::ofstream out_x;
    std::ofstream out_y;
    std::ofstream out_z;
    out_t.open( data_save_path + "data_" + sensor_name + "_t.txt", std::ios::trunc );
    out_x.open( data_save_path + "data_" + sensor_name + "_x.txt", std::ios::trunc );
    out_y.open( data_save_path + "data_" + sensor_name + "_y.txt", std::ios::trunc );
    out_z.open( data_save_path + "data_" + sensor_name + "_z.txt", std::ios::trunc );
    out_t << std::setprecision( 10 );
    out_x << std::setprecision( 10 );
    out_y << std::setprecision( 10 );
    out_z << std::setprecision( 10 );
    for ( int index = 0; index < gyro_ts_x.size( ); ++index )
    {
        out_t << gyro_ts_x[index] << '\n';
        out_x << gyro_d_x[index] << '\n';
        out_y << gyro_d_y[index] << '\n';
        out_z << gyro_d_z[index] << '\n';
    }
    out_t.close( );
    out_x.close( );
    out_y.close( );
    out_z.close( );
}

void
writeYAML( )
{
}

int
main( int argc, char** argv )
{
    ros::init( argc, argv, "gyro_test" );
    ros::NodeHandle n( "~" );
    ros::console::set_logger_level( ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug );

    std::string IMU_TOPIC;
    std::string IMU_NAME;
    int max_cluster;

    IMU_TOPIC      = ros_utils::readParam< std::string >( n, "imu_topic" );
    IMU_NAME       = ros_utils::readParam< std::string >( n, "imu_name" );
    data_save_path = ros_utils::readParam< std::string >( n, "data_save_path" );
    max_time_min   = ros_utils::readParam< int >( n, "max_time_min" );
    max_cluster    = ros_utils::readParam< int >( n, "max_cluster" );

    ros::Subscriber sub_imu = n.subscribe( IMU_TOPIC, //
                                           20000000,
                                           imu_callback,
                                           ros::TransportHints( ).tcpNoDelay( ) );
    //    ros::Publisher pub = n.advertise< geometry_msgs::Vector3Stamped >( ALLAN_TOPIC, 2000 );

    gyr_x = new imu::AllanGyr( "gyr x", max_cluster );
    gyr_y = new imu::AllanGyr( "gyr y", max_cluster );
    gyr_z = new imu::AllanGyr( "gyr z", max_cluster );
    acc_x = new imu::AllanAcc( "acc x", max_cluster );
    acc_y = new imu::AllanAcc( "acc y", max_cluster );
    acc_z = new imu::AllanAcc( "acc z", max_cluster );
    std::cout << "wait for imu data." << std::endl;
    ros::Rate loop( 100 );

    //    ros::spin( );
    while ( !end )
    {
        loop.sleep( );
        ros::spinOnce( );
    }

    ///
    {
        gyr_x->calc( );
        std::vector< double > gyro_v_x  = gyr_x->getVariance( );
        std::vector< double > gyro_d_x  = gyr_x->getDeviation( );
        std::vector< double > gyro_ts_x = gyr_x->getTimes( );

        gyr_y->calc( );
        std::vector< double > gyro_v_y  = gyr_y->getVariance( );
        std::vector< double > gyro_d_y  = gyr_y->getDeviation( );
        std::vector< double > gyro_ts_y = gyr_y->getTimes( );

        gyr_z->calc( );
        std::vector< double > gyro_v_z  = gyr_z->getVariance( );
        std::vector< double > gyro_d_z  = gyr_z->getDeviation( );
        std::vector< double > gyro_ts_z = gyr_z->getTimes( );

        std::cout << "Gyro X " << std::endl;
        FitAllanGyr fit_x( gyro_v_x, gyro_ts_x );
        std::cout << "  bias " << gyr_x->getAvgValue( ) / 3600 << " degree/s" << std::endl;
        std::cout << "-------------------" << std::endl;

        std::cout << "Gyro y " << std::endl;
        FitAllanGyr fit_y( gyro_v_y, gyro_ts_y );
        std::cout << "  bias " << gyr_y->getAvgValue( ) / 3600 << " degree/s" << std::endl;
        std::cout << "-------------------" << std::endl;

        std::cout << "Gyro z " << std::endl;
        FitAllanGyr fit_z( gyro_v_z, gyro_ts_z );
        std::cout << "  bias " << gyr_z->getAvgValue( ) / 3600 << " degree/s" << std::endl;
        std::cout << "-------------------" << std::endl;

        std::vector< double > gyro_sim_d_x = fit_x.calcSimDeviation( gyro_ts_x );
        std::vector< double > gyro_sim_d_y = fit_y.calcSimDeviation( gyro_ts_y );
        std::vector< double > gyro_sim_d_z = fit_z.calcSimDeviation( gyro_ts_z );

        writeData3( IMU_NAME + "_sim_gyr", gyro_ts_x, gyro_sim_d_x, gyro_sim_d_y, gyro_sim_d_z );
        writeData3( IMU_NAME + "_gyr", gyro_ts_x, gyro_d_x, gyro_d_y, gyro_d_z );
    }
    std::cout << "==============================================" << std::endl;
    std::cout << "==============================================" << std::endl;

    {
        acc_x->calc( );
        std::vector< double > acc_v_x  = acc_x->getVariance( );
        std::vector< double > acc_d_x  = acc_x->getDeviation( );
        std::vector< double > acc_ts_x = acc_x->getTimes( );

        acc_y->calc( );
        std::vector< double > acc_v_y  = acc_y->getVariance( );
        std::vector< double > acc_d_y  = acc_y->getDeviation( );
        std::vector< double > acc_ts_y = acc_y->getTimes( );

        acc_z->calc( );
        std::vector< double > acc_v_z  = acc_z->getVariance( );
        std::vector< double > acc_d_z  = acc_z->getDeviation( );
        std::vector< double > acc_ts_z = acc_z->getTimes( );

        std::cout << "acc X " << std::endl;
        FitAllanAcc fit_x_acc( acc_v_x, acc_ts_x );
        std::cout << "-------------------" << std::endl;

        std::cout << "acc y " << std::endl;
        FitAllanAcc fit_y_acc( acc_v_y, acc_ts_y );
        std::cout << "-------------------" << std::endl;

        std::cout << "acc z " << std::endl;
        FitAllanAcc fit_z_acc( acc_v_z, acc_ts_z );
        std::cout << "-------------------" << std::endl;
        std::vector< double > acc_sim_d_x = fit_x_acc.calcSimDeviation( acc_ts_x );
        std::vector< double > acc_sim_d_y = fit_y_acc.calcSimDeviation( acc_ts_x );
        std::vector< double > acc_sim_d_z = fit_z_acc.calcSimDeviation( acc_ts_x );
        writeData3( IMU_NAME + "_sim_acc", acc_ts_x, acc_sim_d_x, acc_sim_d_y, acc_sim_d_z );
        writeData3( IMU_NAME + "_acc", acc_ts_x, acc_d_x, acc_d_y, acc_d_z );
    }

    return 0;
}