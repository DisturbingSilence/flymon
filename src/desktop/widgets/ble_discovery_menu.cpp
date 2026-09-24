#include <desktop/widgets/ble_discovery_menu.h>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/fl_ask.H>
#include <format>


ble_discovery_menu_t::ble_discovery_menu_t(std::function<void(SimpleBLE::Peripheral&)> on_connect) :
    Fl_Double_Window(520,400,"BLE Device Discovery"),on_connect_clbck(std::move(on_connect))
{
    color(fl_rgb_color(17, 23, 30));

    begin();
    title_box = new Fl_Box(20,15,300,30,"BLE Devices");
    title_box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    title_box->labelsize(18);
    title_box->labelcolor(FL_WHITE);

    scan_button = new Fl_Button(380,15,120,32,"Scan");
    scan_button->callback(
    [](Fl_Widget*,void* userdata)
    {
        auto* window = static_cast<ble_discovery_menu_t*>(userdata);
        window->scan();
    }, this);

    devices = new Fl_Hold_Browser(20,65,480,230);
    devices->textsize(14);
    status_box = new Fl_Box(20,305,300,25,"Ready");
    status_box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    status_box->labelcolor(fl_rgb_color(130, 145, 160));
    cancel_button = new Fl_Button(290,350,100,32,"Cancel");
    cancel_button->callback(
    [](Fl_Widget*,void* userdata)
    {
        auto* window = static_cast<ble_discovery_menu_t*>(userdata);
        window->hide();
    },this);
    connect_button = new Fl_Button(400,350,100,32,"Connect");
    connect_button->callback(
    [](Fl_Widget*,void* userdata)
    {
        auto* window = static_cast<ble_discovery_menu_t*>(userdata);
        uint32_t index = window->devices->value();
        if (index <= 0)
        {
            window->status_box->copy_label("Select a device first");
            return;
        }
        if (index > window->discovered_devices.size()) return;
        SimpleBLE::Peripheral& device = window->discovered_devices[index - 1];
        if (window->on_connect_clbck) window->on_connect_clbck(device);
        window->hide();
    },this);
    end();
    resizable(devices);
}
void ble_discovery_menu_t::scan()
{
    if (!SimpleBLE::Adapter::bluetooth_enabled())
    {
        fl_alert("Bluetooth is not enabled or permission is missing.");
        return;
    }
    auto adapters = SimpleBLE::Adapter::get_adapters();
    if (adapters.empty())
    {
        fl_alert("No bluetooth adapters found");
        return;
    }
    auto adapter = adapters.front();
    status_box->copy_label("Scanning...");
    devices->clear();
    adapter.scan_for(5000);
    discovered_devices = adapter.scan_get_results();
    std::string line;
    line.reserve(256);
    for (auto& device : discovered_devices)
    {
        auto id = device.identifier();
        auto addr = device.address();
        line = std::format("{:<20s} {:<20s}",id.c_str(),addr.c_str());
        devices->add(line.c_str());
    }
    line = std::format("{} device(s) found",discovered_devices.size());
    status_box->copy_label(line.c_str());
}
