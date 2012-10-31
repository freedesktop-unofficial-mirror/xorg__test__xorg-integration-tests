#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdexcept>
#include <xorg/gtest/xorg-gtest.h>

#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>

#include "xit-server-input-test.h"

int XITServerInputTest::RegisterXI2(int major, int minor)
{
    int event_start;
    int error_start;

    if (!XQueryExtension(Display(), "XInputExtension", &xi2_opcode,
                         &event_start, &error_start))
        ADD_FAILURE() << "XQueryExtension returned FALSE";
    else {
        xi_event_base = event_start;
        xi_error_base = error_start;
    }

    int major_return = major;
    int minor_return = minor;
    if (XIQueryVersion(Display(), &major_return, &minor_return) != Success)
        ADD_FAILURE() << "XIQueryVersion failed";
    if (major_return != major)
       ADD_FAILURE() << "XIQueryVersion returned invalid major";

    return minor_return;
}

void XITServerInputTest::StartServer() {
    XITServerTest::StartServer();
    RegisterXI2();
}
