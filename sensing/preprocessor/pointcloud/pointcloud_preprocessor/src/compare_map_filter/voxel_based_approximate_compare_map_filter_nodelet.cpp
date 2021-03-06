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

#include "pointcloud_preprocessor/compare_map_filter/voxel_based_approximate_compare_map_filter_nodelet.h"
#include <pluginlib/class_list_macros.h>

#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/search/kdtree.h>
#include <pcl/segmentation/segment_differences.h>

namespace pointcloud_preprocessor
{
bool VoxelBasedApproximateCompareMapFilterNodelet::child_init(
  ros::NodeHandle & nh, bool & has_service)
{
  set_map_in_voxel_grid_ = false;
  // Enable the dynamic reconfigure service
  has_service = true;
  srv_ = boost::make_shared<
    dynamic_reconfigure::Server<pointcloud_preprocessor::CompareMapFilterConfig> >(nh);
  dynamic_reconfigure::Server<pointcloud_preprocessor::CompareMapFilterConfig>::CallbackType f =
    boost::bind(&VoxelBasedApproximateCompareMapFilterNodelet::config_callback, this, _1, _2);
  srv_->setCallback(f);
  return (true);
}

void VoxelBasedApproximateCompareMapFilterNodelet::filter(
  const PointCloud2::ConstPtr & input, const IndicesPtr & indices, PointCloud2 & output)
{
  boost::mutex::scoped_lock lock(mutex_);
  if (voxel_map_ptr_ == NULL) {
    output = *input;
    return;
  }
  pcl::PointCloud<pcl::PointXYZ>::Ptr pcl_input(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr pcl_output(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::fromROSMsg(*input, *pcl_input);
  pcl_output->points.reserve(pcl_input->points.size());
  for (size_t i = 0; i < pcl_input->points.size(); ++i) {
    const int index = voxel_grid_.getCentroidIndexAt(voxel_grid_.getGridCoordinates(
      pcl_input->points.at(i).x, pcl_input->points.at(i).y, pcl_input->points.at(i).z));
    if (index == -1)  // empty voxel
    {
      // map_ptr_->points.at(index)
      pcl_output->points.push_back(pcl_input->points.at(i));
    }
  }

  pcl::toROSMsg(*pcl_output, output);
  output.header = input->header;
}

void VoxelBasedApproximateCompareMapFilterNodelet::input_target_callback(
  const PointCloudConstPtr & map)
{
  boost::mutex::scoped_lock lock(mutex_);
  set_map_in_voxel_grid_ = true;
  tf_input_frame_ = map->header.frame_id;
  voxel_map_ptr_.reset(new pcl::PointCloud<pcl::PointXYZ>);
  voxel_grid_.setLeafSize(distance_threshold_, distance_threshold_, distance_threshold_);
  voxel_grid_.setInputCloud(map);
  voxel_grid_.setSaveLeafLayout(true);
  voxel_grid_.filter(*voxel_map_ptr_);
}

void VoxelBasedApproximateCompareMapFilterNodelet::subscribe()
{
  Filter::subscribe();
  sub_map_ = pnh_->subscribe(
    "map", 1, &VoxelBasedApproximateCompareMapFilterNodelet::input_target_callback, this);
}

void VoxelBasedApproximateCompareMapFilterNodelet::unsubscribe()
{
  Filter::unsubscribe();
  sub_map_.shutdown();
}

void VoxelBasedApproximateCompareMapFilterNodelet::config_callback(
  pointcloud_preprocessor::CompareMapFilterConfig & config, uint32_t level)
{
  boost::mutex::scoped_lock lock(mutex_);

  if (distance_threshold_ != config.distance_threshold) {
    distance_threshold_ = config.distance_threshold;
    voxel_grid_.setLeafSize(distance_threshold_, distance_threshold_, distance_threshold_);
    voxel_grid_.setSaveLeafLayout(true);
    if (set_map_in_voxel_grid_) voxel_grid_.filter(*voxel_map_ptr_);
    NODELET_DEBUG(
      "[%s::config_callback] Setting new distance threshold to: %f.", getName().c_str(),
      config.distance_threshold);
  }
  // ---[ These really shouldn't be here, and as soon as dynamic_reconfigure improves, we'll remove them and inherit
  // from Filter
  if (tf_output_frame_ != config.output_frame) {
    tf_output_frame_ = config.output_frame;
    NODELET_DEBUG(
      "[config_callback] Setting the output TF frame to: %s.", tf_output_frame_.c_str());
  }
  // ]---
}

}  // namespace pointcloud_preprocessor

PLUGINLIB_EXPORT_CLASS(
  pointcloud_preprocessor::VoxelBasedApproximateCompareMapFilterNodelet, nodelet::Nodelet);