#pragma once
#include <FL/Fl_Gl_Window.H>
#include <chrono>

struct flight_state_t
{
    float yaw   = 0.0f;
    float roll  = 0.0f;
    float pitch = 0.0f;

    float ax = 0.0f;
    float ay = 0.0f;
    float az = 0.0f;

    float gx = 0.0f;
    float gy = 0.0f;
    float gz = 0.0f;
};
struct attitude_widget_t : public Fl_Gl_Window
{
    attitude_widget_t(
        int x,
        int y,
        int w,
        int h,
        const char* label = nullptr);
    void update_state(const flight_state_t& state);
protected:
    void draw() override;
private:
    static void animation_timer(void* userdata);
    void update_smoothing();
    flight_state_t target_state{};
    flight_state_t display_state{};
    bool initialized = false;

    std::chrono::steady_clock::time_point last_frame_time;
};
