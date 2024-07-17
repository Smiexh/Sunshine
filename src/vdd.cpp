#include "platform/common.h"
#include "config.h"
#include "confighttp.h"
#include "globals.h"
#include "network.h"

#include "parsec-vdd.h"

using namespace std::literals;
using namespace parsec_vdd;

namespace vdd {
  constexpr auto REFRESH_INTERVAL = 1s;
  class deinit_t: public platf::deinit_t {
  public:
    deinit_t() {
      if (net::from_enum_string(config::nvhttp.origin_web_ui_allowed) > net::LAN) {
      }

      // Start the mapping thread
      vdd_thread = std::thread { &deinit_t::vdd_thread_proc, this };
    }

    ~deinit_t() {
      vdd_thread.join();
    }

    void
    vdd_thread_proc() {
      auto shutdown_event = mail::man->event<bool>(mail::shutdown);
      std::vector<int> displays;
      bool running = false;
      DeviceStatus status = QueryDeviceStatus(&VDD_CLASS_GUID, VDD_HARDWARE_ID);

      if (status != DEVICE_OK) {
        printf("Parsec VDD device is not OK, got status %d.\n", status);
        shutdown_event->raise(true);
      }

      // Obtain device handle.
      HANDLE vdd = OpenDeviceHandle(&VDD_ADAPTER_GUID);
      std::thread updater([&running, vdd] {
        BOOST_LOG(info) << "vdd thread enter!!!!!!!!!!!!!!!!!!!!!!!! "sv;
        while (running) {
          VddUpdate(vdd);
          std::this_thread::sleep_for(100ms);
        }
        BOOST_LOG(info) << "vdd thread exit!!!!!!!!!!!!!!!!!!!!!!!! "sv;
      });

      if (vdd == NULL || vdd == INVALID_HANDLE_VALUE) {        
        shutdown_event->raise(true);
      }
      else {
        updater.detach();
        running = true;
        if (displays.size() < VDD_MAX_DISPLAYS) {
          int index = VddAddDisplay(vdd);
          displays.push_back(index);
        }
      }

      while (!shutdown_event->view(REFRESH_INTERVAL));

      BOOST_LOG(info) << "vdd exit!!!!!!!!!!!!!!!!!!!!!!!! "sv;

      // Remove all before exiting.
      for (int index : displays) {
        VddRemoveDisplay(vdd, index);
      }

      if (updater.joinable()) {
        updater.join();
      }

      // Close the device handle.
      CloseDeviceHandle(vdd);
    }

    std::thread vdd_thread;
  };

  std::unique_ptr<platf::deinit_t>
  start() {
    return std::make_unique<deinit_t>();
  }
}  // namespace vdd
