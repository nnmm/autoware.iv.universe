/*
 * Copyright 2020 Tier IV, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <awapi_awiv_adapter/awapi_obstacle_avoidance_state_publisher.h>

namespace autoware_api
{
AutowareIvObstacleAvoidanceStatePublisher::AutowareIvObstacleAvoidanceStatePublisher()
: Node("awapi_awiv_obstacle_avoidance_state_publisher_node")
{
  // publisher
  pub_state_ = this->create_publisher<autoware_api_msgs::msg::ObstacleAvoidanceStatus>(
    "output/obstacle_avoid_status", 1);
}

void AutowareIvObstacleAvoidanceStatePublisher::statePublisher(const AutowareInfo & aw_info)
{
  autoware_api_msgs::msg::ObstacleAvoidanceStatus status;

  //input header
  status.header.frame_id = "base_link";
  status.header.stamp = this->now();

  // get all info
  getObstacleAvoidReadyInfo(aw_info.obstacle_avoid_ready_ptr, &status);
  getCandidatePathInfo(aw_info.obstacle_avoid_candidate_ptr, &status);

  // publish info
  pub_state_->publish(status);
}

void AutowareIvObstacleAvoidanceStatePublisher::getObstacleAvoidReadyInfo(
  const std_msgs::msg::Bool::ConstSharedPtr & ready_ptr,
  autoware_api_msgs::msg::ObstacleAvoidanceStatus * status)
{
  if (!ready_ptr) {
    RCLCPP_DEBUG_STREAM_THROTTLE(
      get_logger(), *this->get_clock(), 5.0, "obstacle_avoidance_ready is nullptr");
    return;
  }

  status->obstacle_avoidance_ready = ready_ptr->data;
}

void AutowareIvObstacleAvoidanceStatePublisher::getCandidatePathInfo(
  const autoware_planning_msgs::msg::Trajectory::ConstSharedPtr & path_ptr,
  autoware_api_msgs::msg::ObstacleAvoidanceStatus * status)
{
  if (!path_ptr) {
    RCLCPP_DEBUG_STREAM_THROTTLE(
      get_logger(), *this->get_clock(), 5.0,
      "obstacle_avoidance_candidate_path is "
      "nullptr");
    return;
  }

  status->candidate_path = *path_ptr;
}

}  // namespace autoware_api
