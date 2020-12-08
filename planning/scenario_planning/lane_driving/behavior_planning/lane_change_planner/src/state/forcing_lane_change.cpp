// Copyright 2019 Autoware Foundation
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

#include "lane_change_planner/data_manager.hpp"
#include "lane_change_planner/route_handler.hpp"
#include "lane_change_planner/state/forcing_lane_change.hpp"
#include "lane_change_planner/utilities.hpp"

namespace lane_change_planner
{
ForcingLaneChangeState::ForcingLaneChangeState(
  const Status & status, const std::shared_ptr<DataManager> & data_manager_ptr,
  const std::shared_ptr<RouteHandler> & route_handler_ptr)
: StateBase(status, data_manager_ptr, route_handler_ptr)
{
}

State ForcingLaneChangeState::getCurrentState() const {return State::FORCING_LANE_CHANGE;}

void ForcingLaneChangeState::entry()
{
  ros_parameters_ = data_manager_ptr_->getLaneChangerParameters();
  original_lanes_ = route_handler_ptr_->getLaneletsFromIds(status_.lane_follow_lane_ids);
  target_lanes_ = route_handler_ptr_->getLaneletsFromIds(status_.lane_change_lane_ids);
}

autoware_planning_msgs::msg::PathWithLaneId ForcingLaneChangeState::getPath() const
{
  return status_.lane_change_path.path;
}

void ForcingLaneChangeState::update()
{
  current_pose_ = data_manager_ptr_->getCurrentSelfPose();

  // update path
  {
    lanelet::ConstLanelets lanes;
    lanes.insert(lanes.end(), original_lanes_.begin(), original_lanes_.end());
    lanes.insert(lanes.end(), target_lanes_.begin(), target_lanes_.end());

    const double width = ros_parameters_.drivable_area_width;
    const double height = ros_parameters_.drivable_area_height;
    const double resolution = ros_parameters_.drivable_area_resolution;
    status_.lane_change_path.path.drivable_area = util::generateDrivableArea(
      lanes, current_pose_, width, height, resolution, ros_parameters_.vehicle_length,
      *route_handler_ptr_);
  }
}

State ForcingLaneChangeState::getNextState() const
{
  if (hasFinishedLaneChange()) {
    return State::FOLLOWING_LANE;
  }
  return State::FORCING_LANE_CHANGE;
}

bool ForcingLaneChangeState::hasFinishedLaneChange() const
{
  const auto & clock = data_manager_ptr_->getClock();
  static rclcpp::Time start_time = clock->now();

  if (route_handler_ptr_->isInTargetLane(current_pose_, target_lanes_)) {
    return clock->now() - start_time > rclcpp::Duration::from_seconds(2.0);
  } else {
    start_time = clock->now();
  }
  return false;
}

}  // namespace lane_change_planner
