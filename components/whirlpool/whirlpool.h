#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace whirlpool {

  union WhirlpoolProtocol{
    uint8_t raw[21] = {0};  ///< The state in IR code form
    struct {
      // Byte 0~1
      uint8_t pad0[2];
      // Byte 2
      uint8_t Fan     :2;
      uint8_t Power   :1;
      uint8_t Sleep   :1;
      uint8_t AutoTempOffset :3;
      uint8_t Swing1  :1;
      // Byte 3
      uint8_t Mode  :3;
      uint8_t       :1;
      uint8_t Temp  :4;
      // Byte 4
      uint8_t Auto6  :8;
      // Byte 5
      uint8_t         :4;
      uint8_t Super1  :1;
      uint8_t         :2;
      uint8_t Super2  :1;
      // Byte 6
      uint8_t ClockHours  :5;
      uint8_t LightOff    :1;
      uint8_t             :2;
      // Byte 7
      uint8_t ClockMins       :6;
      uint8_t                 :1;
      uint8_t OffTimerEnabled :1;
      // Byte 8
      uint8_t OffHours  :5;
      uint8_t           :1;
      uint8_t Swing2    :1;
      uint8_t           :1;
      // Byte 9
      uint8_t OffMins         :6;
      uint8_t                 :1;
      uint8_t OnTimerEnabled  :1;
      // Byte 10
      uint8_t OnHours :5;
      uint8_t         :3;
      // Byte 11
      uint8_t OnMins  :6;
      uint8_t         :2;
      // Byte 12
      uint8_t       :8;
      // Byte 13
      uint8_t Sum1  :8;
      // Byte 14
      uint8_t       :8;
      // Byte 15
      uint8_t Cmd   :8;
      // Byte 16~17
      uint8_t pad1[2];
      // Byte 18
      uint8_t       :3;
      uint8_t J191  :1;
      uint8_t       :4;
      // Byte 19
      uint8_t       :8;
      // Byte 20
      uint8_t Sum2  :8;
    };
  };

/// Simple enum to represent models.
enum Model {
  MODEL_DG11J1_3A = 0,  /// Temperature range is from 18 to 32
  MODEL_DG11J1_91 = 1,  /// Temperature range is from 16 to 30
};

// Temperature
const float WHIRLPOOL_DG11J1_3A_TEMP_MAX = 32.0;
const float WHIRLPOOL_DG11J1_3A_TEMP_MIN = 18.0;
const float WHIRLPOOL_DG11J1_91_TEMP_MAX = 30.0;
const float WHIRLPOOL_DG11J1_91_TEMP_MIN = 16.0;

class WhirlpoolClimate : public climate_ir::ClimateIR {
 public:
  WhirlpoolClimate()
      : climate_ir::ClimateIR(temperature_min_(), temperature_max_(), 1.0f, true, true,
                              {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                               climate::CLIMATE_FAN_HIGH},
                              {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL}) {}

  void setup() override {
    climate_ir::ClimateIR::setup();

    this->powered_on_assumed = this->mode != climate::CLIMATE_MODE_OFF;
  }

  /// Override control to change settings of the climate device.
  void control(const climate::ClimateCall &call) override {
    send_swing_cmd_ = call.get_swing_mode().has_value();
    temp_change = call.get_target_temperature().has_value();
    mode_change = call.get_mode().has_value();
    climate_ir::ClimateIR::control(call);
  }

  void set_model(Model model) { this->model_ = model; }

  // used to track when to send the power toggle command
  bool powered_on_assumed;

  bool recent_transmit;
  bool transmit_enabled = true;

  void set_transmit_enabled(bool value) {
    this->transmit_enabled = value;
  }

 protected:
  /// Transmit via IR the state of this climate controller.
  void transmit_state() override;
  /// Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;

  bool send_swing_cmd_{false};
  bool temp_change{false};
  bool mode_change{false};
  Model model_;

  float temperature_min_() {
    return (model_ == MODEL_DG11J1_3A) ? WHIRLPOOL_DG11J1_3A_TEMP_MIN : WHIRLPOOL_DG11J1_91_TEMP_MIN;
  }
  float temperature_max_() {
    return (model_ == MODEL_DG11J1_3A) ? WHIRLPOOL_DG11J1_3A_TEMP_MAX : WHIRLPOOL_DG11J1_91_TEMP_MAX;
  }
};

}  // namespace whirlpool
}  // namespace esphome
