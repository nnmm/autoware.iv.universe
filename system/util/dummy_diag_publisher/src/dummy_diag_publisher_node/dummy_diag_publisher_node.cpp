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

#include "dummy_diag_publisher/dummy_diag_publisher_node.hpp"

#include "rclcpp/create_timer.hpp"

namespace
{
template<typename T>
void update_param(
  const std::vector<rclcpp::Parameter> & parameters, const std::string & name, T & value)
{
  auto it = std::find_if(
    parameters.cbegin(), parameters.cend(),
    [&name](const rclcpp::Parameter & parameter) {return parameter.get_name() == name;});
  if (it != parameters.cend()) {
    value = it->template get_value<T>();
  }
}
}  // namespace

rcl_interfaces::msg::SetParametersResult DummyDiagPublisherNode::paramCallback(
  const std::vector<rclcpp::Parameter> & parameters)
{

  rcl_interfaces::msg::SetParametersResult result;
  result.successful = true;
  result.reason = "success";

  DummyDiagPublisherConfig config = config_;
  try {
    int status = static_cast<int>(config.status);
    update_param(parameters, "status", status);
    config.status = Status(status);
    update_param(parameters, "is_active", config.is_active);
    config_ = config;
  } catch (const rclcpp::exceptions::InvalidParameterTypeException & e) {
    result.successful = false;
    result.reason = e.what();
  }

  return result;
}

void DummyDiagPublisherNode::produceDiagnostics(diagnostic_updater::DiagnosticStatusWrapper & stat)
{
  diagnostic_msgs::msg::DiagnosticStatus status;

  if (config_.status == Status::OK) {
    status.level = diagnostic_msgs::msg::DiagnosticStatus::OK;
    status.message = diag_config_.msg_ok;
  } else if (config_.status == Status::WARN) {
    status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
    status.message = diag_config_.msg_warn;
  } else if (config_.status == Status::ERROR) {
    status.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
    status.message = diag_config_.msg_error;
  } else if (config_.status == Status::STALE) {
    status.level = diagnostic_msgs::msg::DiagnosticStatus::STALE;
    status.message = diag_config_.msg_stale;
  } else {
    throw std::runtime_error("invalid status");
  }

  stat.summary(status.level, status.message);
}

void DummyDiagPublisherNode::onTimer()
{
  if (config_.is_active) {
    updater_.force_update();
  }
}

DummyDiagPublisherNode::DummyDiagPublisherNode()
: Node("dummy_diag_publisher"),
  update_rate_(declare_parameter("update_rate", 10.0)),
  updater_(this)
{
  // Get configuration
  std::map<std::string, std::string> configuration_parameters;
  configuration_parameters.insert(std::pair<std::string, std::string>("diag_config.name", ""));
  configuration_parameters.insert(
    std::pair<std::string, std::string>(
      "diag_config.hardware_id",
      ""));
  configuration_parameters.insert(std::pair<std::string, std::string>("diag_config.msg_ok", ""));
  configuration_parameters.insert(std::pair<std::string, std::string>("diag_config.msg_warn", ""));
  configuration_parameters.insert(std::pair<std::string, std::string>("diag_config.msg_error", ""));
  configuration_parameters.insert(std::pair<std::string, std::string>("diag_config.msg_stale", ""));

  this->declare_parameters<std::string>("", configuration_parameters);
  this->get_parameters("diag_config", configuration_parameters);
  diag_config_ = DiagConfig(configuration_parameters);

  // set parameter callback
  config_.status = static_cast<Status>(this->declare_parameter("status", 0));
  config_.is_active = this->declare_parameter("is_active", false);
  set_param_res_ =
    this->add_on_set_parameters_callback(
    std::bind(
      &DummyDiagPublisherNode::paramCallback, this,
      std::placeholders::_1));

  // Diagnostic Updater
  updater_.setHardwareID(diag_config_.hardware_id);
  updater_.add(diag_config_.name, this, &DummyDiagPublisherNode::produceDiagnostics);

  // Timer
  auto timer_callback = std::bind(&DummyDiagPublisherNode::onTimer, this);
  auto period = std::chrono::duration_cast<std::chrono::nanoseconds>(
    std::chrono::duration<double>(update_rate_));

  timer_ = std::make_shared<rclcpp::GenericTimer<decltype(timer_callback)>>(
    this->get_clock(), period, std::move(timer_callback),
    this->get_node_base_interface()->get_context());
  this->get_node_timers_interface()->add_timer(timer_, nullptr);
}
