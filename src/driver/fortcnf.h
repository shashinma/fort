#ifndef FORTCNF_H
#define FORTCNF_H

#include "fortdrv.h"

#include "forttds.h"
#include "forttlsf.h"

typedef struct fort_conf_ref
{
    UINT32 volatile refcount;

    tlsf_t tlsf;
    tommy_list pools;
    tommy_list free_nodes;

    tommy_arrayof exe_nodes;
    tommy_hashdyn exe_map;

    EX_SPIN_LOCK conf_lock;

    FORT_CONF conf;
} FORT_CONF_REF, *PFORT_CONF_REF;

#define FORT_DEVICE_PROV_BOOT        0x01
#define FORT_DEVICE_IS_OPENED        0x02
#define FORT_DEVICE_IS_VALIDATED     0x04
#define FORT_DEVICE_POWER_OFF        0x08
#define FORT_DEVICE_FILTER_TRANSPORT 0x10

typedef struct fort_device_conf
{
    UCHAR volatile flags;

    FORT_CONF_FLAGS volatile conf_flags;
    PFORT_CONF_REF volatile ref;
    KSPIN_LOCK ref_lock;

    PFORT_CONF_ZONES zones;
    EX_SPIN_LOCK zones_lock;
} FORT_DEVICE_CONF, *PFORT_DEVICE_CONF;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_device_conf_open(PFORT_DEVICE_CONF device_conf);

FORT_API UCHAR fort_device_flag_set(PFORT_DEVICE_CONF device_conf, UCHAR flag, BOOL on);

FORT_API UCHAR fort_device_flag(PFORT_DEVICE_CONF device_conf, UCHAR flag);

FORT_API FORT_APP_FLAGS fort_conf_exe_find(
        const PFORT_CONF conf, const char *path, UINT32 path_len);

FORT_API NTSTATUS fort_conf_ref_exe_add_path(
        PFORT_CONF_REF conf_ref, const char *path, UINT32 path_len, FORT_APP_FLAGS flags);

FORT_API NTSTATUS fort_conf_ref_exe_add_entry(
        PFORT_CONF_REF conf_ref, const PFORT_APP_ENTRY entry, BOOL locked);

FORT_API void fort_conf_ref_exe_del_entry(PFORT_CONF_REF conf_ref, const PFORT_APP_ENTRY entry);

FORT_API PFORT_CONF_REF fort_conf_ref_new(const PFORT_CONF conf, ULONG len);

FORT_API void fort_conf_ref_put(PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref);

FORT_API PFORT_CONF_REF fort_conf_ref_take(PFORT_DEVICE_CONF device_conf);

FORT_API FORT_CONF_FLAGS fort_conf_ref_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref);

FORT_API FORT_CONF_FLAGS fort_conf_ref_flags_set(
        PFORT_DEVICE_CONF device_conf, const PFORT_CONF_FLAGS conf_flags);

FORT_API BOOL fort_conf_ref_period_update(
        PFORT_DEVICE_CONF device_conf, BOOL force, int *periods_n);

FORT_API PFORT_CONF_ZONES fort_conf_zones_new(PFORT_CONF_ZONES zones, ULONG len);

FORT_API void fort_conf_zones_set(PFORT_DEVICE_CONF device_conf, PFORT_CONF_ZONES zones);

FORT_API void fort_conf_zone_flag_set(
        PFORT_DEVICE_CONF device_conf, PFORT_CONF_ZONE_FLAG zone_flag);

FORT_API BOOL fort_conf_zones_ip_included(
        PFORT_DEVICE_CONF device_conf, UINT32 zones_mask, UINT32 remote_ip);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTCNF_H
