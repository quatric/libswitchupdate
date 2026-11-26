/* The update popup's logic, shared by GUI apps: they only draw it. */
#include <stdio.h>
#include "nxupdate.h"

/* HidNpadButton_A / _B from <switch.h>, kept here so the popup logic
 * builds anywhere. */
#define BUTTON_A 0x0001
#define BUTTON_B 0x0002

static int open;

int NXUPDATE_Popup(uint32_t down, nxupdate_popup *p)
{
    if (NXUPDATE_TakeNotice()) open = 1;
    if (!open) return NXUPDATE_POPUP_CLOSED;

    int a = (down & BUTTON_A) != 0, b = (down & BUTTON_B) != 0;
    int result = NXUPDATE_POPUP_OPEN;
    switch (NXUPDATE_InstallState()) {
    case NXUPDATE_INSTALL_IDLE:
        if (a && NXUPDATE_CanInstall()) NXUPDATE_Install();
        else if (a || b) open = 0;
        break;
    case NXUPDATE_INSTALL_DOWNLOADING:
    case NXUPDATE_INSTALL_EXTRACTING:
        if (b) NXUPDATE_Cancel();
        break;
    case NXUPDATE_INSTALL_DONE:
        if (a) result = NXUPDATE_POPUP_EXIT;
        break;
    case NXUPDATE_INSTALL_FAILED:
        if (a || b) open = 0;
        break;
    }
    if (!open) return NXUPDATE_POPUP_CLOSED;

    /* Describe the state after this frame's input. */
    p->progress = -1;
    switch (NXUPDATE_InstallState()) {
    case NXUPDATE_INSTALL_IDLE:
        p->title = "Update available";
        snprintf(p->text, sizeof p->text, "%s", NXUPDATE_Message());
        p->hint = NXUPDATE_CanInstall() ? "A: Update now    B: Later" : "Press A to continue";
        p->detail = NXUPDATE_CanInstall() ? "" : NXUPDATE_Url();
        break;
    case NXUPDATE_INSTALL_DOWNLOADING:
    case NXUPDATE_INSTALL_EXTRACTING:
        p->title = "Updating...";
        snprintf(p->text, sizeof p->text, "%s %s",
                 NXUPDATE_InstallState() == NXUPDATE_INSTALL_DOWNLOADING ? "Downloading" : "Installing",
                 NXUPDATE_Latest());
        p->hint = NXUPDATE_InstallState() == NXUPDATE_INSTALL_DOWNLOADING ? "B: Cancel" : "";
        p->detail = "";
        p->progress = NXUPDATE_Progress();
        break;
    case NXUPDATE_INSTALL_DONE:
        p->title = "Updated";
        snprintf(p->text, sizeof p->text, "Version %s is installed.", NXUPDATE_Latest());
        p->hint = "A: Back to the Homebrew Menu";
        p->detail = "";
        break;
    case NXUPDATE_INSTALL_FAILED:
        p->title = "Update failed";
        snprintf(p->text, sizeof p->text, "Could not update: %s.", NXUPDATE_InstallError());
        p->hint = "Press A to continue";
        p->detail = "";
        break;
    }
    return result;
}
