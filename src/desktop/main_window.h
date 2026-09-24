#pragma once
#include <FL/Fl_Double_Window.H>
#include <simpleble/SimpleBLE.h>
#include <mutex>
#include <deque>
#include <chrono>
#include <mavlink/common/mavlink.h>
#include <desktop/widgets/attitude_widget.h>
struct Fl_Box;
struct Fl_Menu_Bar;
struct ble_discovery_menu_t;
struct attitude_widget_t;

struct main_window_t : public Fl_Double_Window
{
    main_window_t(int w,int h,const char* title);
    void update_state(const flight_state_t& state);

private:
    void create_telemetry_panel(int w);
    Fl_Box* section_label(int x,int y,int w,const char* text);
    Fl_Box* value_label(int x,int y,int w);
    Fl_Box* telemetry_label(int x,int y,int w);
    void update_telemetry();
    void connect_device(SimpleBLE::Peripheral& device);
    static void menu_callback(Fl_Widget* widget,void* userdata);
    static void poll_mavlink_queue(void* userdata);
    void handle_mavlink_msg(const mavlink_message_t& msg);
    void disconnect();
private:
    Fl_Menu_Bar* menu_bar = nullptr;
    Fl_Group* panel_group = nullptr;
    Fl_Box* roll_label = nullptr;
    Fl_Box* pitch_label = nullptr;
    Fl_Box* yaw_label = nullptr;
    Fl_Box* accel_label = nullptr;
    Fl_Box* gyro_label = nullptr;
    Fl_Box* connection_label = nullptr;
    ble_discovery_menu_t* ble_menu = nullptr;
    attitude_widget_t* attitude_widget = nullptr;
    SimpleBLE::Peripheral ble_connection{};
    flight_state_t fstate{};
    std::mutex mavlink_queue_mutex;
    std::deque<mavlink_message_t> mavlink_queue;
    mavlink_message_t mavlink_msg{};
    mavlink_status_t  mavlink_status{};
};
