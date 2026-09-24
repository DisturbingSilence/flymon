#include <desktop/main_window.h>
#include <desktop/widgets/ble_discovery_menu.h>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>
#include <format>
#include <cstring>
#include <config.h>
#include <FL/fl_ask.H>

void main_window_t::handle_mavlink_msg(const mavlink_message_t& msg)
{
    switch(msg.msgid)
    {
        case MAVLINK_MSG_ID_ATTITUDE:
        {
            mavlink_attitude_t attitude;
            mavlink_msg_attitude_decode(&msg, &attitude);

            flight_state_t state = fstate;

            // MAVLink ATTITUDE angles are radians.
            state.roll =
                attitude.roll * 180.0f / M_PI;

            state.pitch =
                attitude.pitch * 180.0f / M_PI;

            state.yaw =
                attitude.yaw * 180.0f / M_PI;

            state.gx =
                attitude.rollspeed * 180.0f / M_PI;

            state.gy =
                attitude.pitchspeed * 180.0f / M_PI;

            state.gz =
                attitude.yawspeed * 180.0f / M_PI;

            update_state(state);

            break;
        }
    };
}
void main_window_t::poll_mavlink_queue(void* userdata)
{
    auto* window = static_cast<main_window_t*>(userdata);
    std::deque<mavlink_message_t> batch;
    {
        std::lock_guard<std::mutex> lock(window->mavlink_queue_mutex);
        std::swap(batch,window->mavlink_queue);
    }
    for(auto& msg : batch) window->handle_mavlink_msg(msg);
    Fl::repeat_timeout(0.03,poll_mavlink_queue,userdata);
}
void main_window_t::update_state(const flight_state_t& state)
{
    fstate = state;
    update_telemetry();
    if(attitude_widget) attitude_widget->update_state(state);
}
Fl_Box* main_window_t::value_label(int x,int y,int w)
{
    auto* box = new Fl_Box(x,y,w,25);
    box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box->labelsize(16);
    box->labelcolor(fl_rgb_color(215,222,229));
    return box;
}
Fl_Box* main_window_t::telemetry_label(int x,int y,int w)
{
    auto* box = new Fl_Box(x,y,w,60);
    box->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    box->labelsize(14);
    box->labelcolor(fl_rgb_color(190,200,210));
    return box;
}
Fl_Box* main_window_t::section_label(int x,int y,int w,const char* text)
{
    auto* box = new Fl_Box(x,y,w,25,text);
    box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box->labelsize(12);
    box->labelcolor(fl_rgb_color(90,165,230));
    return box;
}

void main_window_t::connect_device(SimpleBLE::Peripheral& device)
{
    try
    {
        std::cout << std::format("Connecting to {}({})\n",device.identifier(),device.address());
        device.connect();
        std::cout << "Connected\n";
        const auto str = device.identifier();
        auto success_status = std::format("  ●  {}  ({})",str,device.address());
        connection_label->copy_label(success_status.c_str());
        connection_label->labelcolor(fl_rgb_color(230,190,70));
        auto services = device.services();
        bool is_required_service_present = false;
        for(auto service : services)
        {
            if(service.uuid() != std::string(JDY23_SERVICE_UUID)) continue;
            for(auto characteristic : service.characteristics())
            {
                if(characteristic.uuid() == std::string(JDY23_NOTIFY_CHAR_UUID))
                {
                    is_required_service_present = true;
                    break;
                }
            }
            if(is_required_service_present) break;
        }
        if(!is_required_service_present)
        {
            fl_alert("Target device is missing required service");
            device.disconnect();
            connection_label->copy_label("●  DISCONNECTED");
            connection_label->labelcolor(fl_rgb_color(120,135,150));
            return;
        }
        ble_connection = device;
        mavlink_status = {};
        mavlink_msg = {};

        ble_connection.notify(
            JDY23_SERVICE_UUID,
            JDY23_NOTIFY_CHAR_UUID,
            [this](SimpleBLE::ByteArray payload)
            {
                for(unsigned char c : payload)
                {
                    if(mavlink_parse_char(MAVLINK_COMM_0,c,&mavlink_msg,&mavlink_status))
                    {
                        std::lock_guard<std::mutex> lock(mavlink_queue_mutex);
                        mavlink_queue.push_back(mavlink_msg);
                    }
                }
            });

        connection_label->copy_label("●  AUTHENTICATING...");
        connection_label->labelcolor(fl_rgb_color(230,190,70));
    }
    catch(const SimpleBLE::Exception::OperationFailed& e)
    {
        std::cerr << std::format("BLE connection failed:{}\n",e.what());
        connection_label->copy_label("●  CONNECTION FAILED");
        connection_label->labelcolor(fl_rgb_color(220,80,80));
        return;
    }
    catch(const std::exception& e)
    {
        std::cerr << std::format("BLE exception :{}\n",e.what());
        connection_label->copy_label("●  BLE ERROR");
        connection_label->labelcolor(fl_rgb_color(220,80,80));
        return;
    }
}

void main_window_t::disconnect()
{
    {
        std::lock_guard<std::mutex> lock(mavlink_queue_mutex);
        mavlink_queue.clear();
    }
    try
    {
        if(ble_connection.is_connected())
        {
            ble_connection.disconnect();
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << std::format("BLE disconnect failed: {}\n",e.what());
    }

    connection_label->copy_label("●  DISCONNECTED");
    connection_label->labelcolor(fl_rgb_color(120,135,150));
    flight_state_t state = {};
    update_state(state);
}

void main_window_t::menu_callback(Fl_Widget* widget,void* userdata)
{
    auto* window = static_cast<main_window_t*>(userdata);
    auto* item = static_cast<Fl_Menu_Bar*>(widget)->mvalue();

    if(!item) return;

    auto label = std::string(item->label());
    if(label.empty()) return;

    if(label == "Quit")
    {
        window->disconnect();
        window->hide();
        return;
    }

    if(label == "Discover BLE Devices")
    {
        window->ble_menu->show();
        return;
    }

    if(label == "Disconnect")
    {
        window->disconnect();
        return;
    }
}

void main_window_t::update_telemetry()
{
    std::string buffer;
    buffer.reserve(256);

    buffer = std::format("ROLL\t\t {:+8.2f}°",fstate.roll);
    roll_label->copy_label(buffer.c_str());

    buffer = std::format("YAW\t\t {:+8.2f}°",fstate.yaw);
    yaw_label->copy_label(buffer.c_str());

    buffer = std::format("PITCH\t\t{:+8.2f}°",fstate.pitch);
    pitch_label->copy_label(buffer.c_str());

    buffer = std::format(
        "X  {:+7.2f} g\n"
        "Y  {:+7.2f} g\n"
        "Z  {:+7.2f} g",
        fstate.ax,
        fstate.ay,
        fstate.az);

    accel_label->copy_label(buffer.c_str());

    buffer = std::format(
        "X  {:+7.2f} °/s\n"
        "Y  {:+7.2f} °/s\n"
        "Z  {:+7.2f} °/s",
        fstate.gx,
        fstate.gy,
        fstate.gz);
    gyro_label->copy_label(buffer.c_str());
}

void main_window_t::create_telemetry_panel(int w)
{
    constexpr int margin = 15;
    int y = 45;

    section_label(margin,y,w - margin * 2,"ATTITUDE");
    y += 40;

    yaw_label = value_label(margin,y,w - margin * 2);
    y += 30;

    roll_label = value_label(margin,y,w - margin * 2);
    y += 30;

    pitch_label = value_label(margin,y,w - margin * 2);
    y += 45;

    section_label(margin,y,w - margin * 2,"ACCELEROMETER");
    y += 40;

    accel_label = telemetry_label(margin,y,w - margin * 2);
    y += 75;

    section_label(margin,y,w - margin * 2,"GYROSCOPE");
    y += 40;

    gyro_label = telemetry_label(margin,y,w - margin * 2);
    y += 95;

    section_label(margin,y,w - margin * 2,"CONNECTION");
    y += 40;

    connection_label = new Fl_Box(margin,y,w - margin * 2,30);
    connection_label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    connection_label->labelcolor(fl_rgb_color(120,135,150));
    connection_label->copy_label("●  DISCONNECTED");
}

main_window_t::main_window_t(int w,int h,const char* title) : Fl_Double_Window(w,h,title)
{
    color(fl_rgb_color(11,15,20));

    begin();

    callback([](Fl_Widget* widget,void* userdata)
    {
        auto* win = static_cast<main_window_t*>(userdata);
        win->disconnect();
        win->hide();
    },this);

    menu_bar = new Fl_Menu_Bar(0,0,w,28);

    menu_bar->add("Quit","^q",menu_callback,this);
    menu_bar->add("Connection/Discover BLE Devices","^d",menu_callback,this);
    menu_bar->add("Connection/Disconnect",nullptr,menu_callback,this);

    const int menu_h = 28;
    const int panel_w = 285;
    const int telemetry_w = 285;

    attitude_widget = new attitude_widget_t(
        telemetry_w,
        menu_h,
        w - telemetry_w,
        h - menu_h);

    panel_group = new Fl_Group(
        0,
        menu_h,
        telemetry_w,
        h - menu_h);

    panel_group->color(fl_rgb_color(17,23,30));
    create_telemetry_panel(panel_w);

    panel_group->end();
    end();

    ble_menu = new ble_discovery_menu_t(
        [this](SimpleBLE::Peripheral& device)
        {
            connect_device(device);
        });

    flight_state_t fs = {};
    update_state(fs);

    show();

    Fl::add_timeout(0.03,poll_mavlink_queue,this);
}
