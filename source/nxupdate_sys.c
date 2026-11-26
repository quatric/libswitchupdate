/* The Switch side (libnx): threads, time, the bsd socket service through
 * nifm, and entropy from the csrng service. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <switch.h>
#include <mbedtls/platform_time.h>
#include "nxupdate_internal.h"

#define STACK_SIZE (128 * 1024)

/* ── Threads and time ────────────────────────────────────────────────────── */

int nxupdate_sys_thread(void *(*fn)(void *))
{
    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, STACK_SIZE);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int r = pthread_create(&t, &attr, fn, NULL);
    pthread_attr_destroy(&attr);
    return r == 0 ? 0 : -1;
}

unsigned long long nxupdate_sys_ms(void) { return armTicksToNs(armGetSystemTick()) / 1000000ULL; }
void nxupdate_sys_sleep_ms(int ms) { svcSleepThread((s64)ms * 1000000); }

mbedtls_ms_time_t mbedtls_ms_time(void) { return (mbedtls_ms_time_t)nxupdate_sys_ms(); }

/* ── Network ─────────────────────────────────────────────────────────────── */

/* socketInitialize and nifmInitialize are reference counted in libnx, so the
 * library takes its own reference and drops it again when it is done; an app
 * that keeps its own sockets up is unaffected. */
static int net_ref;

int nxupdate_sys_net_up(volatile int *cancel)
{
    if (*cancel) return -1;
    if (net_ref) return 0;
    if (R_FAILED(nifmInitialize(NifmServiceType_User))) return -1;
    NifmInternetConnectionStatus st;
    NifmInternetConnectionType type;
    u32 strength = 0;
    int ok = R_SUCCEEDED(nifmGetInternetConnectionStatus(&type, &strength, &st)) &&
             st == NifmInternetConnectionStatus_Connected;
    if (!ok || R_FAILED(socketInitializeDefault())) {
        nifmExit();
        return -1;
    }
    net_ref = 1;
    return 0;
}

void nxupdate_sys_net_down(void)
{
    if (!net_ref) return;
    socketExit();
    nifmExit();
    net_ref = 0;
}

/* ── Entropy ─────────────────────────────────────────────────────────────── */

int nxupdate_sys_entropy(void *data, unsigned char *out, size_t len, size_t *olen)
{
    (void)data;
    if (R_FAILED(csrngInitialize())) return -1;
    Result r = csrngGetRandomBytes(out, len);
    csrngExit();
    if (R_FAILED(r)) return -1;
    *olen = len;
    return 0;
}

/* ── Files ───────────────────────────────────────────────────────────────── */

int nxupdate_sys_release_file(const char *path)
{
    (void)path; /* a running .nro is loaded into memory; nothing holds it */
    return -1;
}

const char *nxupdate_sys_sd_root(void) { return "sdmc:/"; }
