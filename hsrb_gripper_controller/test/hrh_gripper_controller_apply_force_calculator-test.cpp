/*
Copyright (c) 2018 TOYOTA MOTOR CORPORATION
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted (subject to the limitations in the disclaimer
below) provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its contributors may be used
  to endorse or promote products derived from this software without specific
  prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS
LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/
#include <vector>
#include <gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include <hsrb_gripper_controller/hrh_gripper_apply_force_action.hpp>

namespace {

// キャリブレーションファイルパス
const char* const kCalibrationFile = "/tmp/test.yaml";

}  // unnamed namespace

namespace hsrb_gripper_controller {

class HrhGripperControllerApplyForceCalculatorTest : public ::testing::Test {
 public:
  HrhGripperControllerApplyForceCalculatorTest() : logger_(rclcpp::get_logger("test_logger")) {}

 protected:
  rclcpp::Logger logger_;
};

class ForceCalibrationFile {
 public:
  ForceCalibrationFile(const std::vector<std::vector<double>>& left_force,
                       const std::vector<std::vector<double>>& right_force) {
    YAML::Emitter emitter;
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "hand_left_force";
    emitter << YAML::Value << YAML::BeginSeq;
    for (size_t i = 0; i < left_force.size(); i++) {
      emitter << YAML::Flow << YAML::BeginSeq;
      for (size_t j = 0; j < left_force[i].size(); j++) {
        emitter << left_force[i][j];
      }
      emitter << YAML::EndSeq;
    }
    emitter << YAML::EndSeq;
    emitter << YAML::Key << "hand_right_force";
    emitter << YAML::Value << YAML::BeginSeq;
    for (size_t i = 0; i < right_force.size(); i++) {
      emitter << YAML::Flow << YAML::BeginSeq;
      for (size_t j = 0; j < right_force[i].size(); j++) {
        emitter << right_force[i][j];
      }
      emitter << YAML::EndSeq;
    }
    emitter << YAML::EndSeq;
    emitter << YAML::Key << "hand_spring" << YAML::Value << 5.0;
    emitter << YAML::Key << "arm_length" << YAML::Value << 7.0;
    emitter << YAML::EndMap;

    FILE* f = fopen(kCalibrationFile, "w");
    fprintf(f, "%s\n", emitter.c_str());
    fclose(f);
  }
  ~ForceCalibrationFile() {
    remove(kCalibrationFile);
  }
};

// 挟み込み力計算においてキャリブファイルを正しく読み込んで補正された力を取得できる
TEST_F(HrhGripperControllerApplyForceCalculatorTest, ApplyForceActionCalculatorSuccess1) {
  std::vector<std::vector<double>> force(2);
  force[0].push_back(0.0);
  force[0].push_back(0.1);
  force[1].push_back(1.0);
  force[1].push_back(0.1);
  ForceCalibrationFile force_calibration_file(force, force);

  auto force_calculator = std::make_shared<HrhGripperApplyForceCalculator>(kCalibrationFile, logger_);
  EXPECT_EQ(((3.0 * 5.0 / 7.0 - 0.1) + (5.0 * 5.0 / 7.0 - 0.1)) / 2,
            force_calculator->GetCurrentForce(0.5, 3.0, 5.0));
  EXPECT_EQ(0.0,
            force_calculator->GetCurrentForce(0.5, -3.0, -5.0));
}

// 挟み込み力計算においてhand_motor_posがキャリブの最大値より大きいとき、力は0.0を返す
TEST_F(HrhGripperControllerApplyForceCalculatorTest, ApplyForceActionCalculatorSuccess2) {
  std::vector<std::vector<double>> force(2);
  force[0].push_back(0.0);
  force[0].push_back(0.1);
  force[1].push_back(1.0);
  force[1].push_back(0.1);
  ForceCalibrationFile force_calibration_file(force, force);

  auto force_calculator = std::make_shared<HrhGripperApplyForceCalculator>(kCalibrationFile, logger_);
  EXPECT_EQ(0.0,
            force_calculator->GetCurrentForce(1.5, 3.0, 5.0));
  EXPECT_EQ(0.0,
            force_calculator->GetCurrentForce(1.5, -3.0, -5.0));
}

// 挟み込み力計算においてhand_motor_posがキャリブの最小値より小さいとき、力はcalib_points[0][1]との差と0.0の大きい方を返す
TEST_F(HrhGripperControllerApplyForceCalculatorTest, ApplyForceActionCalculatorSuccess3) {
  std::vector<std::vector<double>> force(2);
  force[0].push_back(0.0);
  force[0].push_back(0.1);
  force[1].push_back(1.0);
  force[1].push_back(0.1);
  ForceCalibrationFile force_calibration_file(force, force);

  auto force_calculator = std::make_shared<HrhGripperApplyForceCalculator>(kCalibrationFile, logger_);
  EXPECT_EQ(((3.0 * 5.0 / 7.0 - 0.1) + (5.0 * 5.0 / 7.0 - 0.1)) / 2,
            force_calculator->GetCurrentForce(-0.5, 3.0, 5.0));
  EXPECT_EQ(0.0,
            force_calculator->GetCurrentForce(-0.5, -3.0, -5.0));
}

// 挟み込み力計算においてキャリブの点が１つしかなければ力は0.0を返す
TEST_F(HrhGripperControllerApplyForceCalculatorTest, ApplyForceActionCalculatorFailure1) {
  std::vector<std::vector<double>> force(1);
  force[0].push_back(0.0);
  force[0].push_back(0.1);
  ForceCalibrationFile force_calibration_file(force, force);

  auto force_calculator = std::make_shared<HrhGripperApplyForceCalculator>(kCalibrationFile, logger_);
  EXPECT_EQ(0.0,
            force_calculator->GetCurrentForce(0.5, 3.0, 5.0));
  EXPECT_EQ(0.0,
            force_calculator->GetCurrentForce(0.5, -3.0, -5.0));
}

// 挟み込み力計算においてキャリブの点が2次元で表されていなければキャリブ値を考慮せずに力を返す
TEST_F(HrhGripperControllerApplyForceCalculatorTest, ApplyForceActionCalculatorFailure2) {
  std::vector<std::vector<double>> left_force(1);
  left_force[0].push_back(0.1);
  std::vector<std::vector<double>> right_force(3);
  right_force[0].push_back(1.0);
  right_force[0].push_back(0.1);
  right_force[0].push_back(0.0);
  ForceCalibrationFile force_calibration_file(left_force, right_force);

  auto force_calculator = std::make_shared<HrhGripperApplyForceCalculator>(kCalibrationFile, logger_);
  EXPECT_EQ((3.0 * 5.0 / 7.0 + 5.0 * 5.0 / 7.0) / 2,
            force_calculator->GetCurrentForce(0.5, 3.0, 5.0));
  EXPECT_EQ((-3.0 * 5.0 / 7.0 + -5.0 * 5.0 / 7.0) / 2,
            force_calculator->GetCurrentForce(0.5, -3.0, -5.0));
}

// 挟み込み力計算においてキャリブファイルが存在しなければデフォルト値で計算した力を返す
TEST_F(HrhGripperControllerApplyForceCalculatorTest, ApplyForceActionCalculatorFailure3) {
  auto force_calculator = std::make_shared<HrhGripperApplyForceCalculator>(kCalibrationFile, logger_);
  EXPECT_EQ((3.0 * 1.0 / 1.0 + 5.0 * 1.0 / 1.0) / 2,
            force_calculator->GetCurrentForce(0.5, 3.0, 5.0));
  EXPECT_EQ((-3.0 * 1.0 / 1.0 + -5.0 * 1.0 / 1.0) / 2,
            force_calculator->GetCurrentForce(0.5, -3.0, -5.0));
}

}  // namespace hsrb_gripper_controller

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
