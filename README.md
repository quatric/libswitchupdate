# libswitchupdate

Checks GitHub for a newer release of a Switch homebrew app and can install it
in place over the running `.nro`, built on libnx. The Switch counterpart of
[lib3dsupdate](https://github.com/quatric/lib3dsupdate),
[libwiiupdate](https://github.com/quatric/libwiiupdate) and
[libwiiuupdate](https://github.com/quatric/libwiiuupdate).

- Runs on its own thread: brings the network up (nifm + bsd sockets), asks the
  GitHub releases API over HTTPS and compares the newest release tag with the
  running version
- Ships its own small mbedTLS (TLS 1.2, certificate chain and host name
  verified against the bundled roots) and inflate-only miniz. Their symbols are
  renamed inside the archive, so apps that link their own copies are unaffected
- Picks the highest numbered, non-prerelease tag, so rolling tags such as
  `latest` are ignored
- Failures are quiet: no network or no reply just means no notice
- Updates in place: downloads the release's Switch zip (streamed to the SD
  card), whose `switch/<Name>/` folder is unpacked over the app's folder. Every
  file is written under a temporary name first, so a failed download or
  unpack leaves the installed copy untouched

## Build

Requires devkitPro with devkitA64 and libnx.

```sh
export DEVKITPRO=/opt/devkitpro DEVKITA64=/opt/devkitpro/devkitA64
make            # produces lib/libswitchupdate.a
make install    # copies into $DEVKITPRO/portlibs/switch
```

## Usage

```c
#include <nxupdate.h>

NXUPDATE_SetAppPath(argc > 0 ? argv[0] : NULL);   // "sdmc:/switch/MyApp/MyApp.nro"
NXUPDATE_Start("quatric", "MyApp", APP_VERSION);

while (appletMainLoop()) {
    padUpdate(&pad);
    nxupdate_popup pop;
    int st = NXUPDATE_Popup((uint32_t)padGetButtonsDown(&pad), &pop);
    if (st == NXUPDATE_POPUP_EXIT) break;           // installed: back to the Homebrew Menu
    if (st == NXUPDATE_POPUP_OPEN) {
        draw_box(pop.title, pop.text, pop.detail, pop.hint);
        if (pop.progress >= 0) draw_progress(pop.progress);
    }
}
```

Or drive the state machine yourself with `NXUPDATE_TakeNotice()`,
`NXUPDATE_Install()`, `NXUPDATE_InstallState()`, `NXUPDATE_Progress()` and
`NXUPDATE_Cancel()`; see `include/nxupdate.h`. `A` and `B` are
`HidNpadButton_A` / `HidNpadButton_B` (bits 0 and 1).

Link `-lswitchupdate` ahead of `-lnx`.

### Which asset is installed

By file name: a zip named for the Switch (`switch` in the name), then a
`.nro`, then a zip that names no other console. Zips or files naming `wii`,
`wup` or `3ds`, and `.cia`/`.wuhb`/`.dol`, are never installed, so one release
can carry every console's build.

The target folder is the folder of `argv[0]`. Without a path on the SD card
the update goes to `sdmc:/switch/<Name>/`. Files in the app folder that the
zip does not contain, such as saved settings, are left alone, and the `.nro`
is written under the running file's name, so a renamed copy is updated in
place.

## Tests

`make -f tests/Makefile.host && ./build/host_test` runs the version, asset
selection and unpack tests on a desktop. `build/host_check <owner> <repo>
<version>` runs the full network path against GitHub.
