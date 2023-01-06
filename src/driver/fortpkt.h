#ifndef FORTPKT_H
#define FORTPKT_H

#include "fortdrv.h"

#include "common/fortconf.h"
#include "forttmr.h"

#define FORT_PACKET_FLUSH_ALL 0xFFFFFFFF

#define FORT_PACKET_QUEUE_BAD_INDEX ((UINT16) -1)

#define FORT_PACKET_INBOUND 0x01
#define FORT_PACKET_IP6     0x02

typedef struct fort_packet_in
{
    IF_INDEX interfaceIndex;
    IF_INDEX subInterfaceIndex;
} FORT_PACKET_IN, *PFORT_PACKET_IN;

typedef struct fort_packet_out
{
    SCOPE_ID remoteScopeId;
    UINT64 endpointHandle;

    ip_addr_t remoteAddr;
} FORT_PACKET_OUT, *PFORT_PACKET_OUT;

typedef struct fort_packet
{
    struct fort_packet *next;

    LARGE_INTEGER latency_start; /* Time it was placed in the latency queue */
    UINT32 data_length; /* Size of the packet (in bytes) */

    UCHAR flags;

    /* Data for re-injection */
    COMPARTMENT_ID compartmentId;
    PNET_BUFFER_LIST netBufList;
    union {
        FORT_PACKET_IN in;
        FORT_PACKET_OUT out;
    };
} FORT_PACKET, *PFORT_PACKET;

typedef struct fort_packet_list
{
    PFORT_PACKET packet_head;
    PFORT_PACKET packet_tail;
} FORT_PACKET_LIST, *PFORT_PACKET_LIST;

typedef struct fort_packet_queue
{
    /* All packets are first buffered into the bandwidth queue and released
     * at the appropriate rate for the configured bandwidth into the latency queue.
     * When they are added to the latency queue they are timestamped when they
     * entered and they are released when the appropriate latency has expired.
     * Only the bandwidth queue is affected by the queue buffer size.
     * The latency queue has no limit.
     */
    FORT_PACKET_LIST bandwidth_list;
    FORT_PACKET_LIST latency_list;

    FORT_SPEED_LIMIT limit;

    UINT64 queued_bytes; /* accumulated size of queued packets */
    UINT64 available_bytes; /* accumulated bytes available for sending */
    LARGE_INTEGER last_tick; /* last time the queue was checked */

    KSPIN_LOCK lock;
} FORT_PACKET_QUEUE, *PFORT_PACKET_QUEUE;

typedef struct fort_shaper
{
    UINT32 limit_io_bits;

    LONG volatile group_io_bits;
    LONG volatile active_io_bits;

    HANDLE injection_transport4_id;
    HANDLE injection_transport6_id;

    FORT_TIMER timer;

    PFORT_PACKET_QUEUE queues[FORT_CONF_GROUP_MAX * 2]; /* in/out-bound pairs */

    KSPIN_LOCK lock;
} FORT_SHAPER, *PFORT_SHAPER;

#if defined(__cplusplus)
extern "C" {
#endif

FORT_API void fort_shaper_open(PFORT_SHAPER shaper);

FORT_API void fort_shaper_close(PFORT_SHAPER shaper);

FORT_API void fort_shaper_conf_update(PFORT_SHAPER shaper, const PFORT_CONF_IO conf_io);

FORT_API void fort_shaper_conf_flags_update(PFORT_SHAPER shaper, const PFORT_CONF_FLAGS conf_flags);

FORT_API BOOL fort_shaper_packet_process(PFORT_SHAPER shaper,
        const FWPS_INCOMING_VALUES0 *inFixedValues,
        const FWPS_INCOMING_METADATA_VALUES0 *inMetaValues, PNET_BUFFER_LIST netBufList,
        UINT64 flowContext);

FORT_API void fort_shaper_flush(PFORT_SHAPER shaper, UINT32 group_io_bits, BOOL drop);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FORTPKT_H
