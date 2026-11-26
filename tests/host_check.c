/* End-to-end check against the real GitHub API from a desktop, using the
 * library's own TLS configuration and root certificates:
 *   make -f tests/Makefile.host build/host_check
 *   ./build/host_check quatric PictoChat-Client 1.0.0
 * With a fourth argument (argv[0] of a .nro, on the stand-in SD card build/sd)
 * it also installs the update there:
 *   ./build/host_check quatric PictoChat-Client 1.0.0 sdmc:/switch/PictoChat/PictoChat.nro
 */
#include <stdio.h>
#include <unistd.h>
#include "nxupdate.h"

int main(int argc, char **argv)
{
    if (argc < 4) { fprintf(stderr, "usage: %s owner repo version [app_path]\n", argv[0]); return 2; }
    if (argc > 4) NXUPDATE_SetAppPath(argv[4]);
    NXUPDATE_Start(argv[1], argv[2], argv[3]);
    nxupdate_state st = NXUPDATE_Wait(30000);
    static const char *names[] = { "idle", "checking", "up to date", "available", "failed", "cancelled" };
    printf("%s: %s latest=%s installable=%d\n%s\n", NXUPDATE_Url(), names[st], NXUPDATE_Latest(),
           NXUPDATE_CanInstall(), NXUPDATE_Message());
    if (argc > 4 && NXUPDATE_CanInstall()) {
        if (NXUPDATE_Install() < 0) { printf("install did not start\n"); return 1; }
        int last = -1;
        nxupdate_install_state ist;
        while ((ist = NXUPDATE_InstallState()) == NXUPDATE_INSTALL_DOWNLOADING || ist == NXUPDATE_INSTALL_EXTRACTING) {
            if (NXUPDATE_Progress() / 10 != last) printf("  %d%%\n", (last = NXUPDATE_Progress() / 10) * 10);
            usleep(50000);
        }
        NXUPDATE_Wait(5000);
        printf("install: %s %s -> %s\n", ist == NXUPDATE_INSTALL_DONE ? "done" : "failed", NXUPDATE_InstallError(),
               NXUPDATE_InstallDir());
        return ist != NXUPDATE_INSTALL_DONE;
    }
    return st == NXUPDATE_FAILED;
}
