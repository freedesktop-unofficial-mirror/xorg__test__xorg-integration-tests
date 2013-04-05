/*
 * Copyright © 2012-2013 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <pixman.h>

#include <xorg/gtest/xorg-gtest.h>

#include <X11/Xatom.h>
#include <X11/extensions/XInput2.h>

#include "xit-server-input-test.h"
#include "xit-event.h"
#include "device-interface.h"
#include "helpers.h"

using namespace xorg::testing;

class PointerMotionTest : public XITServerInputTest,
                          public DeviceInterface {
public:
    /**
     * Initializes a standard mouse device.
     */
    virtual void SetUp() {
        SetDevice("mice/PIXART-USB-OPTICAL-MOUSE-HWHEEL.desc");
        XITServerInputTest::SetUp();
    }

    /**
     * Sets up an xorg.conf for a single evdev CoreKeyboard device based on
     * the evemu device. The input from GetParam() is used as XkbLayout.
     */
    virtual void SetUpConfigAndLog() {
        config.AddDefaultScreenWithDriver();
        config.AddInputSection("evdev", "--device--",
                               "Option \"CorePointer\" \"on\"\n"
                               "Option \"GrabDevice\" \"on\"\n"
                               "Option \"Device\" \"" + dev->GetDeviceNode() + "\"");
        /* add default keyboard device to avoid server adding our device again */
        config.AddInputSection("kbd", "kbd-device",
                               "Option \"CoreKeyboard\" \"on\"\n");
        config.WriteConfig();
    }
};

enum device_type {
    MOUSE, TABLET, TOUCHPAD
};

class PointerAccelerationTest : public XITServerInputTest,
                                public DeviceInterface,
                                public ::testing::WithParamInterface<enum device_type>
{
public:
    virtual void SetUpMouse() {
        SetDevice("mice/PIXART-USB-OPTICAL-MOUSE-HWHEEL.desc");

        driver = "evdev";
        config_section = "Option \"CorePointer\" \"on\"\n"
                         "Option \"GrabDevice\" \"on\"\n"
                         "Option \"Device\" \"" + dev->GetDeviceNode() + "\"";
    }

    virtual void SetUpTablet() {
        SetDevice("tablets/Wacom-Intuos4-6x9.desc");
        driver = "wacom";
        config_section =  "Option \"device\" \"" + dev->GetDeviceNode() + "\"\n"
                          "Option \"GrabDevice\" \"on\"\n"
                          "Option \"Type\" \"stylus\"\n"
                          "Option \"CorePointer\" \"on\"\n"
                          "Option \"Mode\" \"relative\"\n";
    }

    virtual void SetUpTouchpad() {
        SetDevice("touchpads/SynPS2-Synaptics-TouchPad.desc");
        driver = "synaptics";
        config_section =  "Option \"device\" \"" + dev->GetDeviceNode() + "\"\n"
                          "Option \"GrabDevice\" \"on\"\n"
                          "Option \"CorePointer\" \"on\"\n";
    }

    virtual void SetUp() {

        switch(GetParam()) {
            case MOUSE: SetUpMouse(); break;
            case TABLET: SetUpTablet(); break;
            case TOUCHPAD: SetUpTouchpad(); break;
        }

        XITServerInputTest::SetUp();
    }

    virtual void SetUpConfigAndLog() {
        config.AddDefaultScreenWithDriver();
        config.SetAutoAddDevices(false);
        config.AddInputSection(driver, "device", config_section);
        config.WriteConfig();
    }

    virtual void InitPosition(enum device_type type) {
        switch(type) {
            case MOUSE: break;
            case TABLET:
                    dev->PlayOne(EV_ABS, ABS_X, 1000);
                    dev->PlayOne(EV_ABS, ABS_Y, 1000);
                    dev->PlayOne(EV_ABS, ABS_DISTANCE, 0);
                    dev->PlayOne(EV_KEY, BTN_TOOL_PEN, 1, true);
                    break;
            case TOUCHPAD:
                    dev->PlayOne(EV_ABS, ABS_X, 1500);
                    dev->PlayOne(EV_ABS, ABS_Y, 2000);
                    dev->PlayOne(EV_ABS, ABS_PRESSURE, 34);
                    dev->PlayOne(EV_KEY, BTN_TOOL_FINGER, 1, true);
                    break;
        }
    }

    virtual void Move(enum device_type type) {
        int x, y;
        int minx, maxx, miny, maxy;

        switch(type) {
            case MOUSE:
                for (int i = 0; i < 20; i++)
                    dev->PlayOne(EV_REL, REL_X, 1, true);

                for (int i = 0; i < 20; i++)
                    dev->PlayOne(EV_REL, REL_Y, 1, true);
                break;
            case TABLET:
                dev->GetAbsData(ABS_X, &minx, &maxx);
                dev->GetAbsData(ABS_Y, &miny, &maxy);
                x = 1000;
                y = 1000;

                while (x < 10000)
                {
                    x += (maxx - minx)/10.0;
                    y += (maxy - miny)/10.0;
                    dev->PlayOne(EV_ABS, ABS_X, x);
                    dev->PlayOne(EV_ABS, ABS_Y, y);

                    dev->PlayOne(EV_ABS, ABS_DISTANCE, 0);
                    dev->PlayOne(EV_KEY, BTN_TOOL_PEN, 1, true);
                }

                dev->PlayOne(EV_KEY, BTN_TOOL_PEN, 0, true);
                break;
            case TOUCHPAD:
                dev->GetAbsData(ABS_X, &minx, &maxx);
                dev->GetAbsData(ABS_Y, &miny, &maxy);
                x = 1500;
                y = 2000;

                while (x < 3000)
                {
                    x += (maxx - minx)/10.0;
                    y += (maxy - miny)/10.0;
                    dev->PlayOne(EV_ABS, ABS_X, x);
                    dev->PlayOne(EV_ABS, ABS_Y, y);

                    dev->PlayOne(EV_ABS, ABS_PRESSURE, 40);
                    dev->PlayOne(EV_ABS, ABS_TOOL_WIDTH, 10);
                    dev->PlayOne(EV_KEY, BTN_TOUCH, 1, true);
                    dev->PlayOne(EV_KEY, BTN_TOOL_FINGER, 1, true);
                }

                dev->PlayOne(EV_KEY, BTN_TOOL_FINGER, 0, true);
                break;
        }
    }

private:
    std::string config_section;
    std::string driver;
};

TEST_P(PointerAccelerationTest, IdenticalMovementVerticalHorizontal)
{
    enum device_type device_type = GetParam();
    std::string devtype;
    switch(device_type) {
        case MOUSE: devtype = "mouse"; break;
        case TABLET: devtype = "tablet"; break;
        case TOUCHPAD: devtype = "touchpad"; break;

    }

    XORG_TESTCASE("Start server with devices in mode relative\n"
                  "Move the pointer diagonally down\n"
                  "Verify x and y relative movement are the same\n"
                  "https://bugs.freedesktop.org/show_bug.cgi?id=31636");
    SCOPED_TRACE("Device type is " + devtype);

    ::Display *dpy = Display();

    ASSERT_EQ(FindInputDeviceByName(dpy, "device"), 1);

    InitPosition(device_type);

    double x1, y1;
    QueryPointerPosition(dpy, &x1, &y1);

    Move(device_type);

    double x2, y2;
    QueryPointerPosition(dpy, &x2, &y2);

    /* have we moved at all? */
    ASSERT_NE(x1, x2);
    ASSERT_NE(y1, y2);

    /* we moved diagonally, expect same accel in both directions */
    ASSERT_EQ(x2 - x1, y2 - y1);
}

INSTANTIATE_TEST_CASE_P(, PointerAccelerationTest, ::testing::Values(MOUSE, TABLET, TOUCHPAD));

class PointerButtonMotionMaskTest : public PointerMotionTest,
                                    public ::testing::WithParamInterface<int>
{
};

TEST_P(PointerButtonMotionMaskTest, ButtonXMotionMask)
{
    XORG_TESTCASE("Select for ButtonXMotionMask\n"
                  "Move pointer\n"
                  "Verify no events received\n"
                  "Press button and move\n"
                  "Verify events received\n");
    int button = GetParam();

    ::Display *dpy = Display();
    XSelectInput(dpy, DefaultRootWindow(dpy), Button1MotionMask << (button - 1));
    XSync(dpy, False);

    dev->PlayOne(EV_REL, REL_X, 10, true);
    XSync(dpy, False);
    ASSERT_EQ(XPending(dpy), 0);

    int devbutton;
    switch(button) {
        case 1: devbutton = BTN_LEFT; break;
        case 2: devbutton = BTN_MIDDLE; break;
        case 3: devbutton = BTN_RIGHT; break;
        default: FAIL();
    }

    dev->PlayOne(EV_KEY, devbutton, 1, true);
    dev->PlayOne(EV_REL, REL_X, 10, true);
    dev->PlayOne(EV_KEY, devbutton, 0, true);

    ASSERT_EVENT(XEvent, motion, dpy, MotionNotify);
    ASSERT_TRUE(motion->xmotion.state & (Button1Mask << (button -1)));

    XSync(dpy, False);
    ASSERT_EQ(XPending(dpy), 0);
}

INSTANTIATE_TEST_CASE_P(, PointerButtonMotionMaskTest, ::testing::Range(1, 4));

class PointerSubpixelTest : public PointerMotionTest {
    /**
     * Sets up an xorg.conf for a single evdev CoreKeyboard device based on
     * the evemu device. The input from GetParam() is used as XkbLayout.
     */
    virtual void SetUpConfigAndLog() {
        config.AddDefaultScreenWithDriver();
        config.AddInputSection("evdev", "--device--",
                               "Option \"CorePointer\" \"on\"\n"
                               "Option \"GrabDevice\" \"on\"\n"
                               "Option \"ConstantDeceleration\" \"20\"\n"
                               "Option \"Device\" \"" + dev->GetDeviceNode() + "\"");
        /* add default keyboard device to avoid server adding our device again */
        config.AddInputSection("kbd", "kbd-device",
                               "Option \"CoreKeyboard\" \"on\"\n");
        config.WriteConfig();
    }
};

TEST_F(PointerSubpixelTest, NoSubpixelCoreEvents)
{
    XORG_TESTCASE("Move pointer by less than a pixels\n"
                  "Ensure no core motion event is received\n"
                  "Ensure XI2 motion events are received\n");

    ::Display *dpy = Display();
    ::Display *dpy2 = XOpenDisplay(server.GetDisplayString().c_str());
    ASSERT_TRUE(dpy2);

    double x, y;
    QueryPointerPosition(dpy, &x, &y);

    XSelectInput(dpy, DefaultRootWindow(dpy), PointerMotionMask);
    XSync(dpy, False);

    dev->PlayOne(EV_REL, REL_X, 1, true);
    dev->PlayOne(EV_REL, REL_X, 1, true);
    dev->PlayOne(EV_REL, REL_X, 1, true);
    dev->PlayOne(EV_REL, REL_X, 1, true);

    ASSERT_FALSE(XServer::WaitForEvent(Display(), 500));

    XSelectInput(dpy, DefaultRootWindow(dpy), NoEventMask);
    XSync(dpy, False);

    XIEventMask mask;
    mask.deviceid = XIAllMasterDevices;
    mask.mask_len = XIMaskLen(XI_Motion);
    mask.mask = new unsigned char[mask.mask_len]();
    XISetMask(mask.mask, XI_Motion);

    XISelectEvents(dpy2, DefaultRootWindow(dpy), &mask, 1);
    XSync(dpy2, False);

    delete[] mask.mask;

    dev->PlayOne(EV_REL, REL_X, 1, true);
    dev->PlayOne(EV_REL, REL_X, 1, true);
    dev->PlayOne(EV_REL, REL_X, 1, true);
    dev->PlayOne(EV_REL, REL_X, 1, true);

    ASSERT_TRUE(XServer::WaitForEventOfType(dpy2, GenericEvent, xi2_opcode, XI_Motion));

    double new_x, new_y;
    QueryPointerPosition(dpy, &new_x, &new_y);
    ASSERT_EQ(x, new_x);
    ASSERT_EQ(y, new_y);

    XCloseDisplay(dpy2);
}

class PointerRelativeTransformationMatrixTest : public PointerMotionTest {
public:
    void SetDeviceMatrix(::Display *dpy, int deviceid, struct pixman_f_transform *m) {
        float matrix[9];

        for (int i = 0; i < 9; i++)
            matrix[i] = m->m[i/3][i%3];

        Atom prop = XInternAtom(dpy, "Coordinate Transformation Matrix", True);
        XIChangeProperty(dpy, deviceid, prop, XInternAtom(dpy, "FLOAT", True),
                         32, PropModeReplace,
                         reinterpret_cast<unsigned char*>(matrix), 9);
    }

    void DisablePtrAccel(::Display *dpy, int deviceid) {
        int data = -1;

        Atom prop = XInternAtom(dpy, "Device Accel Profile", True);
        XIChangeProperty(dpy, deviceid, prop, XA_INTEGER, 32,
                         PropModeReplace, reinterpret_cast<unsigned char*>(&data), 1);
    }

    void MoveAndCompare(::Display *dpy, int dx, int dy) {
        double x, y;

        QueryPointerPosition(dpy, &x, &y);
        dev->PlayOne(EV_REL, REL_X, dx);
        dev->PlayOne(EV_REL, REL_Y, dy, true);
        ASSERT_EVENT(XIDeviceEvent, motion, dpy, GenericEvent, xi2_opcode, XI_Motion);
        ASSERT_EQ(motion->root_x, x + dx);
        ASSERT_EQ(motion->root_y, y + dy);
    }
};


TEST_F(PointerRelativeTransformationMatrixTest, IgnoreTranslationComponent)
{
    XORG_TESTCASE("Apply a translation matrix to the device\n"
                  "Move the pointer.\n"
                  "Verify matrix does not affect movement\n");

    ::Display *dpy = Display();

    int deviceid;
    ASSERT_EQ(FindInputDeviceByName(dpy, "--device--", &deviceid), 1);

    XIEventMask mask;
    mask.deviceid = XIAllMasterDevices;
    mask.mask_len = XIMaskLen(XI_Motion);
    mask.mask = new unsigned char[mask.mask_len]();
    XISetMask(mask.mask, XI_Motion);

    XISelectEvents(dpy, DefaultRootWindow(dpy), &mask, 1);

    DisablePtrAccel(dpy, deviceid);

    struct pixman_f_transform m;
    pixman_f_transform_init_translate(&m, 10, 0);
    SetDeviceMatrix(dpy, deviceid, &m);

    MoveAndCompare(dpy, 10, 0);
    MoveAndCompare(dpy, 0, 10);
    MoveAndCompare(dpy, 5, 5);

    pixman_f_transform_init_translate(&m, 0, 10);
    SetDeviceMatrix(dpy, deviceid, &m);

    MoveAndCompare(dpy, 10, 0);
    MoveAndCompare(dpy, 0, 10);
    MoveAndCompare(dpy, 5, 5);
}

class PointerRelativeRotationMatrixTest : public PointerRelativeTransformationMatrixTest,
                                          public ::testing::WithParamInterface<int> {
};

TEST_P(PointerRelativeRotationMatrixTest, RotationTest)
{
    XORG_TESTCASE("Apply a coordinate transformation to the relative device\n"
                  "Move the pointer.\n"
                  "Verify movement against matrix\n");

    ::Display *dpy = Display();

    int deviceid;
    ASSERT_EQ(FindInputDeviceByName(dpy, "--device--", &deviceid), 1);

    XIEventMask mask;
    mask.deviceid = XIAllMasterDevices;
    mask.mask_len = XIMaskLen(XI_Motion);
    mask.mask = new unsigned char[mask.mask_len]();
    XISetMask(mask.mask, XI_Motion);

    XISelectEvents(dpy, DefaultRootWindow(dpy), &mask, 1);

    int angle = GetParam() * M_PI/180.0;

    struct pixman_f_transform m;
    pixman_f_transform_init_rotate(&m, cos(angle), sin(angle));

    SetDeviceMatrix(dpy, deviceid, &m);

    int coords[][2] = {
        {1, 0}, {-2, 0}, {3, 2}, {4, -7},
        {-3, 6}, {-5, -9}, {0, 3}, {0, -5},
        {0, 0}, /* null-terminated */
    };

    int i = 0;
    while(coords[i][0] && coords[i][1]) {
        double dx = coords[i][0], dy = coords[i][1];
        struct pixman_f_vector p;
        p.v[0] = dx;
        p.v[1] = dy;
        p.v[2] = 1;

        ASSERT_TRUE(pixman_f_transform_point(&m, &p));

        double x, y;
        QueryPointerPosition(dpy, &x, &y);

        /* Move pointer */
        dev->PlayOne(EV_REL, REL_X, dx);
        dev->PlayOne(EV_REL, REL_Y, dy, true);

        /* Compare to two decimal places */
        ASSERT_EVENT(XIDeviceEvent, motion, dpy, GenericEvent, xi2_opcode, XI_Motion);
        ASSERT_LT(fabs(motion->root_x - (x + p.v[0])), 0.001);
        ASSERT_LT(fabs(motion->root_y - (y + p.v[1])), 0.001);
        i++;
    }

}

INSTANTIATE_TEST_CASE_P(, PointerRelativeRotationMatrixTest, ::testing::Range(0, 360, 15));

class DeviceChangedTest : public XITServerInputTest {
public:
    virtual void SetUp() {
        mouse = std::auto_ptr<xorg::testing::evemu::Device>(
                new xorg::testing::evemu::Device(
                    RECORDINGS_DIR "/mice/PIXART-USB-OPTICAL-MOUSE.desc")
                );

        touchpad = std::auto_ptr<xorg::testing::evemu::Device>(
                new xorg::testing::evemu::Device(
                    RECORDINGS_DIR "/touchpads/SynPS2-Synaptics-TouchPad.desc")
                );
        xi2_major_minimum = 2;
        xi2_minor_minimum = 2;
        XITServerInputTest::SetUp();
    }

    virtual void SetUpConfigAndLog() {
        config.AddDefaultScreenWithDriver();
        config.AddInputSection("evdev", "--mouse--",
                               "Option \"CorePointer\" \"on\"\n"
                               "Option \"GrabDevice\" \"on\"\n"
                               "Option \"Device\" \"" + mouse->GetDeviceNode() + "\"");
        config.AddInputSection("synaptics", "--touchpad--",
                               "Option \"CorePointer\" \"on\"\n"
                               "Option \"GrabDevice\" \"on\"\n"
                               "Option \"Device\" \"" + touchpad->GetDeviceNode() + "\"");
        /* add default keyboard device to avoid server adding our device again */
        config.AddInputSection("kbd", "kbd-device",
                               "Option \"CoreKeyboard\" \"on\"\n");
        config.WriteConfig();
    }

    std::auto_ptr<xorg::testing::evemu::Device> mouse;
    std::auto_ptr<xorg::testing::evemu::Device> touchpad;
};


#ifdef HAVE_XI22
TEST_F(DeviceChangedTest, DeviceChangedEvent)
{
    XORG_TESTCASE("Create touchpad and mouse.\n"
                  "Scroll down (smooth scrolling) on touchpad\n"
                  "Move mouse\n"
                  "Move touchpad\n"
                  "DeviceChangedEvent from touchpad must have last smooth scroll valuators set\n"
                  "https://bugs.freedesktop.org/show_bug.cgi?id=62321");

    ::Display *dpy = Display();

    int ndevices;
    XIDeviceInfo *info = XIQueryDevice(dpy, XIAllDevices, &ndevices);
    ASSERT_EQ(ndevices, 7); /* VCP, VCK, xtest * 2, touchpad, mouse, kbd */

    int vscroll_axis = -1;
    int deviceid = -1;

    for (int i = 0; deviceid == -1 && i < ndevices; i++) {
        if (strcmp(info[i].name, "--touchpad--"))
            continue;

        deviceid = info[i].deviceid;
        XIDeviceInfo *di = &info[i];
        for (int j = 0; j < di->num_classes; j++) {
            if (di->classes[j]->type != XIScrollClass)
                continue;

            XIScrollClassInfo *scroll = reinterpret_cast<XIScrollClassInfo*>(di->classes[j]);
            if (scroll->scroll_type == XIScrollTypeVertical) {
                vscroll_axis = scroll->number;
                break;
            }
        }
    }
    XIFreeDeviceInfo(info);

    ASSERT_GT(vscroll_axis, -1);

    XIEventMask mask;
    mask.deviceid = XIAllMasterDevices;
    mask.mask_len = XIMaskLen(XI_Motion);
    mask.mask = new unsigned char[mask.mask_len]();
    XISetMask(mask.mask, XI_Motion);
    XISetMask(mask.mask, XI_DeviceChanged);
    XISelectEvents(dpy, DefaultRootWindow(dpy), &mask, 1);

    touchpad->Play(RECORDINGS_DIR "touchpads/SynPS2-Synaptics-TouchPad-two-finger-scroll-down.events");

    XSync(dpy, False);
    while(XPending(dpy) > 1) {
        XEvent ev;
        XNextEvent(dpy, &ev);
        XSync(dpy, False);
    }

    ASSERT_EVENT(XIDeviceEvent, motion, dpy, GenericEvent, xi2_opcode, XI_Motion);
    ASSERT_GT(motion->valuators.mask_len, 0);
    ASSERT_TRUE(XIMaskIsSet(motion->valuators.mask, vscroll_axis));

    double last_value;
    double *valuators = motion->valuators.values;

    for (int i = 0; i < vscroll_axis; i++) {
        if (XIMaskIsSet(motion->valuators.mask, i))
            valuators++;
    }

    last_value = *valuators;
    ASSERT_GT(last_value, 0);

    mouse->PlayOne(EV_REL, REL_X, 1, true);

    XSync(dpy, True); /* discard DCE from mouse */

    touchpad->Play(RECORDINGS_DIR "touchpads/SynPS2-Synaptics-TouchPad-two-finger-scroll-down.events");

    ASSERT_EVENT(XIDeviceChangedEvent, dce, dpy, GenericEvent, xi2_opcode, XI_DeviceChanged);
    ASSERT_EQ(dce->sourceid, deviceid);
    ASSERT_EQ(dce->reason, XISlaveSwitch);

    ASSERT_GT(dce->num_classes, 1);

    for (int i = 0; i < dce->num_classes; i++) {
        if (dce->classes[i]->type != XIValuatorClass)
            continue;

        XIValuatorClassInfo *v = reinterpret_cast<XIValuatorClassInfo*>(dce->classes[i]);
        if (v->number != vscroll_axis)
            continue;
        ASSERT_EQ(v->value, last_value);
        return;
    }

    FAIL() << "Failed to find vscroll axis in DCE";
}
#endif

enum MatrixType {
    IDENTITY,
    LEFT_HALF,
    RIGHT_HALF,
};

class PointerAbsoluteTransformationMatrixTest : public XITServerInputTest,
                                                public DeviceInterface,
                                                public ::testing::WithParamInterface<std::tr1::tuple<enum ::MatrixType, int> > {
public:
    virtual void SetUp() {
        SetDevice("tablets/Wacom-Intuos4-6x9.desc");
        XITServerInputTest::SetUp();
    }

    virtual std::string EvdevOptions(enum MatrixType which) {
        std::string matrix;
        switch(which) {
            case LEFT_HALF:
            matrix = "0.5 0 0 0 1 0 0 0 1";
                break;
            case RIGHT_HALF:
                matrix = "0.5 0 0.5 0 1 0 0 0 1";
                break;
            case IDENTITY:
                matrix = "1 0 0 0 1 0 0 0 1";
                break;
        }

        return "Option \"CorePointer\" \"on\"\n"
               "Option \"GrabDevice\" \"on\"\n"
               "Option \"TransformationMatrix\" \"" + matrix +"\"\n"
               "Option \"Device\" \"" + dev->GetDeviceNode() + "\"";
    }

    virtual void SetUpConfigAndLog() {
        std::tr1::tuple<enum MatrixType, int> t = GetParam();
        enum MatrixType mtype = std::tr1::get<0>(t);
        int nscreens = std::tr1::get<1>(t);

        std::string opts = EvdevOptions(mtype);

        switch(nscreens) {
            case 1: SetUpSingleScreen(opts); break;
            case 2: SetupDualHead(opts); break;
            default:
                    FAIL();
        }
    }

    virtual void SetUpSingleScreen(std::string &evdev_options) {
        config.AddDefaultScreenWithDriver();
        config.AddInputSection("evdev", "--device--", evdev_options);
        config.AddInputSection("kbd", "kbd-device",
                               "Option \"CoreKeyboard\" \"on\"\n");
        config.WriteConfig();
    }

    virtual void SetupDualHead(std::string &evdev_options) {
        config.SetAutoAddDevices(true);
        config.AppendRawConfig(std::string() +
            "Section \"ServerLayout\"\n"
            "	Identifier     \"X.org Configured\"\n"
            "	Screen         0 \"Screen0\"\n"
            "	Screen         1 \"Screen1\" RightOf \"Screen0\"\n"
            "	Option         \"Xinerama\" \"off\"\n"
            "   Option         \"AutoAddDevices\" \"off\"\n"
            "EndSection\n"
            "\n"
            "Section \"Device\"\n"
            "	Identifier  \"Card0\"\n"
            "	Driver      \"dummy\"\n"
            "EndSection\n"
            "\n"
            "Section \"Device\"\n"
            "	Identifier  \"Card1\"\n"
            "	Driver      \"dummy\"\n"
            "EndSection\n"
            "\n"
            "Section \"Screen\"\n"
            "	Identifier \"Screen0\"\n"
            "	Device     \"Card0\"\n"
            "EndSection\n"
            "\n"
            "Section \"Screen\"\n"
            "	Identifier \"Screen1\"\n"
            "	Device     \"Card1\"\n"
            "EndSection");
        config.AddInputSection("evdev", "--device--", evdev_options, false);
        config.AddInputSection("kbd", "kbd-device",
                               "Option \"CoreKeyboard\" \"on\"\n",
                               false);
        config.WriteConfig();
    }
};


TEST_P(PointerAbsoluteTransformationMatrixTest, XI2ValuatorData)
{
    std::tr1::tuple<enum MatrixType, int> t = GetParam();
    enum MatrixType mtype = std::tr1::get<0>(t);
    int nscreens = std::tr1::get<1>(t);

    std::string matrix;
    switch(mtype) {
        case IDENTITY: matrix = "identity marix"; break;
        case LEFT_HALF: matrix = "left half"; break;
        case RIGHT_HALF: matrix = "right half"; break;
    }
    std::stringstream screens;
    screens << nscreens;

    XORG_TESTCASE("Set transformation matrix on absolute tablet\n"
                  "XI2 valuator data must match input data\n"
                  "https://bugs.freedesktop.org/show_bug.cgi?id=63098\n");
    SCOPED_TRACE("Matrix: " + matrix);
    SCOPED_TRACE("nscreens: " + screens.str());


    ::Display *dpy = Display();

    /* Can't register for XIAllDevices, on a dual-head crossing into the new
       stream will generate an XTest Pointer event */
    int deviceid;
    ASSERT_TRUE(FindInputDeviceByName(dpy, "--device--", &deviceid));

    XIEventMask mask;
    mask.deviceid = deviceid;
    mask.mask_len = XIMaskLen(XI_Motion);
    mask.mask = new unsigned char[mask.mask_len]();
    XISetMask(mask.mask, XI_Motion);
    XISetMask(mask.mask, XI_ButtonPress);
    XISetMask(mask.mask, XI_ButtonRelease);

    XISelectEvents(dpy, RootWindow(dpy, 0), &mask, 1);
    if (nscreens > 1)
        XISelectEvents(dpy, RootWindow(dpy, 1), &mask, 1);
    XSync(dpy, False);
    delete[] mask.mask;

    int w = DisplayWidth(dpy, 0);

    int minx, maxx;
    dev->GetAbsData(ABS_X, &minx, &maxx);

    dev->PlayOne(EV_ABS, ABS_X, 1000);
    dev->PlayOne(EV_ABS, ABS_Y, 1000);
    dev->PlayOne(EV_ABS, ABS_DISTANCE, 0);
    dev->PlayOne(EV_KEY, BTN_TOOL_PEN, 1, true);

    /* drop first event */
    ASSERT_EVENT(XIDeviceEvent, m, dpy, GenericEvent, xi2_opcode, XI_Motion);

    dev->PlayOne(EV_ABS, ABS_X, minx, true);

    double expected_minx = minx,
           expected_maxx = maxx;
    switch (mtype) {
        /* no matrix, we expect the device range */
        case IDENTITY:
            break;
        /* left half we expect the device to go min-max/2 on a single-screen
           setup. dual-screen is per-device range*/
        case LEFT_HALF:
            if (nscreens == 1)
                expected_maxx = maxx/2;
            break;
        /* right half we expect the device to go from something smaller than
           max/2 to max. the device has ~46 units per pixel, if we map to
           512 on the screen that's not maxx/2, it's something less than
           that (see the scaling formula) */
        case RIGHT_HALF:
            if (nscreens == 1)
                expected_minx = w/2.0 * maxx/(w-1);
            else {
            /*  problem: scale to desktop == 1024
                re-scale to device == doesn't give us minx because the
                half-point of the display may not be the exact half-point of
                the device if the device res is > screen res.

                device range:   [.............|.............]
                                              ^ device range/2
                pixels: ...[    ][    ][    ]|[     ][     ][     ]...
                                             ^ 0/0 on screen2
                so by scaling from desktop/2 (0/0 on S2) we re-scale into a
                device range that isn't exactly 0 because that device
                coordinate would resolve to pixel desktop/2-1.

                so we expect minx to be _less_ than what would be one-pixel
                on the device.
             */
                expected_minx = maxx/(w-1);
            }
            break;
    }

    ASSERT_EVENT(XIDeviceEvent, m1, dpy, GenericEvent, xi2_opcode, XI_Motion);
    ASSERT_EQ(m1->deviceid, deviceid);
    ASSERT_TRUE(XIMaskIsSet(m1->valuators.mask, 0));
    if (mtype == RIGHT_HALF && nscreens > 1)
        ASSERT_TRUE(double_cmp(m1->valuators.values[0], expected_minx) <= 0);
    else
        ASSERT_TRUE(double_cmp(m1->valuators.values[0], expected_minx) == 0);

    /* y will be scaled in some weird way (depending on proportion of
       screen:tablet), so we ignore it here */

    dev->PlayOne(EV_ABS, ABS_X, maxx, true);

    ASSERT_EVENT(XIDeviceEvent, m2, dpy, GenericEvent, xi2_opcode, XI_Motion);
    ASSERT_EQ(m1->deviceid, deviceid);
    ASSERT_TRUE(XIMaskIsSet(m2->valuators.mask, 0));
    ASSERT_TRUE(double_cmp(m2->valuators.values[0], expected_maxx) == 0);
}

INSTANTIATE_TEST_CASE_P(, PointerAbsoluteTransformationMatrixTest,
                        ::testing::Combine(::testing::Values(IDENTITY, LEFT_HALF, RIGHT_HALF),
                                           ::testing::Values(1, 2)));
