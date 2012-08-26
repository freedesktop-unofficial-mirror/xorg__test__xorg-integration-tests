#include <stdexcept>
#include <fstream>
#include <xorg/gtest/xorg-gtest.h>

#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>

#include "input-driver-test.h"
#include "helpers.h"

static int count_devices(Display *dpy) {
    int ndevices;
    XIDeviceInfo *info;

    info = XIQueryDevice(dpy, XIAllDevices, &ndevices);
    XIFreeDeviceInfo(info);
    return ndevices;
}

TEST_P(SimpleInputDriverTest, LegacyDriver)
{
    std::string param;

    param = GetParam();

    int expected_devices;

    /* apparently when the acecad driver is loaded neither mouse nor kbd
     * loads - probably a bug but that's current behaviour on RHEL 6.3 */
    if (param.compare("acecad") == 0)
        expected_devices = 4;
    else {
        expected_devices = 6;
	/* xserver git 1357cd7251 , no <default pointer> */
	if (server.GetVersion().compare("1.11.0") > 0)
		expected_devices--;
    }

    ASSERT_EQ(count_devices(Display()), expected_devices);

    /* No joke, they all fail. some fail for missing Device option, others
     * for not finding the device, etc. */
    ASSERT_EQ(FindInputDeviceByName(Display(), "--device--"), 0);
}

INSTANTIATE_TEST_CASE_P(, SimpleInputDriverTest,
        ::testing::Values("acecad", "aiptek", "elographics",
                          "fpit", "hyperpen",  "mutouch",
                          "penmount"));

/**
 * Void input driver test class
 */
class VoidInputDriverTest : public InputDriverTest {
public:
    /**
     * Initialize an xorg.conf with a single CorePointer void device.
     */
    virtual void SetUp() {
        InputDriverTest::SetUp("void");
    }
};

TEST_F(VoidInputDriverTest, VoidDriver)
{
    ASSERT_EQ(FindInputDeviceByName(Display(), "--device--"), 1);
}


/***********************************************************************
 *                                                                     *
 *                               ACECAD                                *
 *                                                                     *
 ***********************************************************************/
TEST(AcecadInputDriver, WithOptionDevice)
{
    XOrgConfig config;
    xorg::testing::XServer server;

    config.AddInputSection("acecad", "--device--",
                           "Option \"CorePointer\" \"on\"\n"
                           "Option \"Device\" \"/dev/input/event0\"\n");
    config.AddDefaultScreenWithDriver();
    StartServer("acecad-type-stylus", server, config);

    ::Display *dpy = XOpenDisplay(server.GetDisplayString().c_str());
    int major = 2;
    int minor = 0;
    ASSERT_EQ(Success, XIQueryVersion(dpy, &major, &minor));

    int ndevices;
    XIDeviceInfo *info;

    info = XIQueryDevice(dpy, XIAllDevices, &ndevices);
    XIFreeDeviceInfo(info);

    /* VCP, VCK, xtest, mouse, keyboard, acecad */
    int expected_devices = 7;

    /* xserver git 1357cd7251 , no <default pointer> */
    if (server.GetVersion().compare("1.11.0") > 0)
	    expected_devices--;

    ASSERT_EQ(count_devices(dpy), expected_devices);
    ASSERT_EQ(FindInputDeviceByName(dpy, "--device--"), 1);

    config.RemoveConfig();
    server.RemoveLogFile();
}

/***********************************************************************
 *                                                                     *
 *                               AIPTEK                                *
 *                                                                     *
 ***********************************************************************/

TEST(AiptekInputDriver, TypeStylus)
{
    XOrgConfig config;
    xorg::testing::XServer server;

    config.AddInputSection("aiptek", "--device--",
                           "Option \"CorePointer\" \"on\"\n"
                           "Option \"Device\" \"/dev/input/event0\"\n"
                           "Option \"Type\" \"stylus\"");
    config.AddDefaultScreenWithDriver();
    StartServer("aiptek-type-stylus", server, config);

    ::Display *dpy = XOpenDisplay(server.GetDisplayString().c_str());
    int major = 2;
    int minor = 0;
    ASSERT_EQ(Success, XIQueryVersion(dpy, &major, &minor));

    /* VCP, VCK, xtest, mouse, keyboard, aiptek */
    int expected_devices = 7;

    /* xserver git 1357cd7251, no <default pointer> */
    if (server.GetVersion().compare("1.11.0") > 0)
	    expected_devices--;

    ASSERT_EQ(count_devices(dpy), expected_devices);
    ASSERT_EQ(FindInputDeviceByName(dpy, "--device--"), 1);

    config.RemoveConfig();
    server.RemoveLogFile();
}

TEST(AiptekInputDriver, TypeCursor)
{
    XOrgConfig config;
    xorg::testing::XServer server;

    config.AddInputSection("aiptek", "--device--",
                           "Option \"CorePointer\" \"on\"\n"
                           "Option \"Device\" \"/dev/input/event0\"\n"
                           "Option \"Type\" \"cursor\"");
    config.AddDefaultScreenWithDriver();
    StartServer("aiptek-type-cursor", server, config);

    ::Display *dpy = XOpenDisplay(server.GetDisplayString().c_str());
    int major = 2;
    int minor = 0;
    ASSERT_EQ(Success, XIQueryVersion(dpy, &major, &minor));

    /* VCP, VCK, xtest, mouse, keyboard, aiptek */
    int expected_devices = 7;

    /* xserver git 1357cd7251, no <default pointer> */
    if (server.GetVersion().compare("1.11.0") > 0)
	    expected_devices--;

    ASSERT_EQ(count_devices(dpy), expected_devices);
    ASSERT_EQ(FindInputDeviceByName(dpy, "--device--"), 1);

    config.RemoveConfig();
}

TEST(AiptekInputDriver, TypeEraser)
{
    XOrgConfig config;
    xorg::testing::XServer server;

    config.AddInputSection("aiptek", "--device--",
                           "Option \"CorePointer\" \"on\"\n"
                           "Option \"Device\" \"/dev/input/event0\"\n"
                           "Option \"Type\" \"eraser\"");
    config.AddDefaultScreenWithDriver();
    StartServer("aiptek-type-eraser", server, config);

    ::Display *dpy = XOpenDisplay(server.GetDisplayString().c_str());
    int major = 2;
    int minor = 0;
    ASSERT_EQ(Success, XIQueryVersion(dpy, &major, &minor));

    /* VCP, VCK, xtest, mouse, keyboard, aiptek */
    int expected_devices = 7;

    /* xserver git 1357cd7251, no <default pointer> */
    if (server.GetVersion().compare("1.11.0") > 0)
	    expected_devices--;

    ASSERT_EQ(count_devices(dpy), expected_devices);
    ASSERT_EQ(FindInputDeviceByName(dpy, "--device--"), 1);

    config.RemoveConfig();
    server.RemoveLogFile();
}

/***********************************************************************
 *                                                                     *
 *                            ELOGRAPHICS                              *
 *                                                                     *
 ***********************************************************************/
TEST(ElographicsDriver, Load)
{
    XOrgConfig config;
    xorg::testing::XServer server;

    config.AddInputSection("elographics", "--device--",
                           "Option \"CorePointer\" \"on\"\n"
                           "Option \"Device\" \"/dev/input/event0\"\n");
    config.AddDefaultScreenWithDriver();
    StartServer("elographics", server, config);

    ::Display *dpy = XOpenDisplay(server.GetDisplayString().c_str());
    int major = 2;
    int minor = 0;
    ASSERT_EQ(Success, XIQueryVersion(dpy, &major, &minor));

    /* VCP, VCK, xtest, mouse, keyboard, aiptek */
    int expected_devices = 7;

    /* xserver git 1357cd7251, no <default pointer> */
    if (server.GetVersion().compare("1.11.0") > 0)
	    expected_devices--;

    ASSERT_EQ(count_devices(dpy), expected_devices);
    if (FindInputDeviceByName(dpy, "--device--") != 1) {
        SCOPED_TRACE("\n"
                     "	Elographics device '--device--' not found.\n"
                     "	Maybe this is elographics < 1.4.\n"
                     "	Checking for TOUCHSCREEN instead, see git commit\n"
                     "	xf86-input-elographics-1.3.0-1-g55f337f");
        ASSERT_EQ(FindInputDeviceByName(dpy, "TOUCHSCREEN"), 1);
    } else
        ASSERT_EQ(FindInputDeviceByName(dpy, "--device--"), 1);

    config.RemoveConfig();
    server.Terminate(3000);
    server.RemoveLogFile();
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
