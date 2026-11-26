/* Host-side test of version comparison, release parsing and zip unpacking:
 *   make -f tests/Makefile.host build/host_test && ./build/host_test [releases.json] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "miniz.h"
#include "nxupdate_internal.h"

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)

static int file_is(const char *path, const char *want)
{
    char got[64] = "";
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fread(got, 1, sizeof got - 1, f);
    fclose(f);
    return !strcmp(got, want);
}

static void make_zip(const char *zip, const char *const *entries)
{
    mz_zip_archive zw;
    memset(&zw, 0, sizeof zw);
    CHECK(mz_zip_writer_init_file(&zw, zip, 0));
    for (; *entries; entries += 2)
        mz_zip_writer_add_mem(&zw, entries[0], entries[1], strlen(entries[1]), MZ_DEFAULT_COMPRESSION);
    mz_zip_writer_finalize_archive(&zw);
    mz_zip_writer_end(&zw);
}

int main(int argc, char **argv)
{
    CHECK(NXUPDATE_CompareVersions("v1.10", "1.9.2") > 0);
    CHECK(NXUPDATE_CompareVersions("v1.0", "1.0.0") == 0);
    CHECK(NXUPDATE_CompareVersions("1.0", "v1.0.1") < 0);
    CHECK(NXUPDATE_CompareVersions("v2.0-beta", "2.0") == 0);

    char tag[48];
    const char *json = "[{\"tag_name\":\"latest\",\"prerelease\":false},"
                       "{\"tag_name\":\"v1.1.0\",\"draft\":false,\"prerelease\":false,\"body\":\"x \\\"tag_name\\\": \\\"v9\\\"\"},"
                       "{\"tag_name\":\"v2.0.0\",\"prerelease\":true},"
                       "{\"tag_name\":\"v1.0.0\",\"prerelease\":false}]";
    CHECK(nxupdate_newest_tag(json, tag, sizeof tag) == 0);
    CHECK(!strcmp(tag, "v1.1.0"));
    CHECK(nxupdate_newest_tag("[]", tag, sizeof tag) < 0);

    /* A release for every console: the Switch zip wins; Wii, Wii U and 3DS files never do. */
    char asset[256];
    const char *rel =
        "[{\"tag_name\":\"v1.3\",\"assets\":["
        "{\"browser_download_url\":\"https://x/quatric/App-Wii/releases/download/v1.3/App-WUP.zip\"},"
        "{\"browser_download_url\":\"https://x/quatric/App-Wii/releases/download/v1.3/App-Wii.zip\"},"
        "{\"browser_download_url\":\"https://x/quatric/App-Wii/releases/download/v1.3/App-3DS.cia\"},"
        "{\"browser_download_url\":\"https://x/quatric/App-Wii/releases/download/v1.3/App.nro\"},"
        "{\"browser_download_url\":\"https://x/quatric/App-Wii/releases/download/v1.3/App-Switch.zip\"},"
        "{\"browser_download_url\":\"https://x/quatric/App-Wii/releases/download/v1.3/App.wuhb\"}],"
        "\"body\":\"\\\"browser_download_url\\\":\\\"https://evil/x-switch.zip\\\"\"},"
        "{\"tag_name\":\"v1.2\",\"assets\":[{\"browser_download_url\":\"https://x/App-Wii.zip\"},"
        "{\"browser_download_url\":\"https://x/App.nro\"}]},"
        "{\"tag_name\":\"v1.0\",\"assets\":[{\"browser_download_url\":\"https://x/App.zip\"}]},"
        "{\"tag_name\":\"v0.9\",\"assets\":[{\"browser_download_url\":\"https://x/App-Wii.zip\"},"
        "{\"browser_download_url\":\"https://x/App-WiiU.zip\"},{\"browser_download_url\":\"https://x/App.dol\"}]}]";
    CHECK(nxupdate_release_asset(rel, "v1.3", asset, sizeof asset) == 0 && strstr(asset, "/App-Switch.zip"));
    CHECK(nxupdate_release_asset(rel, "v1.2", asset, sizeof asset) == 0 && !strcmp(asset, "https://x/App.nro"));
    CHECK(nxupdate_release_asset(rel, "v1.0", asset, sizeof asset) == 0 && !strcmp(asset, "https://x/App.zip"));
    CHECK(nxupdate_release_asset(rel, "v0.9", asset, sizeof asset) < 0);
    CHECK(nxupdate_release_asset(rel, "v2", asset, sizeof asset) < 0);

    /* Unpack: only switch/<Name>/ lands in the app folder, nothing escapes it. */
    const char *zip = "build/test.zip";
    static const char *const entries[] = {
        "README.md", "readme",
        "switch/MyApp/MyApp.nro", "new nro",
        "switch/MyApp/data/level1.bin", "level",
        "switch/MyApp/../../escape.txt", "bad",
        NULL,
    };
    make_zip(zip, entries);
    char name[64], err[128];
    CHECK(nxupdate_zip_app_name(zip, name, sizeof name) == 0 && !strcmp(name, "MyApp"));
    system("rm -rf build/sd && mkdir -p build/sd/switch/MyApp && echo old > build/sd/switch/MyApp/MyApp.nro"
           " && echo keep > build/sd/switch/MyApp/settings.txt");
    CHECK(nxupdate_extract(zip, "build/sd/switch/MyApp", "MyApp.nro", NULL, NULL, err, sizeof err) == 0);
    CHECK(file_is("build/sd/switch/MyApp/MyApp.nro", "new nro"));
    struct stat st;
    CHECK(stat("build/sd/switch/MyApp/data/level1.bin", &st) == 0);
    CHECK(file_is("build/sd/switch/MyApp/settings.txt", "keep\n"));
    CHECK(stat("build/sd/switch/MyApp/README.md", &st) != 0);
    CHECK(stat("build/sd/escape.txt", &st) != 0 && stat("build/escape.txt", &st) != 0);
    CHECK(stat("build/sd/switch/MyApp/MyApp.nro.wutmp", &st) != 0);
    CHECK(nxupdate_extract("build/missing.zip", "build/sd/switch/MyApp", NULL, NULL, NULL, err, sizeof err) < 0);

    /* A copy kept straight in sdmc:/switch under another name is updated under that name. */
    system("rm -rf build/sd && mkdir -p build/sd/switch && echo old > build/sd/switch/Mine.nro");
    CHECK(nxupdate_extract(zip, "build/sd/switch", "Mine.nro", NULL, NULL, err, sizeof err) == 0);
    CHECK(file_is("build/sd/switch/Mine.nro", "new nro"));
    CHECK(stat("build/sd/switch/MyApp.nro", &st) != 0);

    if (argc > 1) {
        FILE *f = fopen(argv[1], "rb");
        static char buf[1 << 20];
        size_t n = fread(buf, 1, sizeof buf - 1, f);
        buf[n] = 0;
        fclose(f);
        printf("%s -> %s\n", argv[1], nxupdate_newest_tag(buf, tag, sizeof tag) ? "(none)" : tag);
        if (!nxupdate_newest_tag(buf, tag, sizeof tag)) {
            if (!nxupdate_release_asset(buf, tag, asset, sizeof asset)) printf("asset: %s\n", asset);
        }
    }
    printf(failures ? "%d failures\n" : "ok\n", failures);
    return failures != 0;
}
