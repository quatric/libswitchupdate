/* libswitchupdate: check GitHub for a newer release of a Switch homebrew app, and
 * optionally install it over the running .nro (started from the Homebrew Menu).
 *
 * The check runs on its own thread (network bring-up, HTTPS request to the
 * GitHub releases API, version comparison) so it never blocks the app:
 *
 *     NXUPDATE_SetAppPath(argc > 0 ? argv[0] : NULL);
 *     NXUPDATE_Start("quatric", "MyApp", "1.2.0");
 *     ...each frame...
 *     if (NXUPDATE_TakeNotice()) show_popup(NXUPDATE_Message());
 *
 * Failures are silent: no network or no reply just means no notice.
 *
 * Installing: NXUPDATE_Install() and poll NXUPDATE_InstallState() /
 * NXUPDATE_Progress(). The release's zip (or .nro) is
 * unpacked over the app's folder on the SD card. Once it reports
 * NXUPDATE_INSTALL_DONE the app should exit.
 */
#ifndef NXUPDATE_H
#define NXUPDATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NXUPDATE_IDLE = 0,     /* NXUPDATE_Start not called yet */
    NXUPDATE_CHECKING,     /* thread running */
    NXUPDATE_UP_TO_DATE,   /* no release newer than the running version */
    NXUPDATE_AVAILABLE,    /* NXUPDATE_Latest() is newer */
    NXUPDATE_FAILED,       /* no network, TLS or HTTP error, bad reply */
    NXUPDATE_CANCELLED,    /* NXUPDATE_Cancel was called */
} nxupdate_state;

typedef enum {
    NXUPDATE_INSTALL_IDLE = 0,
    NXUPDATE_INSTALL_DOWNLOADING,
    NXUPDATE_INSTALL_EXTRACTING,  /* unpacking the zip */
    NXUPDATE_INSTALL_DONE,        /* the new version is in place */
    NXUPDATE_INSTALL_FAILED,      /* see NXUPDATE_InstallError(); the old copy is untouched */
} nxupdate_install_state;

/* Start a background check of github.com/<owner>/<repo>/releases against
 * current_version (e.g. "1.2.0" or "v1.2.0"). Brings up the soc service for
 * the check if the app has not, and lets go of it afterwards. Returns 0 when
 * the thread started. Strings are copied. */
int NXUPDATE_Start(const char *owner, const char *repo, const char *current_version);

nxupdate_state NXUPDATE_State(void);

/* Block until the check (or install) thread has finished, or timeout_ms
 * passes (<0 waits forever). Apps that call socInit themselves must wait
 * first: libnx counts socketInitialize calls, so both can coexist. */
nxupdate_state NXUPDATE_Wait(int timeout_ms);

/* Stop caring about the result. The thread finishes in the background; its
 * result is discarded and the state becomes NXUPDATE_CANCELLED. During an
 * install this stops the download. */
void NXUPDATE_Cancel(void);

/* Returns 1 exactly once, the first time it is called after a newer release
 * was found. Meant for a GUI popup: poll it every frame. */
int NXUPDATE_TakeNotice(void);

/* Newest release tag found, e.g. "v1.3.0" ("" until known). */
const char *NXUPDATE_Latest(void);
/* The running version as passed to NXUPDATE_Start. */
const char *NXUPDATE_Current(void);
/* "github.com/<owner>/<repo>/releases" */
const char *NXUPDATE_Url(void);
/* A short line for popups, e.g. "Version v1.3.0 is available (you have v1.2.0)." */
const char *NXUPDATE_Message(void);

/* Tell the library where the running .nro lives: argv[0] from main
 * ("sdmc:/switch/MyApp/MyApp.nro" from the Homebrew Menu). Without it, the
 * update goes to sdmc:/switch/<folder named in the zip>/. */
void NXUPDATE_SetAppPath(const char *argv0);

/* 1 when an update was found and its release has an asset this copy can
 * install: a Switch zip or a .nro. */
int NXUPDATE_CanInstall(void);

/* Download and install the update on a background thread. NXUPDATE_Cancel
 * stops the download; unpacking is not interrupted.
 * Returns 0 if started. */
int NXUPDATE_Install(void);

nxupdate_install_state NXUPDATE_InstallState(void);
/* 0-100 over download and unpacking. */
int NXUPDATE_Progress(void);
/* Why the install failed, e.g. "download failed" ("" otherwise). */
const char *NXUPDATE_InstallError(void);
/* The folder the update was unpacked to . */
const char *NXUPDATE_InstallDir(void);

/* Ready-made popup logic for GUI apps. Call once per frame with the HidNpadButton_*
 * bits pressed this frame (padGetButtonsDown(), cast to uint32_t); while it returns OPEN, draw a box
 * from *popup and keep the buttons away from the app. A installs (or closes
 * the popup when there is nothing to install), B closes it or cancels the
 * download. EXIT means the update is installed and the user pressed A: leave
 * the app the way it normally exits. */
enum { NXUPDATE_POPUP_CLOSED = 0, NXUPDATE_POPUP_OPEN, NXUPDATE_POPUP_EXIT };
typedef struct {
    const char *title;   /* "Update available", "Updating...", "Updated", "Update failed" */
    char text[160];      /* e.g. "Version v1.3.0 is available (you have v1.2.0)." */
    const char *detail;  /* a second line (the releases URL) or "" */
    const char *hint;    /* buttons, e.g. "A: Update now    B: Later", or "" */
    int progress;        /* 0-100 while updating (draw a bar), else -1 */
} nxupdate_popup;
int NXUPDATE_Popup(uint32_t keys_down, nxupdate_popup *popup);

/* Compare two version strings numerically ("v1.10" > "1.9.2"). Leading text
 * before the first digit is skipped. Returns <0, 0 or >0. */
int NXUPDATE_CompareVersions(const char *a, const char *b);

#ifdef __cplusplus
}
#endif

#endif
