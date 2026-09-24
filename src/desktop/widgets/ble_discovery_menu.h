#pragma once
#include <FL/Fl_Double_Window.H>
#include <simpleble/SimpleBLE.h>
#include <functional>

struct Fl_Box;
struct Fl_Button;
struct Fl_Hold_Browser;
struct ble_discovery_menu_t : public Fl_Double_Window
{
    ble_discovery_menu_t(std::function<void(SimpleBLE::Peripheral&)> on_connect);
    void scan();
private:
    Fl_Box* title_box = nullptr;
    Fl_Hold_Browser* devices = nullptr;
    Fl_Button* scan_button = nullptr;
    Fl_Button* cancel_button = nullptr;
    Fl_Button* connect_button = nullptr;
    Fl_Box* status_box = nullptr;
    std::vector<SimpleBLE::Peripheral> discovered_devices;
    std::function<void(SimpleBLE::Peripheral&)> on_connect_clbck;
};
