#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "helpers.h"

#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xrandr.h>

void StartServer(std::string prefix, ::xorg::testing::XServer &server, XOrgConfig &config) {
    std::stringstream conf;
    conf << "/tmp/" << prefix << ".conf";

    std::stringstream logfile;
    logfile << "/tmp/Xorg-" << prefix << ".log";

    config.SetPath(conf.str());
    config.WriteConfig();
    server.SetOption("-config", config.GetPath());
    server.SetOption("-logfile", logfile.str());
    server.SetDisplayNumber(133);
    server.Start();
    server.WaitForConnections();
}

int FindInputDeviceByName(Display *dpy, const std::string &device_name, int *deviceid)
{
    int ndevices;
    XIDeviceInfo *info;

    info = XIQueryDevice(dpy, XIAllDevices, &ndevices);

    int found = 0;
    while(ndevices--) {
        if (strcmp(info[ndevices].name, device_name.c_str()) == 0) {
            if (deviceid)
                *deviceid = info[ndevices].deviceid;
            found++;
        }
    }

    if (found > 1) {
        SCOPED_TRACE("More than one device named '" + device_name +
                     "' found.\nThis may cause some tests to fail.\n");
    }

    XIFreeDeviceInfo(info);

    return found;
}

Bool InitRandRSupport (Display *dpy, int *event_base, int *error_base)
{
    int ignore1, ignore2;
    int major, minor;

    if (event_base == NULL)
        event_base = &ignore1;

    if (error_base == NULL)
        error_base = &ignore2;

    if (!XRRQueryExtension (dpy, event_base, error_base))
        return False;

    XRRQueryVersion (dpy, &major, &minor);
    if (major <= 1 && minor < 2)
        return False;

    XSelectInput (dpy, DefaultRootWindow(dpy), StructureNotifyMask);
    XRRSelectInput (dpy,
                    DefaultRootWindow(dpy),
                    RRScreenChangeNotifyMask	|
                    RRCrtcChangeNotifyMask	|
                    RROutputPropertyNotifyMask);

    return True;
}

int GetNMonitors (Display *dpy)
{
    XRRScreenResources *resources;
    int i, n_active_outputs = 0;

    resources = XRRGetScreenResourcesCurrent (dpy, DefaultRootWindow(dpy));
    for (i = 0; i < resources->noutput; ++i) {
        XRROutputInfo *output = XRRGetOutputInfo (dpy, resources, resources->outputs[i]);

        if ((output->connection != RR_Disconnected) && output->crtc)
            n_active_outputs++;

        XRRFreeOutputInfo (output);
    }

    XRRFreeScreenResources (resources);

    return n_active_outputs;
}

void GetMonitorGeometry (Display *dpy, int monitor, int *x, int *y, int *width, int *height)
{
    XRRScreenResources *resources;
    int i, n_active_outputs = 0;

    resources = XRRGetScreenResourcesCurrent (dpy, DefaultRootWindow(dpy));
    for (i = 0; i < resources->noutput; ++i) {
        XRROutputInfo *output = XRRGetOutputInfo (dpy, resources, resources->outputs[i]);

        if ((output->connection == RR_Disconnected) || !output->crtc) {
            XRRFreeOutputInfo (output);
            continue;
        }

        if (n_active_outputs == monitor) {
            XRRCrtcInfo *crtc = XRRGetCrtcInfo (dpy, resources, output->crtc);

            if (x)
                *x = crtc->x;
            if (y)
                *y = crtc->y;
            if (width)
                *width = crtc->width;
            if (height)
                *height = crtc->height;

            XRRFreeCrtcInfo (crtc);
            break;
        }
        n_active_outputs++;
    }

    XRRFreeScreenResources (resources);
}
