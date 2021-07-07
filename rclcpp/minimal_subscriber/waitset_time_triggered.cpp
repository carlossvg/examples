// Copyright 2021, Apex.AI Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

/* This example creates a subclass of Node and uses a wait-set based loop and polls periodically
 * for messages in a timer callback using a wait-set based loop. */

class MinimalSubscriber : public rclcpp::Node
{
public:
  MinimalSubscriber()
  : Node("minimal_subscriber")
  {
    subscription_ = this->create_subscription<std_msgs::msg::String>(
      "topic",
      1,
      [this](std_msgs::msg::String::UniquePtr msg) {
        RCLCPP_INFO(this->get_logger(), "I heard: '%s'", msg->data.c_str());
      });
    auto timer_callback = [this]() -> void {
        std_msgs::msg::String msg;
        rclcpp::MessageInfo msg_info;
        if (subscription_->take(msg, msg_info)) {
          std::shared_ptr<void> type_erased_msg = std::make_shared<std_msgs::msg::String>(msg);
          subscription_->handle_message(type_erased_msg, msg_info);
        } else {
          RCLCPP_INFO(this->get_logger(), "No message available");
        }
      };
    timer_ = create_wall_timer(500ms, timer_callback);
    wait_set_.add_timer(timer_);
  }

  void run()
  {
    while (rclcpp::ok()) {
      // Wait for the timer event to trigger. Set a 1 ms margin to trigger a timeout.
      const auto wait_result = wait_set_.wait(501ms);
      if (wait_result.kind() == rclcpp::WaitResultKind::Ready) {
        if (wait_result.get_wait_set().get_rcl_wait_set().timers[0U]) {
          timer_->execute_callback();
        }
      } else if (wait_result.kind() == rclcpp::WaitResultKind::Timeout) {
        if (rclcpp::ok()) {
          RCLCPP_ERROR(this->get_logger(), "Wait-set failed with timeout");
        }
      }
    }
  }

private:
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::WaitSet wait_set_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<MinimalSubscriber>();
  node->run();
  rclcpp::shutdown();
  return 0;
}