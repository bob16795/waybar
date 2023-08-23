#pragma once

#include <gtkmm/button.h>
#include <wayland-client.h>

#include "ALabel.hpp"
#include "bar.hpp"
#include "dwl-ipc-unstable-v2-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

namespace waybar::modules::dwl {

class Title : public waybar::ALabel {
 public:
  Title(const std::string &, const waybar::Bar &, const Json::Value &);
  virtual ~Title();

  struct zdwl_ipc_manager_v2 *status_manager_;
  struct wl_seat *seat_;

  auto update() -> void override;

 private:
  const waybar::Bar &bar_;
  struct zdwl_ipc_output_v2 *output_status_;
};

} /* namespace waybar::modules::dwl */
