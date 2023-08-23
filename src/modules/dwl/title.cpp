#include "modules/dwl/title.hpp"

#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <spdlog/spdlog.h>
#include <wayland-client.h>

#include <algorithm>

#include "client.hpp"
#include "dwl-ipc-unstable-v2-client-protocol.h"

#define TAG_INACTIVE 0
#define TAG_ACTIVE 1
#define TAG_URGENT 2

namespace waybar::modules::dwl {

/* dwl stuff */
static std::string active_title = "";
static std::string active_layout = "";
static uint32_t active_tag = 0;

void toggle_visibility(void *data, zdwl_ipc_output_v2 *zdwl_output_v2) {
  // Intentionally empty
}

void active(void *data, zdwl_ipc_output_v2 *zdwl_output_v2, uint32_t active) {
  // Intentionally empty
}

static void set_tag(void *data, zdwl_ipc_output_v2 *zdwl_output_v2, uint32_t tag, uint32_t state,
                    uint32_t clients, uint32_t focused) {
    if (state & TAG_ACTIVE) active_tag = tag + 1;
    (static_cast<Title *>(data))->update();
}

void set_layout_symbol(void *data, zdwl_ipc_output_v2 *zdwl_output_v2, const char *layout) {
  active_layout = layout;
  (static_cast<Title *>(data))->update();
}

void title(void *data, zdwl_ipc_output_v2 *zdwl_output_v2, const char *title) {
  active_title = std::string(title);
  (static_cast<Title *>(data))->update();
}

void dwl_frame(void *data, zdwl_ipc_output_v2 *zdwl_output_v2) {
  // Intentionally empty
}

static void set_layout(void *data, zdwl_ipc_output_v2 *zdwl_output_v2, uint32_t layout) {
  // Intentionally empty
}

static void appid(void *data, zdwl_ipc_output_v2 *zdwl_output_v2, const char *appid){
    // Intentionally empty
};

static const zdwl_ipc_output_v2_listener output_status_listener_impl{
    .toggle_visibility = toggle_visibility,
    .active = active,
    .tag = set_tag,
    .layout = set_layout,
    .title = title,
    .appid = appid,
    .layout_symbol = set_layout_symbol,
    .frame = dwl_frame,
};

static void handle_global(void *data, struct wl_registry *registry, uint32_t name,
                          const char *interface, uint32_t version) {
  if (std::strcmp(interface, zdwl_ipc_manager_v2_interface.name) == 0) {
    static_cast<Title *>(data)->status_manager_ = static_cast<struct zdwl_ipc_manager_v2 *>(
        (zdwl_ipc_manager_v2 *)wl_registry_bind(registry, name, &zdwl_ipc_manager_v2_interface, 1));
  }
  if (std::strcmp(interface, wl_seat_interface.name) == 0) {
    version = std::min<uint32_t>(version, 1);
    static_cast<Title *>(data)->seat_ = static_cast<struct wl_seat *>(
        wl_registry_bind(registry, name, &wl_seat_interface, version));
  }
}

static void handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
  /* Ignore event */
}

static const wl_registry_listener registry_listener_impl = {.global = handle_global,
                                                            .global_remove = handle_global_remove};

Title::Title(const std::string &id, const waybar::Bar &bar, const Json::Value &config)
    : waybar::ALabel(config, "title", id, "", 0, true),
      status_manager_(nullptr),
      seat_(nullptr),
      bar_(bar) {
  struct wl_display *display = Client::inst()->wl_display;
  struct wl_registry *registry = wl_display_get_registry(display);
  wl_registry_add_listener(registry, &registry_listener_impl, this);
  wl_display_roundtrip(display);

  if (!status_manager_) {
    spdlog::error("dwl_status_manager_v2 not advertised");
    return;
  }

  if (!seat_) {
    spdlog::error("wl_seat not advertised");
  }

  update();

  struct wl_output *output = gdk_wayland_monitor_get_wl_output(bar_.output->monitor->gobj());
  output_status_ = zdwl_ipc_manager_v2_get_output(status_manager_, output);
  zdwl_ipc_output_v2_add_listener(output_status_, &output_status_listener_impl, this);

  //zdwl_ipc_manager_v2_destroy(status_manager_);
  //status_manager_ = nullptr;
}

Title::~Title() {
  if (status_manager_) {
    zdwl_ipc_manager_v2_destroy(status_manager_);
  }
}

auto Title::update() -> void {
  std::string format = "{title}";
  if (config_["format"].isString())
    format = config_["format"].asString();

  label_.show();
  label_.set_markup(fmt::format(fmt::runtime(format),
                                fmt::arg("title", active_title),
                                fmt::arg("tag", active_tag),
                                fmt::arg("layout", active_layout)));
  ALabel::update();
}

} /* namespace waybar::modules::dwl */
