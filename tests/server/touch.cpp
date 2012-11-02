#include <stdexcept>

#include "helpers.h"
#include "device-interface.h"
#include "xit-server-input-test.h"

#include <xorg/gtest/xorg-gtest.h>

#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>

/**
 * A test fixture for testing the XInput 2.x extension.
 *
 * @tparam The XInput 2.x minor version
 */
class TouchTest : public XITServerInputTest,
                  public DeviceInterface,
                  public ::testing::WithParamInterface<int> {
protected:
    virtual void SetUp() {
        SetDevice("tablets/N-Trig-MultiTouch.desc");
        XITServerInputTest::SetUp();
    }

    virtual void SetUpConfigAndLog() {
        config.AddDefaultScreenWithDriver();
        config.SetAutoAddDevices(true);
        config.WriteConfig();
    }

    virtual int RegisterXI2(int major, int minor)
    {
        return XITServerInputTest::RegisterXI2(2, GetParam());
    }
};

TEST_P(TouchTest, XITouchscreenPointerEmulation)
{
    XORG_TESTCASE("Register for button and motion events.\n"
                  "Create a touch and a pointer device.\n"
                  "Moving the mouse must have a zero button mask.\n"
                  "Create a touch point on the screen.\n"
                  "Moving the mouse now must have button 1 set.\n"
                  "Release the touch point.\n"
                  "Moving the mouse now must have a zero button mask.\n");

    ASSERT_TRUE(xorg::testing::XServer::WaitForDevice(Display(), "N-Trig MultiTouch"));

    XIEventMask mask;
    mask.deviceid = XIAllMasterDevices;
    mask.mask_len = XIMaskLen(XI_Motion);
    mask.mask = reinterpret_cast<unsigned char*>(calloc(mask.mask_len, 1));
    XISetMask(mask.mask, XI_ButtonPress);
    XISetMask(mask.mask, XI_ButtonRelease);
    XISetMask(mask.mask, XI_Motion);

    XISelectEvents(Display(), DefaultRootWindow(Display()), &mask, 1);

    free(mask.mask);

    XFlush(Display());

    std::auto_ptr<xorg::testing::evemu::Device> mouse_device;
    try {
      mouse_device = std::auto_ptr<xorg::testing::evemu::Device>(
          new xorg::testing::evemu::Device(
              RECORDINGS_DIR "mice/PIXART-USB-OPTICAL-MOUSE.desc")
          );
    } catch (std::runtime_error &error) {
      std::cerr << "Failed to create evemu device, skipping test.\n";
      return;
    }

    ASSERT_TRUE(xorg::testing::XServer::WaitForDevice(Display(), "PIXART USB OPTICAL MOUSE"));

    XEvent event;
    XGenericEventCookie *xcookie;
    XIDeviceEvent *device_event;

    /* Move the mouse, check that the button is not pressed. */
    mouse_device->PlayOne(EV_REL, ABS_X, -1, 1);
    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_Motion));

    ASSERT_EQ(Success, XNextEvent(Display(), &event));
    xcookie = &event.xcookie;
    ASSERT_TRUE(XGetEventData(Display(), xcookie));

    device_event = reinterpret_cast<XIDeviceEvent*>(xcookie->data);
    ASSERT_FALSE(XIMaskIsSet(device_event->buttons.mask, 1));
    XFreeEventData(Display(), xcookie);


    /* Touch the screen, wait for press event */
    dev->Play(RECORDINGS_DIR "tablets/N-Trig-MultiTouch.touch_1_begin.events");
    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_ButtonPress));

    ASSERT_EQ(Success, XNextEvent(Display(), &event));


    /* Move the mouse again, button 1 should now be pressed. */
    mouse_device->PlayOne(EV_REL, ABS_X, -1, 1);
    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_Motion));

    ASSERT_EQ(Success, XNextEvent(Display(), &event));
    xcookie = &event.xcookie;
    ASSERT_TRUE(XGetEventData(Display(), xcookie));

    device_event = reinterpret_cast<XIDeviceEvent*>(xcookie->data);
    ASSERT_TRUE(XIMaskIsSet(device_event->buttons.mask, 1));
    XFreeEventData(Display(), xcookie);


    /* Release the screen, wait for release event */
    dev->Play(RECORDINGS_DIR "tablets/N-Trig-MultiTouch.touch_1_end.events");
    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_ButtonRelease));

    ASSERT_EQ(Success, XNextEvent(Display(), &event));


    /* Move the mouse again, button 1 should now be released. */
    mouse_device->PlayOne(EV_REL, ABS_X, -1, 1);
    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_Motion));

    ASSERT_EQ(Success, XNextEvent(Display(), &event));
    xcookie = &event.xcookie;
    ASSERT_TRUE(XGetEventData(Display(), xcookie));

    device_event = reinterpret_cast<XIDeviceEvent*>(xcookie->data);
    ASSERT_FALSE(XIMaskIsSet(device_event->buttons.mask, 1));
    XFreeEventData(Display(), xcookie);
}

TEST_P(TouchTest, EmulatedButtonMaskOnTouchBeginEndCore)
{
    XORG_TESTCASE("Select for core Motion and ButtonPress/Release events on the root window.\n"
                  "Create a pointer-emulating touch event.\n"
                  "Expect a Motion event before the button press.\n"
                  "The button mask in the motion event must be 0.\n"
                  "The button mask in the button press event must be 0.\n"
                  "The button mask in the button release event must be set for button 1.");

    XSelectInput(Display(), DefaultRootWindow(Display()),
                 PointerMotionMask|ButtonPressMask|ButtonReleaseMask);
    XSync(Display(), False);

    dev->Play(RECORDINGS_DIR "tablets/N-Trig-MultiTouch.touch_1_begin.events");
    dev->Play(RECORDINGS_DIR "tablets/N-Trig-MultiTouch.touch_1_end.events");

    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           MotionNotify,
                                                           -1, -1));
    XEvent ev;
    XNextEvent(Display(), &ev);
    ASSERT_EQ(ev.type, MotionNotify);
    EXPECT_EQ(ev.xmotion.state, 0U);

    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           ButtonPress,
                                                           -1, -1));
    XNextEvent(Display(), &ev);
    ASSERT_EQ(ev.type, ButtonPress);
    EXPECT_EQ(ev.xbutton.state, 0U);
    EXPECT_EQ(ev.xbutton.button, 1U);

    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           ButtonRelease,
                                                           -1, -1));
    while (ev.type != ButtonRelease)
        XNextEvent(Display(), &ev);

    ASSERT_EQ(ev.type, ButtonRelease);
    EXPECT_EQ(ev.xbutton.state, (unsigned int)Button1Mask);
    EXPECT_EQ(ev.xbutton.button, 1U) << "ButtonRelease must have button 1 mask down";
}

TEST_P(TouchTest, EmulatedButtonMaskOnTouchBeginEndXI2)
{
    XORG_TESTCASE("Select for XI_Motion and XI_ButtonPress/Release events on the root window.\n"
                  "Create a pointer-emulating touch event.\n"
                  "Expect a Motion event before the button press.\n"
                  "The button mask in the motion event must be 0.\n"
                  "The button mask in the button press event must be 0.\n"
                  "The button mask in the button release event must be set for button 1.");

    XIEventMask mask;
    mask.deviceid = VIRTUAL_CORE_POINTER_ID;
    mask.mask_len = XIMaskLen(XI_Motion);
    mask.mask = new unsigned char[mask.mask_len]();
    XISetMask(mask.mask, XI_Motion);
    XISetMask(mask.mask, XI_ButtonPress);
    XISetMask(mask.mask, XI_ButtonRelease);

    ASSERT_EQ(Success,
              XISelectEvents(Display(), DefaultRootWindow(Display()), &mask, 1));
    XSync(Display(), False);
    delete[] mask.mask;

    dev->Play(RECORDINGS_DIR "tablets/N-Trig-MultiTouch.touch_1_begin.events");
    dev->Play(RECORDINGS_DIR "tablets/N-Trig-MultiTouch.touch_1_end.events");

    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_Motion));
    XEvent ev;
    XNextEvent(Display(), &ev);
    ASSERT_EQ(ev.type, GenericEvent);
    ASSERT_EQ(ev.xcookie.extension, xi2_opcode);
    ASSERT_EQ(ev.xcookie.evtype, XI_Motion);
    ASSERT_TRUE(XGetEventData(Display(), &ev.xcookie));

    XIDeviceEvent *motion = reinterpret_cast<XIDeviceEvent*>(ev.xcookie.data);
    for (int i = 0; i < motion->buttons.mask_len * 8; i++)
        EXPECT_FALSE(XIMaskIsSet(motion->buttons.mask, i)) << "mask down for button " << i;

    XFreeEventData(Display(), &ev.xcookie);

    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_ButtonPress));
    XNextEvent(Display(), &ev);
    ASSERT_EQ(ev.type, GenericEvent);
    ASSERT_EQ(ev.xcookie.extension, xi2_opcode);
    ASSERT_EQ(ev.xcookie.evtype, XI_ButtonPress);
    ASSERT_TRUE(XGetEventData(Display(), &ev.xcookie));

    XIDeviceEvent *press = reinterpret_cast<XIDeviceEvent*>(ev.xcookie.data);
    for (int i = 0; i < press->buttons.mask_len * 8; i++)
        EXPECT_FALSE(XIMaskIsSet(press->buttons.mask, i)) << "mask down for button " << i;

    XFreeEventData(Display(), &ev.xcookie);

    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_ButtonRelease));
    while(ev.xcookie.evtype != XI_ButtonRelease)
        XNextEvent(Display(), &ev);
    ASSERT_EQ(ev.type, GenericEvent);
    ASSERT_EQ(ev.xcookie.extension, xi2_opcode);
    ASSERT_EQ(ev.xcookie.evtype, XI_ButtonRelease);
    ASSERT_TRUE(XGetEventData(Display(), &ev.xcookie));

    XIDeviceEvent *release = reinterpret_cast<XIDeviceEvent*>(ev.xcookie.data);
    EXPECT_GT(release->buttons.mask_len, 0);
    EXPECT_TRUE(XIMaskIsSet(release->buttons.mask, 1)) << "ButtonRelease must have button 1 down";
    for (int i = 0; i < release->buttons.mask_len * 8; i++) {
        if (i == 1)
            continue;
        EXPECT_FALSE(XIMaskIsSet(release->buttons.mask, i)) << "mask down for button " << i;
    }

    XFreeEventData(Display(), &ev.xcookie);
}

TEST_P(TouchTest, XIQueryPointerTouchscreen)
{
    XORG_TESTCASE("Create a touch device, create a touch point from that device\n"
                  "XIQueryPointer() should return:\n"
                  " - button 1 state down for XI 2.0 and 2.1 clients\n"
                  " - button 1 state up for XI 2.2+ clients");

    XIEventMask mask;
    mask.deviceid = XIAllMasterDevices;
    mask.mask_len = XIMaskLen(XI_ButtonPress);
    mask.mask = new unsigned char[mask.mask_len]();
    XISetMask(mask.mask, XI_ButtonPress);

    ASSERT_EQ(Success,
              XISelectEvents(Display(), DefaultRootWindow(Display()), &mask,
                             1));

    delete[] mask.mask;

    XFlush(Display());

    ASSERT_TRUE(xorg::testing::XServer::WaitForDevice(Display(), "N-Trig MultiTouch"));

    dev->Play(RECORDINGS_DIR "tablets/N-Trig-MultiTouch.touch_1_begin.events");

    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_ButtonPress));

    XEvent event;
    ASSERT_EQ(Success, XNextEvent(Display(), &event));

    XGenericEventCookie *xcookie = &event.xcookie;
    ASSERT_TRUE(XGetEventData(Display(), xcookie));

    XIDeviceEvent *device_event =
        reinterpret_cast<XIDeviceEvent*>(xcookie->data);

    Window root;
    Window child;
    double root_x;
    double root_y;
    double win_x;
    double win_y;
    XIButtonState buttons;
    XIModifierState modifiers;
    XIGroupState group;
    ASSERT_TRUE(XIQueryPointer(Display(), device_event->deviceid,
                               DefaultRootWindow(Display()), &root, &child,
                               &root_x, &root_y, &win_x, &win_y, &buttons,
                               &modifiers, &group));

    /* Test if button 1 is pressed */
    ASSERT_GE(buttons.mask_len, XIMaskLen(2));
    if (GetParam() < 2)
        EXPECT_TRUE(XIMaskIsSet(buttons.mask, 1));
    else
        EXPECT_FALSE(XIMaskIsSet(buttons.mask, 1));

    XFreeEventData(Display(), xcookie);
}

#ifdef HAVE_XI22
TEST_P(TouchTest, DisableDeviceEndTouches)
{
    XORG_TESTCASE("Register for touch events.\n"
                  "Create a touch device, create a touch point.\n"
                  "Disable the touch device.\n"
                  "Ensure a TouchEnd is sent for that touch point\n");

    /* This is an XInput 2.2 and later test only */
    if (GetParam() < 2)
        return;

    XIEventMask mask;
    mask.deviceid = XIAllMasterDevices;
    mask.mask_len = XIMaskLen(XI_TouchEnd);
    mask.mask = new unsigned char[mask.mask_len]();
    XISetMask(mask.mask, XI_TouchBegin);
    XISetMask(mask.mask, XI_TouchUpdate);
    XISetMask(mask.mask, XI_TouchEnd);

    ASSERT_EQ(Success,
              XISelectEvents(Display(), DefaultRootWindow(Display()), &mask,
                             1));

    delete[] mask.mask;

    XFlush(Display());

    ASSERT_TRUE(xorg::testing::XServer::WaitForDevice(Display(), "N-Trig MultiTouch"));

    dev->Play(RECORDINGS_DIR "tablets/N-Trig-MultiTouch.touch_1_begin.events");

    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_TouchBegin));

    XEvent event;
    ASSERT_EQ(Success, XNextEvent(Display(), &event));

    XGenericEventCookie *xcookie = &event.xcookie;
    ASSERT_TRUE(XGetEventData(Display(), xcookie));

    XIDeviceEvent *device_event =
        reinterpret_cast<XIDeviceEvent*>(xcookie->data);

    XDevice *xdevice = XOpenDevice(Display(), device_event->sourceid);
    XFreeEventData(Display(), xcookie);
    ASSERT_TRUE(xdevice != NULL);

    XDeviceEnableControl enable_control;
    enable_control.enable = false;
    XDeviceControl *control =
        reinterpret_cast<XDeviceControl*>(&enable_control);
    ASSERT_EQ(XChangeDeviceControl(Display(), xdevice, DEVICE_ENABLE, control),
              Success);
    XCloseDevice(Display(), xdevice);
    XFlush(Display());

    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_TouchEnd));
}

/**
 * Test class for testing XISelectEvents on touch masks
 *
 * @tparam The device ID
 */
class XISelectEventsTouchTest : public TouchTest
{
protected:
    virtual int RegisterXI2(int major, int minor)
    {
        return XITServerInputTest::RegisterXI2(2, 2);
    }
};

TEST_P(XISelectEventsTouchTest, TouchSelectionConflicts)
{
    XORG_TESTCASE("If client A has a selection on a device,\n"
                  "client B selecting on the same device returns BadAccess.\n"
                  "If client A has a selection on XIAll(Master)Devices, \n"
                  "selecting on the same or a specific device returns"
                  "BadAccess\n");

    unsigned char m[XIMaskLen(XI_TouchEnd)] = {0};
    XIEventMask mask;
    mask.mask = m;
    mask.mask_len = sizeof(m);

    XISetMask(mask.mask, XI_TouchBegin);
    XISetMask(mask.mask, XI_TouchUpdate);
    XISetMask(mask.mask, XI_TouchEnd);

    int clientA_deviceid = GetParam();

    /* client A */
    mask.deviceid = clientA_deviceid;
    ::Display *dpy2 = XOpenDisplay(server.GetDisplayString().c_str());
    ASSERT_TRUE(dpy2);

    XISelectEvents(dpy2, DefaultRootWindow(dpy2), &mask, 1);
    XSync(dpy2, False);

    /* covers XIAllDevices, XIAllMasterDevices and VCP */
    for (int clientB_deviceid = 0; clientB_deviceid < 3; clientB_deviceid++) {
        SetErrorTrap(Display());
        mask.deviceid = clientB_deviceid;
        XISelectEvents(Display(), DefaultRootWindow(Display()), &mask, 1);
        XErrorEvent *error = ReleaseErrorTrap(Display());

        if (clientA_deviceid <= clientB_deviceid) {
            ASSERT_ERROR(error, BadAccess) << "failed for " << clientA_deviceid << "/" << clientB_deviceid;
        } else {
            ASSERT_NO_ERROR(error) << "failed for " << clientA_deviceid << "/" << clientB_deviceid;
        }
    }
}
INSTANTIATE_TEST_CASE_P(, XISelectEventsTouchTest,
        ::testing::Values(XIAllDevices, XIAllMasterDevices, VIRTUAL_CORE_POINTER_ID));


TEST_P(TouchTest, TouchEventsButtonState)
{
    if (GetParam() < 2)
        return;

    XORG_TESTCASE("Select for touch events on the root window.\n"
                  "Create a pointer-emulating touch event.\n"
                  "The button mask in the touch begin event must be 0.\n"
                  "The button mask in the touch update events must be set for button 1.\n"
                  "The button mask in the touch end event must be set for button 1.");

    XIEventMask mask;
    mask.deviceid = VIRTUAL_CORE_POINTER_ID;
    mask.mask_len = XIMaskLen(XI_TouchEnd);
    mask.mask = new unsigned char[mask.mask_len]();
    XISetMask(mask.mask, XI_TouchBegin);
    XISetMask(mask.mask, XI_TouchUpdate);
    XISetMask(mask.mask, XI_TouchEnd);

    ASSERT_EQ(Success,
              XISelectEvents(Display(), DefaultRootWindow(Display()), &mask, 1));
    XSync(Display(), False);
    delete[] mask.mask;

    dev->Play(RECORDINGS_DIR "tablets/N-Trig-MultiTouch.touch_1_begin.events");
    dev->Play(RECORDINGS_DIR "tablets/N-Trig-MultiTouch.touch_1_end.events");

    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_TouchBegin));

    XEvent ev;
    XNextEvent(Display(), &ev);
    ASSERT_EQ(ev.type, GenericEvent);
    ASSERT_EQ(ev.xcookie.extension, xi2_opcode);
    ASSERT_EQ(ev.xcookie.evtype, XI_TouchBegin);
    ASSERT_TRUE(XGetEventData(Display(), &ev.xcookie));

    XIDeviceEvent *begin = reinterpret_cast<XIDeviceEvent*>(ev.xcookie.data);
    for (int i = 0; i < begin->buttons.mask_len * 8; i++)
        EXPECT_FALSE(XIMaskIsSet(begin->buttons.mask, i)) << "mask down for button " << i;

    XFreeEventData(Display(), &ev.xcookie);

    ASSERT_TRUE(xorg::testing::XServer::WaitForEventOfType(Display(),
                                                           GenericEvent,
                                                           xi2_opcode,
                                                           XI_TouchEnd));

    XNextEvent(Display(), &ev);
    ASSERT_EQ(ev.type, GenericEvent);
    ASSERT_EQ(ev.xcookie.extension, xi2_opcode);
    ASSERT_EQ(ev.xcookie.evtype, XI_TouchEnd);
    ASSERT_TRUE(XGetEventData(Display(), &ev.xcookie));

    XIDeviceEvent *end = reinterpret_cast<XIDeviceEvent*>(ev.xcookie.data);
    EXPECT_GT(end->buttons.mask_len, 0);
    EXPECT_TRUE(XIMaskIsSet(end->buttons.mask, 1)) << "ButtonRelease must have button 1 down";
    for (int i = 0; i < end->buttons.mask_len * 8; i++) {
        if (i == 1)
            continue;
        EXPECT_FALSE(XIMaskIsSet(end->buttons.mask, i)) << "mask down for button " << i;
    }

    XFreeEventData(Display(), &ev.xcookie);
}

#endif /* HAVE_XI22 */

INSTANTIATE_TEST_CASE_P(, TouchTest, ::testing::Range(0, XI_2_Minor + 1));

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
