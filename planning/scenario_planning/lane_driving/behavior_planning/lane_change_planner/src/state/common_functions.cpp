// Copyright 2020 Tier IV, Inc.
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
#include "lane_change_planner/state/common_functions.hpp"
#include "lane_change_planner/utilities.hpp"
#include "lanelet2_extension/utility/utilities.hpp"

namespace lane_change_planner
{
namespace state_machine
{
namespace common_functions
{
bool selectLaneChangePath(
  const std::vector<LaneChangePath> & paths, const lanelet::ConstLanelets & current_lanes,
  const lanelet::ConstLanelets & target_lanes,
  const lanelet::routing::RoutingGraphContainer & overall_graphs,
  const autoware_perception_msgs::msg::DynamicObjectArray::ConstSharedPtr & dynamic_objects,
  const geometry_msgs::msg::Pose & current_pose, const geometry_msgs::msg::Twist & current_twist,
  const bool isInGoalRouteSection, const geometry_msgs::msg::Pose & goal_pose,
  const LaneChangerParameters & ros_parameters, LaneChangePath * selected_path, const rclcpp::Logger & logger, const rclcpp::Clock::SharedPtr & clock)
{
  for (const auto & path : paths) {
    if (!isLaneChangePathSafe(
          path.path, current_lanes, target_lanes, dynamic_objects, current_pose, current_twist,
          ros_parameters, logger, clock, true, path.acceleration)) {
      continue;
    }
    if (!hasEnoughDistance(
          path, current_lanes, target_lanes, current_pose, isInGoalRouteSection, goal_pose,
          overall_graphs)) {
      continue;
    }
    *selected_path = path;
    return true;
  }

  // set first path for force lane change if no valid path found
  if (!paths.empty()) {
    *selected_path = paths.front();
    return false;
  }

  return false;
}

bool hasEnoughDistance(
  const LaneChangePath & path, const lanelet::ConstLanelets & current_lanes,
  const lanelet::ConstLanelets & target_lanes, const geometry_msgs::msg::Pose & current_pose,
  const bool isInGoalRouteSection, const geometry_msgs::msg::Pose & goal_pose,
  const lanelet::routing::RoutingGraphContainer & overall_graphs)
{
  const double lane_change_prepare_distance = path.preparation_length;
  const double lane_changing_distance = path.lane_change_length;
  const double lane_change_total_distance = lane_change_prepare_distance + lane_changing_distance;

  if (lane_change_total_distance > util::getDistanceToEndOfLane(current_pose, current_lanes)) {
    return false;
  }

  if (
    lane_change_total_distance > util::getDistanceToNextIntersection(current_pose, current_lanes)) {
    return false;
  }

  if (
    isInGoalRouteSection &&
    lane_change_total_distance > util::getSignedDistance(current_pose, goal_pose, current_lanes)) {
    return false;
  }

  if (
    lane_change_total_distance >
    util::getDistanceToCrosswalk(current_pose, current_lanes, overall_graphs)) {
    return false;
  }
  return true;
}

bool isLaneChangePathSafe(
  const autoware_planning_msgs::msg::PathWithLaneId & path,
  const lanelet::ConstLanelets & current_lanes, const lanelet::ConstLanelets & target_lanes,
  const autoware_perception_msgs::msg::DynamicObjectArray::ConstSharedPtr & dynamic_objects,
  const geometry_msgs::msg::Pose & current_pose, const geometry_msgs::msg::Twist & current_twist,
  const LaneChangerParameters & ros_parameters, const rclcpp::Logger & logger, const rclcpp::Clock::SharedPtr & clock, const bool use_buffer, const double acceleration)
{
  if (path.points.empty()) {
    return false;
  }
  if (target_lanes.empty() || current_lanes.empty()) {
    return false;
  }
  if (dynamic_objects == nullptr) {
    return true;
  }
  const auto arc = lanelet::utils::getArcCoordinates(current_lanes, current_pose);
  constexpr double check_distance = 100.0;

  // parameters
  const double time_resolution = ros_parameters.prediction_time_resolution;
  const double target_lane_check_start_time = 0.0;
  const double target_lane_check_end_time =
    ros_parameters.lane_change_prepare_duration + ros_parameters.lane_changing_duration;
  const double min_thresh = ros_parameters.min_stop_distance;
  const double stop_time = ros_parameters.stop_time;
  const double vehicle_width = ros_parameters.vehicle_width;
  double buffer;
  double lateral_buffer;
  if (use_buffer) {
    buffer = ros_parameters.hysteresis_buffer_distance;
    lateral_buffer = 0.5;
  } else {
    buffer = 0.0;
    lateral_buffer = 0.0;
  }

  // find obstacle in lane change target lanes
  // retrieve lanes that are merging target lanes as well
  const auto target_lane_object_indices =
    util::filterObjectsByLanelets(*dynamic_objects, target_lanes, logger);

  // find objects in current lane
  const auto current_lane_object_indices_lanelet = util::filterObjectsByLanelets(
    *dynamic_objects, current_lanes, arc.length, arc.length + check_distance, logger);
  const auto current_lane_object_indices = util::filterObjectsByPath(
    *dynamic_objects, current_lane_object_indices_lanelet, path,
    vehicle_width / 2 + lateral_buffer, logger);

  const auto & vehicle_predicted_path = util::convertToPredictedPath(
    path, current_twist, current_pose, target_lane_check_end_time, time_resolution, acceleration, logger, clock);

  for (const auto & i : current_lane_object_indices) {
    const auto & obj = dynamic_objects->objects.at(i);
    for (const auto & obj_path : obj.state.predicted_paths) {
      double distance = util::getDistanceBetweenPredictedPaths(
        obj_path, vehicle_predicted_path, target_lane_check_start_time, target_lane_check_end_time,
        time_resolution, logger, clock);
      double thresh;
      if (isObjectFront(current_pose, obj.state.pose_covariance.pose)) {
        thresh = util::l2Norm(current_twist.linear) * stop_time;
      } else {
        thresh = util::l2Norm(obj.state.twist_covariance.twist.linear) * stop_time;
      }
      thresh = std::max(thresh, min_thresh);
      thresh += buffer;
      if (distance < thresh) {
        return false;
      }
    }
  }

  for (const auto & i : target_lane_object_indices) {
    const auto & obj = dynamic_objects->objects.at(i);
    for (const auto & obj_path : obj.state.predicted_paths) {
      double distance = util::getDistanceBetweenPredictedPaths(
        obj_path, vehicle_predicted_path, target_lane_check_start_time, target_lane_check_end_time,
        time_resolution, logger, clock);
      double thresh;
      if (isObjectFront(current_pose, obj.state.pose_covariance.pose)) {
        thresh = util::l2Norm(current_twist.linear) * stop_time;
      } else {
        thresh = util::l2Norm(obj.state.twist_covariance.twist.linear) * stop_time;
      }
      thresh = std::max(thresh, min_thresh);
      thresh += buffer;
      if (distance < thresh) {
        return false;
      }
    }
  }

  return true;
}

bool isObjectFront(
  const geometry_msgs::msg::Pose & ego_pose, const geometry_msgs::msg::Pose & obj_pose)
{
  tf2::Transform tf_map2ego, tf_map2obj;
  geometry_msgs::msg::Pose obj_from_ego;
  tf2::fromMsg(ego_pose, tf_map2ego);
  tf2::fromMsg(obj_pose, tf_map2obj);
  tf2::toMsg(tf_map2ego.inverse() * tf_map2obj, obj_from_ego);

  return obj_from_ego.position.x > 0;
}

}  // namespace common_functions
}  // namespace state_machine
}  // namespace lane_change_planner
