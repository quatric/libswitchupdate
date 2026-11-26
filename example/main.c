/* Checks quatric/libswitchupdate for a newer release and offers to install it. */
#include <stdio.h>
#include <switch.h>
#include <nxupdate.h>

int main(int argc, char **argv)
{
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    NXUPDATE_SetAppPath(argc > 0 ? argv[0] : NULL);
    NXUPDATE_Start("quatric", "libswitchupdate", "0.0.1");
    printf("Checking for updates... (+ to quit)\n");

    int notified = 0;
    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 down = padGetButtonsDown(&pad);
        if (down & HidNpadButton_Plus) break;
        if (NXUPDATE_TakeNotice()) {
            notified = 1;
            printf("%s\nA: update   B: later\n", NXUPDATE_Message());
        }
        if (notified && (down & HidNpadButton_A) && NXUPDATE_CanInstall()) NXUPDATE_Install();
        if (NXUPDATE_InstallState() == NXUPDATE_INSTALL_DONE) {
            printf("Updated. Press + to exit.\n");
            notified = 0;
        }
        consoleUpdate(NULL);
    }
    consoleExit(NULL);
    return 0;
}
