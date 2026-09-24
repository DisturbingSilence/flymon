#include <FL/Fl.H>
#include <desktop/main_window.h>

int main(int argc, char** argv)
{
    main_window_t window(1280,800,"Flight Monitor");

    return Fl::run();
}
