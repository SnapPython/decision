//作出决策的节点
#ifndef RM_DECISION__COMMANDER_HPP_
#define RM_DECISION__COMMANDER_HPP_

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/publisher.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"

#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_ros/transform_broadcaster.h"
#include "std_msgs/msg/float64.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"
#include "geometry_msgs/msg/pose.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "action_msgs/msg/goal_status_array.hpp"

#include <geometry_msgs/msg/detail/pose_stamped__struct.hpp>
#include <memory>
#include <iostream>
#include <nav_msgs/msg/detail/odometry__struct.hpp>
#include <rclcpp/node.hpp>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <vector>
#include <thread>
#include <optional>
#include <fstream>
#include <string>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <chrono>

#include "rm_decision_interfaces/msg/all_robot_hp.hpp"
#include "auto_aim_interfaces/msg/target.hpp"
#include "rm_decision_interfaces/msg/to_serial.hpp"
#include "rm_decision_interfaces/msg/from_serial.hpp"


#include <sensor_msgs/msg/laser_scan.hpp>
//behave tree
#include "behaviortree_cpp/bt_factory.h"
#include "behaviortree_cpp/action_node.h"
#include "behaviortree_ros2/bt_action_node.hpp"
#include "behaviortree_ros2/plugins.hpp"
#include "behaviortree_ros2/bt_topic_sub_node.hpp"
#include "behaviortree_cpp/loggers/groot2_publisher.h"


namespace rm_decision {

// struct ReceivePacket
//     {
//     uint8_t header = 0x5A;

//     //以下是自瞄的数据
//     uint8_t detect_color;  // 0-red 1-blue 发1
//     float yaw;               // rad
//     float pitch;

//     //以下是导航的数据
//     uint8_t game_progress;  // 4：比赛进行中

//     //以下是增益点信息
//     uint32_t rfid_status; //bit 19：已到达增益点
//     uint32_t event_data; //bit 30-31：中心增益点的占领情况，0 为未被占领，1 为被己方占领，2 为被对方占领，3 为被双方占领

//     //接收其它机器人弹量（半自动哨兵？）
//     uint8_t supply_robot_id;
//     uint8_t supply_projectile_num;

//     uint16_t checksum = 0;     // crc16校验位 https://blog.csdn.net/ydyuse/article/details/105395368
//     }__attribute__((packed));

    class Commander;


    class State {
    private:
    public:
        State(Commander *commander) : commander(commander) {}

        virtual ~State() = default;

        virtual void handle() {};
        Commander *commander;
        // virtual std::optional<std::shared_ptr<State>> check() = 0;
    };

    class Robot {
    public:
        Robot() = default;

        Robot(int id, geometry_msgs::msg::PoseStamped pose, uint hp) : id(id), pose(pose), hp(hp) {}

        int id;
        geometry_msgs::msg::PoseStamped pose;
        uint hp;
        bool attack = false;

        void check() {
            if (pose.pose.position.x >= 4.5) {
                attack = true;
            }
        }
    };

    class PatrolState : public State {
    public:
        explicit PatrolState(Commander *commander) : State(commander) {}

        void handle() override;
    };

    class GoAndStayState : public State {
    public:
        explicit GoAndStayState(Commander *commander) : State(commander) {}

        void handle() override;
    };

    class AttackState : public State {
    public:
        explicit AttackState(Commander *commander) : State(commander) {}

        virtual void handle() override;
    };

    class WaitState : public State {
    public:
        explicit WaitState(Commander *commander) : State(commander) {}

        virtual void handle() override;
    };

    class MoveState : public State {
    public:
        explicit MoveState(Commander *commander) : State(commander) {}

        virtual void handle() override;
    };

    class CjState : public State {
    public:
        explicit CjState(Commander *commander) : State(commander) {}

        virtual void handle() override;
    };


    class Commander : public rclcpp::Node {
    public:
        explicit Commander(const rclcpp::NodeOptions &options); // 重载构造函数

        ~Commander() override; // 析构函数

        rclcpp_action::Client<nav2_msgs::action::NavigateToPose>::SendGoalOptions send_goal_options;

        void nav_to_pose(geometry_msgs::msg::PoseStamped goal_pose);

        void
        goal_response_callback(rclcpp_action::ClientGoalHandle<nav2_msgs::action::NavigateToPose>::SharedPtr future);

        void feedback_callback(
                rclcpp_action::ClientGoalHandle<nav2_msgs::action::NavigateToPose>::SharedPtr future,
                const std::shared_ptr<const nav2_msgs::action::NavigateToPose::Feedback> feedback
        );

        void result_callback(
                const rclcpp_action::ClientGoalHandle<nav2_msgs::action::NavigateToPose>::WrappedResult &result);

        double distence(geometry_msgs::msg::PoseStamped a);

        void processPoses(std::vector<double> &pose_param, std::vector<geometry_msgs::msg::PoseStamped> &nav_points_);

        void getcurrentpose();

        std::vector<geometry_msgs::msg::PoseStamped> generateRandomPoints(int num_points, double radius);

        // double timer();

        geometry_msgs::msg::PoseStamped currentpose;
        // 导航点
        std::vector<geometry_msgs::msg::PoseStamped> Patrol_points_;
        std::vector<geometry_msgs::msg::PoseStamped> Route1_points_;
        std::vector<geometry_msgs::msg::PoseStamped> Route2_points_;
        std::vector<geometry_msgs::msg::PoseStamped> Route3_points_;
        std::vector<geometry_msgs::msg::PoseStamped> dafu_point_;
        std::vector<geometry_msgs::msg::PoseStamped> enemy_outpose_point_;
        std::vector<geometry_msgs::msg::PoseStamped> enemy_base_point_;
        std::vector<geometry_msgs::msg::PoseStamped> self_addhp_point_;
        std::vector<geometry_msgs::msg::PoseStamped> self_defend_points_;
        std::vector<geometry_msgs::msg::PoseStamped> chuji_points_;
        std::vector<geometry_msgs::msg::PoseStamped> move_points_;
        std::vector<std::vector<geometry_msgs::msg::PoseStamped>> list_name = {Route1_points_, Route2_points_,
                                                                               Route3_points_, dafu_point_,
                                                                               enemy_outpose_point_, enemy_base_point_,
                                                                               self_addhp_point_, self_defend_points_,
                                                                               chuji_points_};
        std::vector<geometry_msgs::msg::PoseStamped>::iterator random;
        std::vector<geometry_msgs::msg::PoseStamped>::iterator move;
        geometry_msgs::msg::PoseStamped goal;
        geometry_msgs::msg::PoseStamped home;
        std::shared_future<rclcpp_action::ClientGoalHandle<nav2_msgs::action::NavigateToPose>::SharedPtr> send_goal_future;

        bool checkgoal = true;
        bool first_start = false;
        bool control = false;
        std::chrono::steady_clock::time_point startTime;

        // 机器人血量
        Robot self_1 = Robot(1, geometry_msgs::msg::PoseStamped(), 150);
        Robot self_2 = Robot(2, geometry_msgs::msg::PoseStamped(), 150);
        Robot self_3 = Robot(3, geometry_msgs::msg::PoseStamped(), 150);
        Robot self_4 = Robot(4, geometry_msgs::msg::PoseStamped(), 150);
        Robot self_5 = Robot(5, geometry_msgs::msg::PoseStamped(), 150);
        Robot self_7 = Robot(7, geometry_msgs::msg::PoseStamped(), 600);
        Robot enemy_1 = Robot(1, geometry_msgs::msg::PoseStamped(), 150);
        Robot enemy_2 = Robot(2, geometry_msgs::msg::PoseStamped(), 150);
        Robot enemy_3 = Robot(3, geometry_msgs::msg::PoseStamped(), 150);
        Robot enemy_4 = Robot(4, geometry_msgs::msg::PoseStamped(), 150);
        Robot enemy_5 = Robot(5, geometry_msgs::msg::PoseStamped(), 150);
        Robot enemy_7 = Robot(7, geometry_msgs::msg::PoseStamped(), 600);

        uint enemy_outpost_hp;
        uint enemy_base_hp = 1500;

        int enemy_num = 0;

        uint enemyhp();

        double time = 500.0;
        double start;
        uint event_data;

        // 敌方机器人坐标
        geometry_msgs::msg::PoseStamped enemypose;
        bool tracking = false;
        bool buxue = false;


        //for bt
        int color; //1 red 2 blue
        bool gamestart = false;
        bool outpose = false;
        float self_hp = 400;
        float self_ammo = 400;
        float self_base = 400;
        float self_outpost = 400;
        int nav_state;  //1 for SUCCEEDED 2 for ABORTED 3 for CANCELED 4 for RUNNING
        bool order = false; //用于巡逻模式，详情见飞书
        float goldcoin;
        int buy_ammo;
        int buy_hp = 0;
        bool outpose_order_goal_reached = false;
        bool defend_order_goal_reached = false;
        int strategy;
        bool addhp_ordered;
        int buy_ammo_num;
        int remote_buy_ammo_times;
        int buy_hp_times;

    private:

        rclcpp_action::Client<nav2_msgs::action::NavigateToPose>::SharedPtr nav_to_pose_client;

        void decision();

        void executor();


        void setState(std::shared_ptr<State> state);

        void loadNavPoints();

        void aim_callback(const auto_aim_interfaces::msg::Target::SharedPtr msg);

        void serial_callback(const rm_decision_interfaces::msg::FromSerial::SharedPtr msg);

        void enemypose_callback(const geometry_msgs::msg::PointStamped::SharedPtr msg);
        // void cmd;


        std::shared_ptr<State> currentState;

        rclcpp::Subscription<rm_decision_interfaces::msg::FromSerial>::SharedPtr nav_sub_;
        rclcpp::Subscription<auto_aim_interfaces::msg::Target>::SharedPtr aim_sub_;
        rclcpp::Subscription<geometry_msgs::msg::PointStamped>::SharedPtr enemypose_sub_;
        rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr laser_sub_;

        rclcpp::Publisher<rm_decision_interfaces::msg::ToSerial>::SharedPtr sentry_cmd_pub_;

        std::thread commander_thread_;
        std::thread executor_thread_;

        std::shared_ptr<tf2_ros::Buffer> tf2_buffer_;
        std::shared_ptr<tf2_ros::TransformListener> tf2_listener_;
        // tf2_ros::Buffer buffer{get_clock()};
        // tf2_ros::TransformListener tflistener{buffer};

        //this is used for bt
        void mygimbal_handle() {
            std::cout << "mygimbal_handle is called" << std::endl;
        }
        void myGoToEnemyOutpose_handle() {
            std::cout << "mygimbal_handle is called" << std::endl;
        }
        void myGoToStopEngineer_handle() {
            std::cout << "mygimbal_handle is called" << std::endl;
        }
        void myGoToStopHero_handle() {
            std::cout << "mygimbal_handle is called" << std::endl;
        }
        void myS2GoToOutpose() {
            std::cout << "mygimbal_handle is called" << std::endl;
        }
        void myS3Patro(){
            std::cout << "myS3Patro" << std::endl;
        }

        void myoutpose_handle() {
            order = true;
            if (!outpose_order_goal_reached) {
                std::cout << "outpose_handle is called" << std::endl;
                goal.header.stamp = this->now();
                goal.header.frame_id = "map";
                goal.pose.position.x = enemy_outpose_point_[0].pose.position.x;
                goal.pose.position.y = enemy_outpose_point_[0].pose.position.y;
                goal.pose.position.z = enemy_outpose_point_[0].pose.position.z;
                goal.pose.orientation.x = 0.0;
                goal.pose.orientation.y = 0.0;
                goal.pose.orientation.z = 0.0;
                goal.pose.orientation.w = 1.0;
                setState(std::make_shared<GoAndStayState>(this));
                outpose_order_goal_reached = true;
            } else
                setState(std::make_shared<MoveState>(this));
        }


        // 上面都是进攻的
        void myaddhp_handle() {
            std::cout << "addhp_handle is called" << std::endl;
            goal.header.stamp = this->now();
            goal.header.frame_id = "map";
            goal.pose.position.x = self_addhp_point_[0].pose.position.x;
            goal.pose.position.y = self_addhp_point_[0].pose.position.y;
            goal.pose.position.z = self_addhp_point_[0].pose.position.z;
            goal.pose.orientation.x = 0.0;
            goal.pose.orientation.y = 0.0;
            goal.pose.orientation.z = 0.0;
            goal.pose.orientation.w = 1.0;
            setState(std::make_shared<GoAndStayState>(this));
            order = false;
        }

        void mydefend_handle() {
            order = true;
            if (!defend_order_goal_reached) {
                std::cout << "defend_handle is called" << std::endl;
                goal.header.stamp = this->now();
                goal.header.frame_id = "map";
                goal.pose.position.x = self_defend_points_[0].pose.position.x;
                goal.pose.position.y = self_defend_points_[0].pose.position.y;
                goal.pose.position.z = self_defend_points_[0].pose.position.z;
                goal.pose.orientation.x = 0.0;
                goal.pose.orientation.y = 0.0;
                goal.pose.orientation.z = 0.0;
                goal.pose.orientation.w = 1.0;
                setState(std::make_shared<GoAndStayState>(this));
            } else
                setState(std::make_shared<MoveState>(this));

        }

        void myattack_handle() {
            std::cout << "attack_handle is called" << std::endl;
            setState(std::make_shared<AttackState>(this));
            order = true;
        }

        void myguard_handle() {
            std::cout << "Guard_handle is called" << std::endl;
            setState(std::make_shared<PatrolState>(this));
        }

        void mymovearound_handle() {
            std::cout << "MoveAround_handle is called" << std::endl;
            setState(std::make_shared<MoveState>(this));
        }

        void mybuytorelive_handle() {
            rm_decision_interfaces::msg::ToSerial msg;
            msg.sentry_cmd |= (1 << 31);

            msg.sentry_cmd |= (1 << 30);

            sentry_cmd_pub_->publish(msg);
            std::bitset<32> binary(msg.sentry_cmd);
        }

        void mybuyammoremotely_handle() {
            rm_decision_interfaces::msg::ToSerial msg;



            sentry_cmd_pub_->publish(msg);
            std::bitset<32> binary(msg.sentry_cmd);

        }

        void mybuyhp_handle() {
            std::cout << "buyhp_handle is called" << std::endl;
        }

        BT::NodeStatus wait_for_start() {
            if (gamestart) {
                return BT::NodeStatus::FAILURE;
            } else {
                return BT::NodeStatus::SUCCESS;
            }
        }

        BT::NodeStatus IfAsked() {
            return BT::NodeStatus::SUCCESS;
        }



        BT::NodeStatus outpose_ordered() {
            if (outpose) {
                return BT::NodeStatus::SUCCESS;
            } else {
                outpose_order_goal_reached = false;
                return BT::NodeStatus::FAILURE;
            }
        }


        BT::NodeStatus IfAddHp() {
            if (self_hp <= 150) {
                addhp_ordered = true;
            }
            if (self_hp == 400) {
                addhp_ordered = false;
            }

            if (addhp_ordered && self_ammo < 50) {
                return BT::NodeStatus::SUCCESS;
            } else {
                return BT::NodeStatus::FAILURE;
            }
        }

        BT::NodeStatus IfDefend() {
            if (self_base <= 150) {
                return BT::NodeStatus::SUCCESS;
            } else {
                defend_order_goal_reached = false;
                return BT::NodeStatus::FAILURE;
            }
        }

        BT::NodeStatus IfAttack() {
            if (self_hp >= 1000 && distence(enemypose) <= 3.0) {
                return BT::NodeStatus::SUCCESS;
            } else {
                return BT::NodeStatus::FAILURE;
            }
        }

        BT::NodeStatus IfGuard() {
            return BT::NodeStatus::SUCCESS;
        }

        BT::NodeStatus IfBuyToRelive() {
            if (self_hp == 0 && goldcoin > 300) {
                return BT::NodeStatus::SUCCESS;
            } else return BT::NodeStatus::FAILURE;
        };

        BT::NodeStatus IfBuyAmmoRemotely() {
            if (self_hp >= 300 && goldcoin > 300 && self_ammo < 50 && self_outpost > 0) {
                return BT::NodeStatus::SUCCESS;
            }
            return BT::NodeStatus::FAILURE;
        }

        BT::NodeStatus IfBuyHp() {
            if(self_hp < 200 && goldcoin > 300){
                return BT::NodeStatus::SUCCESS;
            }
            else return BT::NodeStatus::FAILURE;
        }

        BT::NodeStatus IfBuyAmmo() {
            return BT::NodeStatus::FAILURE;
        }
        BT::NodeStatus IfGoToEnemyOutpose() {
            return BT::NodeStatus::FAILURE;
        }
        BT::NodeStatus IfGoToStopEngineer() {
            return BT::NodeStatus::FAILURE;
        }
        BT::NodeStatus IfGoToStopHero() {
            return BT::NodeStatus::FAILURE;
        }
        BT::NodeStatus IfOutposeAlive() {
            return BT::NodeStatus::FAILURE;
        }
        BT::NodeStatus S1() {
            return BT::NodeStatus::FAILURE;
        }
        BT::NodeStatus S2() {
            return BT::NodeStatus::FAILURE;
        }
        BT::NodeStatus S3() {
            return BT::NodeStatus::FAILURE;
        }



        BT::NodeStatus Gimbal_handle() {
            mygimbal_handle();
            return BT::NodeStatus::SUCCESS;
        }




        BT::NodeStatus outpose_handle() {
            myoutpose_handle();
            if (nav_state == 1) {
                return BT::NodeStatus::SUCCESS;
            } else {
                return BT::NodeStatus::RUNNING;
            }
        }



        BT::NodeStatus addhp_handle() {
            myaddhp_handle();
            if (nav_state == 1) {
                return BT::NodeStatus::SUCCESS;
            } else {
                return BT::NodeStatus::RUNNING;
            }
        }

        BT::NodeStatus defend_handle() {
            mydefend_handle();
            if (nav_state == 1) {
                return BT::NodeStatus::SUCCESS;
            } else {
                return BT::NodeStatus::RUNNING;
            }
        }

        BT::NodeStatus attack_handle() {
            myattack_handle();
            if (nav_state == 1) {
                return BT::NodeStatus::SUCCESS;
            } else {
                return BT::NodeStatus::RUNNING;
            }
        }

        BT::NodeStatus Guard() {
            myguard_handle();
            return BT::NodeStatus::SUCCESS;
        }

        BT::NodeStatus MoveAround_handle() {
            mymovearound_handle();
            return BT::NodeStatus::SUCCESS;
        }

        BT::NodeStatus BuyToRelive_handle() {
            mybuytorelive_handle();
            return BT::NodeStatus::SUCCESS;
        }

        BT::NodeStatus BuyAmmoRemotely_handle() {
            mybuyammoremotely_handle();
            return BT::NodeStatus::SUCCESS;
        }

        BT::NodeStatus BuyHp_handle() {
            mybuyhp_handle();
            return BT::NodeStatus::SUCCESS;
        }

        BT::NodeStatus GoToEnemyOutpose_handle() {
            myGoToEnemyOutpose_handle();
            return BT::NodeStatus::SUCCESS;
        }
        BT::NodeStatus BuyAmmo_handle() {
            myGoToEnemyOutpose_handle();
            return BT::NodeStatus::SUCCESS;
        }
        BT::NodeStatus GoToStopEngineer_handle() {
            myGoToStopEngineer_handle();
            return BT::NodeStatus::SUCCESS;
        }
        BT::NodeStatus GoToStopHero_handle() {
            myGoToStopHero_handle();
            return BT::NodeStatus::SUCCESS;
        }

        BT::NodeStatus S2GoToOutpose() {
            myS2GoToOutpose();
            return BT::NodeStatus::SUCCESS;
        }
        BT::NodeStatus S3Patro() {
            myS3Patro();
            return BT::NodeStatus::SUCCESS;
        }

        //above is used for bt
    };
}

#endif // RM_DECISION__COMMANDER_HPP_