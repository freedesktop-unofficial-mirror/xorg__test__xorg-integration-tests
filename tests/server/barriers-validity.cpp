#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <xorg/gtest/xorg-gtest.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XInput2.h>

#include "barriers-common.h"
#include "helpers.h"

class BarrierSimpleTest : public BarrierTest {};

TEST_F(BarrierSimpleTest, CreateAndDestroyBarrier)
{
    XORG_TESTCASE("Create a valid pointer barrier.\n"
                  "Ensure PointerBarrier XID is valid.\n"
                  "Delete pointer barrier\n"
                  "Ensure no error is generated\n");

    ::Display *dpy = Display();
    Window root = DefaultRootWindow(dpy);
    PointerBarrier barrier;

    XSync(dpy, False);

    barrier = XFixesCreatePointerBarrier(dpy, root, 20, 20, 20, 40,
                                         BarrierPositiveX, 0, NULL);
    ASSERT_NE((PointerBarrier)None, barrier) << "Failed to create barrier.";

    SetErrorTrap(dpy);
    XFixesDestroyPointerBarrier(dpy, barrier);
    ASSERT_NO_ERROR(ReleaseErrorTrap(dpy));
}

TEST_F(BarrierSimpleTest, DestroyInvalidBarrier)
{
    XORG_TESTCASE("Delete invalid pointer barrier\n"
                  "Ensure error is generated\n");

    SetErrorTrap(Display());
    XFixesDestroyPointerBarrier(Display(), -1);
    XErrorEvent *error = ReleaseErrorTrap(Display());
    ASSERT_ERROR(error, xfixes_error_base + BadBarrier);
}

#define VALID_DIRECTIONS                                        \
    ::testing::Values(0L,                                       \
                      BarrierPositiveX,                         \
                      BarrierPositiveY,                         \
                      BarrierNegativeX,                         \
                      BarrierNegativeY,                         \
                      BarrierPositiveX | BarrierNegativeX,      \
                      BarrierPositiveY | BarrierNegativeX)

#define INVALID_DIRECTIONS                                              \
    ::testing::Values(BarrierPositiveX |                    BarrierPositiveY, \
                      BarrierNegativeX |                    BarrierNegativeY, \
                      BarrierPositiveX |                                       BarrierNegativeY, \
                      BarrierNegativeX | BarrierPositiveY,              \
                      BarrierPositiveX | BarrierNegativeX | BarrierPositiveY, \
                      BarrierPositiveX | BarrierNegativeX |                    BarrierNegativeY, \
                      BarrierPositiveX |                    BarrierPositiveY | BarrierNegativeY, \
                      BarrierNegativeX | BarrierPositiveY | BarrierNegativeY, \
                      BarrierPositiveX | BarrierNegativeX | BarrierPositiveY | BarrierNegativeY)


class BarrierZeroLength : public BarrierTest,
                          public ::testing::WithParamInterface<long int> {};
TEST_P(BarrierZeroLength, InvalidZeroLengthBarrier)
{
    XORG_TESTCASE("Create a pointer barrier with zero length.\n"
                  "Ensure server returns BadValue.");

    ::Display *dpy = Display();
    Window root = DefaultRootWindow(dpy);
    int directions = GetParam();

    XSync(dpy, False);

    SetErrorTrap(dpy);
    XFixesCreatePointerBarrier(dpy, root, 20, 20, 20, 20, directions, 0, NULL);
    XErrorEvent *error = ReleaseErrorTrap(dpy);
    ASSERT_ERROR(error, BadValue);
}

INSTANTIATE_TEST_CASE_P(, BarrierZeroLength, VALID_DIRECTIONS);

class BarrierNonZeroArea : public BarrierTest,
                           public ::testing::WithParamInterface<long int> {};
TEST_P(BarrierNonZeroArea, InvalidNonZeroAreaBarrier)
{
    XORG_TESTCASE("Create pointer barrier with non-zero area.\n"
                  "Ensure server returns BadValue\n");

    ::Display *dpy = Display();
    Window root = DefaultRootWindow(dpy);
    int directions = GetParam();

    XSync(dpy, False);

    SetErrorTrap(dpy);
    XFixesCreatePointerBarrier(dpy, root, 20, 20, 40, 40, directions, 0, NULL);
    XErrorEvent *error = ReleaseErrorTrap(dpy);
    ASSERT_ERROR(error, BadValue);
}
INSTANTIATE_TEST_CASE_P(, BarrierNonZeroArea, VALID_DIRECTIONS);

static void
BarrierPosForDirections(int directions, int *x1, int *y1, int *x2, int *y2)
{
    *x1 = 20;
    *y1 = 20;

    if (directions & BarrierPositiveY) {
        *x2 = 40;
        *y2 = 20;
    } else {
        *x2 = 20;
        *y2 = 40;
    }
}

class BarrierConflictingDirections : public BarrierTest,
                                     public ::testing::WithParamInterface<long int> {};
TEST_P(BarrierConflictingDirections, InvalidConflictingDirectionsBarrier)
{
    XORG_TESTCASE("Create a barrier with conflicting directions, such\n"
                  "as (PositiveX | NegativeY), and ensure that these\n"
                  "cases are handled properly.");

    ::Display *dpy = Display();
    Window root = DefaultRootWindow(dpy);

    int directions = GetParam();
    int x1, y1, x2, y2;
    BarrierPosForDirections(directions, &x1, &y1, &x2, &y2);

    XSync(dpy, False);

    SetErrorTrap(dpy);
    XFixesCreatePointerBarrier(dpy, root, x1, y1, x2, y2, directions, 0, NULL);
    XErrorEvent *error = ReleaseErrorTrap(dpy);

    /* Nonsensical directions are ignored -- they don't
     * raise a BadValue. Unfortunately, there's no way
     * to query an existing pointer barrier, so we can't
     * actually check that the masked directions are proper.
     *
     * Just ensure we have no error.
     */

    ASSERT_NO_ERROR(error);
}
INSTANTIATE_TEST_CASE_P(, BarrierConflictingDirections, INVALID_DIRECTIONS);

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
