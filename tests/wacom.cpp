#include <stdexcept>
#include <map>
#include <unistd.h>

#include <xorg/gtest/xorg-gtest.h>
#include <linux/input.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#define XK_LATIN1
#include <X11/keysymdef.h>
#include <X11/XF86keysym.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>

#include "wacom_devs.h"

#include <stdexcept>
#include <fstream>
#include <xorg/gtest/xorg-gtest.h>

#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>

#include <xorg/wacom-properties.h>
#include <unistd.h>

class WacomDriverTest : public xorg::testing::Test,
                        public ::testing::WithParamInterface<Tablet> {
protected:
    void WriteConfig() {
        std::stringstream s;
        Tablet tablet = GetParam();
        
        s << "/tmp/wacom-test-" << std::string(tablet.test_id) << ".conf";
        config_file = s.str();

        std::ofstream conffile(config_file.c_str());
        conffile.exceptions(std::ofstream::failbit | std::ofstream::badbit);

        conffile << ""
"            Section \"ServerFlags\""
"                Option \"Log\" \"flush\""
"            EndSection"
""
"            Section \"ServerLayout\""
"                Identifier \"Dummy layout\""
"                Screen 0 \"Dummy screen\" 0 0"
"                Option \"AutoAddDevices\" \"on\""
"            EndSection"
""
"            Section \"Screen\""
"                Identifier \"Dummy screen\""
"                Device \"Dummy video device\""
"            EndSection"
""
"            Section \"Device\""
"                Identifier \"Dummy video device\""
"                Driver \"dummy\""
"            EndSection";
        server.SetOption("-config", config_file);
    }

    void SetUpXIEventMask () {
        XIEventMask evmask;
        unsigned char mask[2] = { 0, 0 };

        XISetMask(mask, XI_HierarchyChanged);

        evmask.deviceid = XIAllDevices;
        evmask.mask_len = sizeof(mask);
        evmask.mask = mask;

        EXPECT_EQ(Success, XISelectEvents(Display(), DefaultRootWindow(Display()), &evmask, 1));
        XSync(Display(), False);
    }

    void CreateDevice ()
    {
        Tablet tablet = GetParam();
        char tool_name[255];

        SetUpXIEventMask ();

        dev = std::auto_ptr<xorg::testing::evemu::Device>(
            new xorg::testing::evemu::Device( RECORDINGS_DIR "tablets/" + std::string (tablet.descfile)));
        // FIXME: I don't think WaitForDevice() does take the timeout param into account
        if (tablet.stylus) {
            snprintf (tool_name, sizeof (tool_name), "%s %s", tablet.name, tablet.stylus);
            ASSERT_TRUE(xorg::testing::XServer::WaitForDevice(Display(), tool_name, 1000))
                << "Tool " << tool_name << " not found" << std::endl;
        }

        if (tablet.eraser) {
            snprintf (tool_name, sizeof (tool_name), "%s %s", tablet.name, tablet.eraser);
            ASSERT_TRUE(xorg::testing::XServer::WaitForDevice(Display(), tool_name, 1000))
                << "Tool " << tool_name << " not found" << std::endl;
        }

        if (tablet.cursor) {
            snprintf (tool_name, sizeof (tool_name), "%s %s", tablet.name, tablet.cursor);
            ASSERT_TRUE(xorg::testing::XServer::WaitForDevice(Display(), tool_name, 1000))
                << "Tool " << tool_name << " not found" << std::endl;
        }

        if (tablet.touch) {
            snprintf (tool_name, sizeof (tool_name), "%s %s", tablet.name, tablet.touch);
            ASSERT_TRUE(xorg::testing::XServer::WaitForDevice(Display(), tool_name, 1000))
                << "Tool " << tool_name << " not found" << std::endl;
        }

        if (tablet.pad) {
            snprintf (tool_name, sizeof (tool_name), "%s %s", tablet.name, tablet.pad);
            ASSERT_TRUE(xorg::testing::XServer::WaitForDevice(Display(), tool_name, 1000))
                << "Tool " << tool_name << " not found" << std::endl;
        }
    }

    void StartServer() {
        Tablet tablet = GetParam();

        server.SetOption("-logfile",log_file);
        server.Start();
        server.WaitForConnections();
        xorg::testing::Test::SetDisplayString(server.GetDisplayString());

        ASSERT_NO_FATAL_FAILURE(xorg::testing::Test::SetUp());

        CreateDevice();
    }

    virtual void SetUp()
    {
        Tablet tablet = GetParam();

        std::stringstream s;
        s << "/tmp/Xorg-wacom-test-" << std::string(tablet.test_id) << ".log";
        log_file = s.str();

        WriteConfig();/* Taken from gnome-settings-daemon */
        StartServer();
    }

    virtual void TearDown()
    {
        if (server.Pid() != -1)
            if (!server.Terminate())
                server.Kill();
        // Keep logs and config for investigation
        // if (config_file.size())
        //     unlink(config_file.c_str());
        // 
        // if (log_file.size())
        //    unlink(log_file.c_str());
    }

    std::string config_file;
    std::string log_file;
    xorg::testing::XServer server;

    std::auto_ptr<xorg::testing::evemu::Device> dev;
};

XIDeviceInfo* get_device_info_for_tool (const char *tool_name, XIDeviceInfo *list, int ndevices)
{
    XIDeviceInfo *info, *found;
    int loop;

    found = NULL;
    info = list;

    for (loop = 0; loop < ndevices; loop++, info++) {
        if (strcmp(info->name, tool_name) == 0) {
            EXPECT_EQ (found, (XIDeviceInfo *) NULL) << "Duplicate " << tool_name <<std::endl;
            found = info;
        }
    }
    EXPECT_NE (found, (XIDeviceInfo *) NULL) << "Tool not found " << tool_name <<std::endl;

    return found;
}

/* Return True if the given device has the property, or False otherwise */
bool test_property(Display *dpy, int deviceid, const char *prop_name)
{
    Status status;
    Atom prop;
    Atom realtype;
    int realformat;
    unsigned long nitems, bytes_after;
    unsigned char *data;

    prop = XInternAtom (dpy, prop_name, False);
    status = XIGetProperty(dpy, deviceid, prop , 0, 1000, False,
                           AnyPropertyType, &realtype, &realformat,
                           &nitems, &bytes_after, &data);
    if (data != NULL)
        XFree (data);
    
    if (status == Success)
        return True;
        
    return False;
}

TEST_P(WacomDriverTest, DeviceNames)
{
    Tablet tablet = GetParam();

    int major = 2;
    int minor = 0;
    ASSERT_EQ(Success, XIQueryVersion(Display(), &major, &minor));

    XIDeviceInfo *info, *list, *deviceinfo;
    bool prop_found;

    /* Wait for devices to settle */
    
    int ndevices;
    list = XIQueryDevice(Display(), XIAllDevices, &ndevices); 
    char tool_name[255];
        
    if (tablet.stylus) {
        snprintf (tool_name, sizeof (tool_name), "%s %s", tablet.name, tablet.stylus);
        deviceinfo = get_device_info_for_tool (tool_name, list, ndevices);

        prop_found = test_property (Display(), deviceinfo->deviceid, WACOM_PROP_PRESSURECURVE);
        EXPECT_EQ(prop_found, True) << "Property " << WACOM_PROP_PRESSURECURVE << " not found on " << tool_name << std::endl;

        prop_found = test_property (Display(), deviceinfo->deviceid, WACOM_PROP_ROTATION);
        EXPECT_EQ(prop_found, True) << "Property " << WACOM_PROP_ROTATION << " not found on " << tool_name << std::endl;

        prop_found = test_property (Display(), deviceinfo->deviceid, WACOM_PROP_TOOL_TYPE);
        EXPECT_EQ(prop_found, True) << "Property " << WACOM_PROP_TOOL_TYPE << " not found on " << tool_name << std::endl;
    }
        
    if (tablet.eraser) {
        snprintf (tool_name, sizeof (tool_name), "%s %s", tablet.name, tablet.eraser);
        deviceinfo = get_device_info_for_tool (tool_name, list, ndevices);

        prop_found = test_property (Display(), deviceinfo->deviceid, WACOM_PROP_PRESSURECURVE);
        EXPECT_EQ(prop_found, True) << "Property " << WACOM_PROP_PRESSURECURVE << " not found on " << tool_name << std::endl;

        prop_found = test_property (Display(), deviceinfo->deviceid, WACOM_PROP_ROTATION);
        EXPECT_EQ(prop_found, True) << "Property " << WACOM_PROP_ROTATION << " not found on " << tool_name << std::endl;
    }
        
    if (tablet.cursor) {
        snprintf (tool_name, sizeof (tool_name), "%s %s", tablet.name, tablet.cursor);
        deviceinfo = get_device_info_for_tool (tool_name, list, ndevices);

        prop_found = test_property (Display(), deviceinfo->deviceid,  WACOM_PROP_WHEELBUTTONS);
        EXPECT_EQ(prop_found, True) << "Property " << WACOM_PROP_WHEELBUTTONS << " not found on " << tool_name << std::endl;
    }
        
    if (tablet.pad) {
        snprintf (tool_name, sizeof (tool_name), "%s %s", tablet.name, tablet.pad);
        deviceinfo = get_device_info_for_tool (tool_name, list, ndevices);

        prop_found = test_property (Display(), deviceinfo->deviceid,  WACOM_PROP_STRIPBUTTONS);
        EXPECT_EQ(prop_found, True) << "Property " << WACOM_PROP_STRIPBUTTONS << " not found on " << tool_name << std::endl;
    }
        
    if (tablet.touch) {
        snprintf (tool_name, sizeof (tool_name), "%s %s", tablet.name, tablet.touch);
        deviceinfo = get_device_info_for_tool (tool_name, list, ndevices);

        prop_found = test_property (Display(), deviceinfo->deviceid,  WACOM_PROP_TOUCH);
        EXPECT_EQ(prop_found, True) << "Property " << WACOM_PROP_TOUCH << " not found on " << tool_name << std::endl;

        prop_found = test_property (Display(), deviceinfo->deviceid,  WACOM_PROP_ENABLE_GESTURE);
        EXPECT_EQ(prop_found, True) << "Property " << WACOM_PROP_ENABLE_GESTURE << " not found on " << tool_name << std::endl;

        prop_found = test_property (Display(), deviceinfo->deviceid,  WACOM_PROP_GESTURE_PARAMETERS);
        EXPECT_EQ(prop_found, True) << "Property " << WACOM_PROP_GESTURE_PARAMETERS << " not found on " << tool_name << std::endl;
    }

    XIFreeDeviceInfo(list);
}

void check_for_type (Display *dpy, XIDeviceInfo *list, int ndevices, const char *type)
{
    XIDeviceInfo *info;
    int loop;
    bool found = false;

    info = list;
    for (loop = 0; loop < ndevices; loop++, info++) {
        if (test_property (dpy, info->deviceid, type)) {
                found = True;
                break;
        }
    }
    ASSERT_TRUE(found) << type << " not found" << std::endl;
}

TEST_P(WacomDriverTest, DeviceType)
{
    Tablet tablet = GetParam();
    int major = 2;
    int minor = 0;
    ASSERT_EQ(Success, XIQueryVersion(Display(), &major, &minor));

    XIDeviceInfo *info, *list;
    int loop;
    bool found;

    int ndevices;
    list = XIQueryDevice(Display(), XIAllDevices, &ndevices); 
        
    if (tablet.stylus)
        check_for_type (Display(), list, ndevices, WACOM_PROP_XI_TYPE_STYLUS);

    if (tablet.eraser)
        check_for_type (Display(), list, ndevices, WACOM_PROP_XI_TYPE_ERASER);

    if (tablet.cursor)
        check_for_type (Display(), list, ndevices, WACOM_PROP_XI_TYPE_CURSOR);

    if (tablet.pad)
        check_for_type (Display(), list, ndevices, WACOM_PROP_XI_TYPE_PAD);

    if (tablet.touch)
        check_for_type (Display(), list, ndevices, WACOM_PROP_XI_TYPE_TOUCH);

    XIFreeDeviceInfo (list);
}

bool set_rotate(Display *dpy, int deviceid, const char *rotate)
{
    Status status;
    int rotation = 0;
    Atom prop, type;
    int format;
    unsigned char* data;
    unsigned long nitems, bytes_after;

    if (strcasecmp(rotate, "cw") == 0)
        rotation = 1;
    else if (strcasecmp(rotate, "ccw") == 0)
        rotation = 2;
    else if (strcasecmp(rotate, "half") == 0)
        rotation = 3;

    prop = XInternAtom(dpy, WACOM_PROP_ROTATION, True);
    if (!prop)
        return False;

    status = XIGetProperty(dpy, deviceid, prop, 0, 1000, False,
                           AnyPropertyType, &type, &format,
                           &nitems, &bytes_after, &data);

    if (status != Success || nitems == 0 || format != 8)
        return False;

    *data = rotation;
    
    XIChangeProperty(dpy, deviceid, prop, type, format,
                          PropModeReplace, data, nitems);
    XFlush(dpy);

    return True;
}

int stylus_move_right (Display *display, xorg::testing::evemu::Device *dev)
{
    int root_x1, root_y1, root_x2, root_y2;
    int loop, step;
    XEvent ev;
    
    XSync(display, False);
    while(XPending(display))
        XNextEvent(display, &ev);

    // Move to device coord (1000,1000)
    dev->PlayOne(EV_ABS, ABS_X, 1000, True);
    dev->PlayOne(EV_ABS, ABS_Y, 1000, True);
    dev->PlayOne(EV_ABS, ABS_DISTANCE, 0, True);
    dev->PlayOne(EV_KEY, BTN_TOOL_PEN, 1, True);
    XSync(display, False);

    // Capture motion events and save screen coordinates
    while(XCheckMaskEvent (display, PointerMotionMask, &ev)) {
        root_x1 = ev.xmotion.x_root;
        root_y1 = ev.xmotion.y_root;
    }

    root_x2 = root_x1;
    root_y2 = root_y1;
    
    step = 10;
    for (loop = 1000; loop < 3000; loop += step) {
        dev->PlayOne(EV_ABS, ABS_X, loop, True);
        dev->PlayOne(EV_ABS, ABS_DISTANCE, step, True);
    }
    dev->PlayOne(EV_KEY, BTN_TOOL_PEN, 0, True);

    XSync(display, False);
    EXPECT_NE(XPending(display), 0);

    while(XCheckMaskEvent (display, PointerMotionMask|PointerMotionHintMask, &ev)) {
        root_x2 = ev.xmotion.x_root;
        root_y2 = ev.xmotion.y_root;
    }
    printf ("X axis: %d Y axis: %d\n", root_x2 - root_x1, root_y2 - root_y1);

    while(XPending(display))
        XNextEvent(display, &ev);

    return (root_x2 - root_x1);
}

TEST_P(WacomDriverTest, Rotation)
{
    Tablet tablet = GetParam();

    int major = 2;
    int minor = 0;
    ASSERT_EQ(Success, XIQueryVersion(Display(), &major, &minor));

    XIDeviceInfo *info, *list, *deviceinfo;
    bool status;

    XSelectInput(Display(), DefaultRootWindow(Display()), ButtonPressMask | 
                                                          ButtonReleaseMask | 
                                                          PointerMotionMask | 
                                                          PointerMotionHintMask |
                                                          ButtonMotionMask);
    /* the server takes a while to start up bust the devices may not respond
       to events yet. Add a noop call that just delays everything long
       enough for this test to work */
    XInternAtom(Display(), "foo", True);
    XFlush(Display());
    
    int ndevices;
    list = XIQueryDevice(Display(), XIAllDevices, &ndevices);
    char tool_name[255];
    int delta;
    
    if (tablet.stylus) {
        snprintf (tool_name, sizeof (tool_name), "%s %s", tablet.name, tablet.stylus);
        deviceinfo = get_device_info_for_tool (tool_name, list, ndevices);

        // Try with no rotation
        status = set_rotate (Display(), deviceinfo->deviceid, "none");
        EXPECT_TRUE(stylus_move_right (Display(), dev.get()) > 0) << "Failed to rotate " << tool_name << std::endl;

        // Set opposite rotation
        status = set_rotate (Display(), deviceinfo->deviceid, "ccw");
        EXPECT_TRUE(stylus_move_right (Display(), dev.get()) < 0) << "Failed to rotate " << tool_name << std::endl;
    }

    XIFreeDeviceInfo(list);
}

INSTANTIATE_TEST_CASE_P(, WacomDriverTest,
        ::testing::ValuesIn(tablets));

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
