#include <desktop/widgets/attitude_widget.h>

#include <FL/gl.h>
#include <FL/Fl.H>

#include <algorithm>
#include <cmath>
#include <chrono>

namespace
{
constexpr float SMOOTHING_SPEED = 2.0f;
float smooth(float current,float target,float dt)
{
    const float alpha = 1.0f - std::exp(-SMOOTHING_SPEED * dt);
    return current + (target - current) * alpha;
}
float smooth_angle(float current,float target,float dt)
{
    float difference = std::fmod(target - current + 180.0f,360.0f);
    if (difference < 0.0f) difference += 360.0f;
    difference -= 180.0f;
    const float alpha = 1.0f - std::exp(-SMOOTHING_SPEED * dt);
    return current + difference * alpha;
}
}
attitude_widget_t::attitude_widget_t(
    int x,int y,
    int w,int h,
    const char* label) : Fl_Gl_Window(x, y, w, h, label)
{
    mode(FL_RGB | FL_DEPTH | FL_STENCIL | FL_DOUBLE);
    last_frame_time = std::chrono::steady_clock::now();
    Fl::add_timeout(1.0 / 60.0,animation_timer,this);
}
void attitude_widget_t::animation_timer(void* userdata)
{
    auto* widget = static_cast<attitude_widget_t*>(userdata);
    widget->redraw();
    Fl::repeat_timeout(1.0 / 60.0,animation_timer,userdata);
}
void attitude_widget_t::update_state(const flight_state_t& state)
{
    target_state = state;
    if (!initialized)
    {
        display_state = state;
        initialized = true;
    }
}
void attitude_widget_t::update_smoothing()
{
    const auto now = std::chrono::steady_clock::now();

    float dt = std::chrono::duration<float>(now - last_frame_time).count();
    last_frame_time = now;
    dt = std::clamp(dt,0.0f,0.05f);
    display_state.roll = smooth_angle(display_state.roll,target_state.roll,dt);
    display_state.pitch = smooth(display_state.pitch,target_state.pitch,dt);
    display_state.yaw = smooth_angle(display_state.yaw,target_state.yaw,dt);
    display_state.gx = smooth(display_state.gx,target_state.gx,dt);
    display_state.gy = smooth(display_state.gy,target_state.gy,dt);
    display_state.gz = smooth(display_state.gz,target_state.gz,dt);
}
void attitude_widget_t::draw()
{
    update_smoothing();
    const float cx = w() * 0.5f;
    const float cy = h() * 0.5f;
    const float r = std::min(w(), h()) * 0.43f;

    glViewport(0,0,w(), h());
    glClearColor(0.025f,0.03f,0.04f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,w(),0,h(),-1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS,1,0xFF);
    glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 128; ++i)
    {
        const float angle = i * 2.0f * M_PI / 128.0f;
        glVertex2f(cx + std::cos(angle) * r,cy + std::sin(angle) * r);
    }
    glEnd();


    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glStencilFunc(GL_EQUAL,1,0xFF);
    glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
    glPushMatrix();

    glTranslatef(cx,cy,0);
    glRotatef(-display_state.roll,0,0,1);
    glTranslatef(0,-display_state.pitch * r / 45.0f,0);
    glColor3f(0.035f,0.16f,0.25f);

    glBegin(GL_QUADS);
    glVertex2f(-r * 2,  0);
    glVertex2f( r * 2,  0);
    glVertex2f( r * 2,  r * 2);
    glVertex2f(-r * 2,  r * 2);

    glColor3f(0.16f,0.095f,0.055f);
    glVertex2f(-r * 2, -r * 2);
    glVertex2f( r * 2, -r * 2);
    glVertex2f( r * 2,  0);
    glVertex2f(-r * 2,  0);
    glEnd();

    glColor3f(0.72f,0.78f,0.82f);
    glBegin(GL_LINES);
    for (int p = -30; p <= 30; p += 10)
    {
        if (p == 0) continue;

        const float y = p * r / 45.0f;
        const float len = (p % 20 == 0) ? r * 0.16f : r * 0.10f;
        glVertex2f(-len, y);
        glVertex2f( len, y);
    }


    glColor3f(0.9f,0.92f,0.94f);
    glVertex2f(-r * 2,0);
    glVertex2f(r * 2,0);
    glEnd();
    glPopMatrix();

    glBegin(GL_LINES);
    glColor3f(0.65f,0.70f,0.75f);
    for (int deg = -60;deg <= 60;deg += 15)
    {
        const float angle =(90.0f - deg) * M_PI / 180.0f;
        const float r1 = r * 0.88f;
        const float r2 = (deg % 30 == 0) ? r * 0.94f : r * 0.91f;
        glVertex2f(
            cx + std::cos(angle) * r1,
            cy + std::sin(angle) * r1);

        glVertex2f(
            cx + std::cos(angle) * r2,
            cy + std::sin(angle) * r2);
    }
    glEnd();

    glColor3f(1.0f,0.72f,0.08f);
    glBegin(GL_LINES);
    glVertex2f(cx - r * 0.23f,cy);
    glVertex2f(cx - r * 0.06f,cy);
    glVertex2f(cx + r * 0.06f,cy);
    glVertex2f(cx + r * 0.23f,cy);
    glVertex2f(cx,cy);
    glVertex2f(cx,cy - r * 0.07f);
    glEnd();


    const float angle = (90.0f - display_state.roll) * M_PI / 180.0f;
    glColor3f(1.0f,0.72f,0.08f);
    glBegin(GL_TRIANGLES);
    glVertex2f(
        cx + std::cos(angle) * r * 0.96f,
        cy + std::sin(angle) * r * 0.96f);
    glVertex2f(
        cx + std::cos(angle + 0.035f) * r * 0.88f,
        cy + std::sin(angle + 0.035f) * r * 0.88f);
    glVertex2f(
        cx + std::cos(angle - 0.035f) * r * 0.88f,
        cy + std::sin(angle - 0.035f) * r * 0.88f);
    glEnd();

    glDisable(GL_STENCIL_TEST);
    glColor3f(0.28f,0.33f,0.38f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 128; ++i)
    {
        const float angle = i * 2.0f * M_PI / 128.0f;
        glVertex2f(cx + std::cos(angle) * r,cy + std::sin(angle) * r);
    }
    glEnd();
    glEnable(GL_DEPTH_TEST);
}
