/* Fort Firewall Process Tree */

#include "fortps.h"

#include "fortcb.h"
#include "fortdev.h"
#include "forttrace.h"
#include "fortutl.h"

#define FORT_PSTREE_POOL_TAG 'PwfF'

#define FORT_SVCHOST_PREFIX L"\\svchost\\"
#define FORT_SVCHOST_EXE    L"svchost.exe"

#define FORT_PSTREE_NAME_LEN_MAX    (120 * sizeof(WCHAR))
#define FORT_PSTREE_NAMES_POOL_SIZE (4 * 1024)

#define FORT_PSNAME_DATA_OFF offsetof(FORT_PSNAME, data)

typedef struct fort_psname
{
    UINT16 refcount;
    UINT16 size;
    WCHAR data[1];
} FORT_PSNAME, *PFORT_PSNAME;

#define FORT_PSNODE_NAME_INHERIT_CHECKED 0x0001
#define FORT_PSNODE_NAME_INHERIT         0x0002
#define FORT_PSNODE_NAME_INHERITED       0x0004
#define FORT_PSNODE_NAME_CUSTOM          0x0008

/* Synchronize with tommy_hashdyn_node! */
typedef struct fort_psnode
{
    struct fort_psnode *next;
    struct fort_psnode *prev;

    PFORT_PSNAME ps_name; /* tommy_hashdyn_node::data */

    tommy_key_t pid_hash; /* tommy_hashdyn_node::index */

    UINT32 process_id;
    UINT32 parent_process_id;

    UINT16 volatile flags;
} FORT_PSNODE, *PFORT_PSNODE;

#define FORT_COMMAND_PATH_LEN  256
#define FORT_COMMAND_PATH_SIZE (FORT_COMMAND_PATH_LEN * sizeof(WCHAR))

#define FORT_COMMAND_LINE_LEN  512
#define FORT_COMMAND_LINE_SIZE (FORT_COMMAND_LINE_LEN * sizeof(WCHAR))

typedef struct fort_pscommand
{
    WCHAR path[FORT_COMMAND_PATH_LEN];
    WCHAR commandLine[FORT_COMMAND_LINE_LEN];
} FORT_PSCOMMAND, *PFORT_PSCOMMAND;

typedef struct _SYSTEM_PROCESSES
{
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    ULONG Reserved1[6];
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    SIZE_T ProcessId;
    SIZE_T ParentProcessId;
    ULONG HandleCount;
    ULONG SessionId;
} SYSTEM_PROCESSES, *PSYSTEM_PROCESSES;

#if !defined(SystemProcessInformation)
#    define SystemProcessInformation 5
#endif

#if defined(FORT_DRIVER)

typedef struct _PEB_LDR_DATA
{
    BYTE Reserved1[8];
    PVOID Reserved2[3];
    LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    BYTE Reserved1[16];
    PVOID Reserved2[10];
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef VOID(NTAPI *PPS_POST_PROCESS_INIT_ROUTINE)(VOID);

typedef struct _PEB
{
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    PPEB_LDR_DATA Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PVOID Reserved4[3];
    PVOID AtlThunkSListPtr;
    PVOID Reserved5;
    ULONG Reserved6;
    PVOID Reserved7;
    ULONG Reserved8;
    ULONG AtlThunkSListPtr32;
    PVOID Reserved9[45];
    BYTE Reserved10[96];
    PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
    BYTE Reserved11[128];
    PVOID Reserved12[1];
    ULONG SessionId;
} PEB, *PPEB;

NTSTATUS NTAPI ZwQuerySystemInformation(ULONG systemInformationClass, PVOID systemInformation,
        ULONG systemInformationLength, PULONG returnLength);

NTSTATUS NTAPI ZwQueryInformationProcess(HANDLE processHandle, ULONG processInformationClass,
        PVOID processInformation, ULONG processInformationLength, PULONG returnLength);

NTSTATUS NTAPI MmCopyVirtualMemory(PEPROCESS sourceProcess, PVOID sourceAddress,
        PEPROCESS targetProcess, PVOID targetAddress, SIZE_T bufferSize,
        KPROCESSOR_MODE previousMode, PSIZE_T returnSize);

#endif

#define fort_pstree_proc_hash(process_id) tommy_inthash_u32((UINT32) (process_id))

#define fort_pstree_get_proc(ps_tree, index)                                                       \
    ((PFORT_PSNODE) tommy_arrayof_ref(&(ps_tree)->procs, (index)))

static NTSTATUS GetProcessImageName(HANDLE processHandle, PUNICODE_STRING path)
{
    NTSTATUS status;

    const USHORT bufferSize = sizeof(UNICODE_STRING) + FORT_CONF_APP_PATH_MAX * sizeof(WCHAR);
    PUNICODE_STRING bufferPath = fort_mem_alloc(bufferSize, FORT_PSTREE_POOL_TAG);
    if (bufferPath == NULL)
        return STATUS_BUFFER_TOO_SMALL;

    ULONG outLength;
    status = ZwQueryInformationProcess(
            processHandle, ProcessImageFileName, bufferPath, bufferSize, &outLength);
    if (NT_SUCCESS(status)) {
        const USHORT pathLength = bufferPath->Length;

        if (outLength == 0 || pathLength == 0) {
            status = STATUS_OBJECT_NAME_NOT_FOUND;
        } else if (path->MaximumLength < pathLength) {
            status = STATUS_BUFFER_TOO_SMALL;
        } else {
            path->Length = pathLength;
            RtlDowncaseUnicodeString(path, bufferPath, FALSE);
            path->Buffer[pathLength / sizeof(WCHAR)] = L'\0';

            status = STATUS_SUCCESS;
        }
    }

    fort_mem_free(bufferPath, FORT_PSTREE_POOL_TAG);

    return status;
}

static HANDLE OpenProcessById(DWORD processId)
{
    NTSTATUS status;

    PEPROCESS peProcess;
    status = PsLookupProcessByProcessId((HANDLE) (ptrdiff_t) processId, &peProcess);
    if (!NT_SUCCESS(status)) {
        LOG("PsTree: Lookup Process Error: pid=%d %x\n", processId, status);
        return NULL;
    }

    HANDLE processHandle;
    status = ObOpenObjectByPointer(
            peProcess, OBJ_KERNEL_HANDLE, NULL, 0, 0, KernelMode, &processHandle);

    ObDereferenceObject(peProcess);

    if (!NT_SUCCESS(status)) {
        LOG("PsTree: Open Process Object Error: pid=%d %x\n", processId, status);
        return NULL;
    }

    return processHandle;
}

static PWCHAR GetUnicodeStringBuffer(PCUNICODE_STRING string, PRTL_USER_PROCESS_PARAMETERS params)
{
#ifdef _X86_
    if ((PCHAR) string->Buffer < (PCHAR) params) {
        return (PWCHAR) ((PCHAR) params + (DWORD) (ptrdiff_t) string->Buffer);
    }
#else
    UNUSED(params);
#endif

    return string->Buffer;
}

static NTSTATUS ReadProcessMemoryBuffer(
        PEPROCESS process, PVOID processData, PVOID out, USHORT size)
{
    SIZE_T outLength = 0;
    const NTSTATUS status = MmCopyVirtualMemory(
            process, processData, IoGetCurrentProcess(), out, size, KernelMode, &outLength);
    if (!NT_SUCCESS(status))
        return status;

    return (size == outLength) ? STATUS_SUCCESS : STATUS_INVALID_BUFFER_SIZE;
}

static NTSTATUS ReadProcessStringBuffer(
        PEPROCESS process, PVOID processData, PUNICODE_STRING out, USHORT size)
{
    const NTSTATUS status = ReadProcessMemoryBuffer(process, processData, out->Buffer, size);
    if (NT_SUCCESS(status)) {
        out->Buffer[size / sizeof(WCHAR)] = L'\0';
        out->Length = (USHORT) size;
    }
    return status;
}

static NTSTATUS GetCurrentProcessPathArgs(
        PEPROCESS process, PUNICODE_STRING path, PUNICODE_STRING commandLine)
{
    NTSTATUS status;

    PROCESS_BASIC_INFORMATION procBasicInfo;
    status = ZwQueryInformationProcess(ZwCurrentProcess(), ProcessBasicInformation, &procBasicInfo,
            sizeof(PROCESS_BASIC_INFORMATION), NULL);
    if (!NT_SUCCESS(status))
        return status;

    if (procBasicInfo.PebBaseAddress == NULL)
        return STATUS_INVALID_ADDRESS;

    PEB peb;
    status = ReadProcessMemoryBuffer(process, procBasicInfo.PebBaseAddress, &peb, sizeof(PEB));
    if (!NT_SUCCESS(status))
        return status;

    PRTL_USER_PROCESS_PARAMETERS params = peb.ProcessParameters;
    if (params == NULL)
        return STATUS_INVALID_ADDRESS;

    const USHORT pathLength = params->ImagePathName.Length;
    if (pathLength != 0 && pathLength <= path->MaximumLength - sizeof(WCHAR)) {
        PVOID userBuffer = (PVOID) GetUnicodeStringBuffer(&params->ImagePathName, params);
        ReadProcessStringBuffer(process, userBuffer, path, pathLength);
    }

    const USHORT commandLineLength = params->CommandLine.Length;
    if (commandLineLength != 0 && commandLineLength <= commandLine->MaximumLength - sizeof(WCHAR)) {
        PVOID userBuffer = (PVOID) GetUnicodeStringBuffer(&params->CommandLine, params);
        ReadProcessStringBuffer(process, userBuffer, commandLine, commandLineLength);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS GetProcessPathArgs(
        HANDLE processHandle, PUNICODE_STRING path, PUNICODE_STRING commandLine)
{
    NTSTATUS status;

    PEPROCESS process;
    status = ObReferenceObjectByHandle(
            processHandle, 0, *PsProcessType, KernelMode, (PVOID *) &process, NULL);
    if (!NT_SUCCESS(status))
        return status;

    /* Copy info from user-mode process */
    KAPC_STATE apcState;
    KeStackAttachProcess(process, &apcState);
    {
        status = GetCurrentProcessPathArgs(process, path, commandLine);
    }
    KeUnstackDetachProcess(&apcState);

    ObDereferenceObject(process);

    return status;
}

UCHAR fort_pstree_flags_set(PFORT_PSTREE ps_tree, UCHAR flags, BOOL on)
{
    return on ? InterlockedOr8(&ps_tree->flags, flags) : InterlockedAnd8(&ps_tree->flags, ~flags);
}

UCHAR fort_pstree_flags(PFORT_PSTREE ps_tree)
{
    return fort_pstree_flags_set(ps_tree, 0, TRUE);
}

static PFORT_PSNAME fort_pstree_name_new(PFORT_PSTREE ps_tree, UINT16 name_size)
{
    FORT_CHECK_STACK();

    PFORT_PSNAME ps_name = fort_pool_malloc(&ps_tree->pool_list,
            FORT_PSNAME_DATA_OFF + name_size + sizeof(WCHAR)); /* include terminating zero */
    if (ps_name != NULL) {
        ps_name->refcount = 1;
        ps_name->size = name_size;
        ps_name->data[name_size / sizeof(WCHAR)] = L'\0';
    }
    return ps_name;
}

static void fort_pstree_name_del(PFORT_PSTREE ps_tree, PFORT_PSNAME ps_name)
{
    if (ps_name != NULL && --ps_name->refcount == 0) {
        fort_pool_free(&ps_tree->pool_list, ps_name);
    }
}

static BOOL fort_pstree_svchost_path_check(PCUNICODE_STRING path)
{
    const USHORT svchostSize = sizeof(FORT_SVCHOST_EXE) - sizeof(WCHAR); /* skip terminating zero */

    const USHORT pathLength = path->Length;
    const PCHAR pathBuffer = (PCHAR) path->Buffer;

    PCUNICODE_STRING sysDrivePath = fort_system_drive_path();
    PCUNICODE_STRING sys32Path = fort_system32_path();

    const USHORT sys32DrivePrefixSize = 2 * sizeof(WCHAR);
    const USHORT sys32PathSize = sys32Path->Length - sys32DrivePrefixSize;

    /* Check the total path length */
    if (pathLength != sysDrivePath->Length + sys32PathSize + svchostSize)
        return FALSE;

    /* Check the file name */
    if (RtlCompareMemory(pathBuffer + (pathLength - svchostSize), FORT_SVCHOST_EXE, svchostSize)
            != svchostSize)
        return FALSE;

    /* Check the drive */
    if (RtlCompareMemory(pathBuffer, sysDrivePath->Buffer, sysDrivePath->Length)
            != sysDrivePath->Length)
        return FALSE;

    /* Check the path */
    if (RtlCompareMemory(pathBuffer + sysDrivePath->Length,
                (PCHAR) sys32Path->Buffer + sys32DrivePrefixSize, sys32PathSize)
            != sys32PathSize)
        return FALSE;

    return TRUE;
}

static BOOL fort_pstree_svchost_check(
        PCUNICODE_STRING path, PCUNICODE_STRING commandLine, PUNICODE_STRING serviceName)
{
    const USHORT svchostSize = sizeof(FORT_SVCHOST_EXE) - sizeof(WCHAR); /* skip terminating zero */
    const USHORT svchostCount = svchostSize / sizeof(WCHAR);
    const USHORT sys32Size = path->Length - svchostSize;
    const USHORT sys32Count = sys32Size / sizeof(WCHAR);

    PCUNICODE_STRING sys32Path = fort_system32_path();
    if (sys32Size != sys32Path->Length)
        return FALSE;

    if (_wcsnicmp(path->Buffer + sys32Count, FORT_SVCHOST_EXE, svchostCount) != 0)
        return FALSE;

    if (_wcsnicmp(path->Buffer, sys32Path->Buffer, sys32Count) != 0)
        return FALSE;

    PWCHAR argp = wcsstr(commandLine->Buffer, L"-s ");
    if (argp == NULL)
        return FALSE;

    argp += (sizeof(L"-s ") - sizeof(WCHAR)) / sizeof(WCHAR); /* skip terminating zero */

    PCWCHAR endp = wcschr(argp, L' ');
    if (endp == NULL) {
        endp = (PCWCHAR) ((PCHAR) commandLine->Buffer + commandLine->Length);
    }

    const USHORT nameLen = (USHORT) ((PCHAR) endp - (PCHAR) argp);
    if (nameLen >= FORT_PSTREE_NAME_LEN_MAX)
        return FALSE;

    serviceName->Length = nameLen;
    serviceName->MaximumLength = nameLen;
    serviceName->Buffer = argp;

    return TRUE;
}

static PFORT_PSNAME fort_pstree_add_service_name(
        PFORT_PSTREE ps_tree, PCUNICODE_STRING path, PCUNICODE_STRING commandLine)
{
    UNICODE_STRING serviceName;
    if (!fort_pstree_svchost_check(path, commandLine, &serviceName))
        return NULL;

    UNICODE_STRING svchostPrefix;
    RtlInitUnicodeString(&svchostPrefix, FORT_SVCHOST_PREFIX);

    PFORT_PSNAME ps_name = fort_pstree_name_new(ps_tree, svchostPrefix.Length + serviceName.Length);
    if (ps_name != NULL) {
        PCHAR data = (PCHAR) &ps_name->data;
        RtlCopyMemory(data, svchostPrefix.Buffer, svchostPrefix.Length);

        UNICODE_STRING nameString;
        nameString.Length = serviceName.Length;
        nameString.MaximumLength = serviceName.Length;
        nameString.Buffer = (PWSTR) (data + svchostPrefix.Length);
        /* RtlDowncaseUnicodeString() must be called in <DISPATCH level only! */
        fort_ascii_downcase(&nameString, &serviceName);
    }

    return ps_name;
}

static PFORT_PSNODE fort_pstree_proc_new(
        PFORT_PSTREE ps_tree, PFORT_PSNAME ps_name, tommy_key_t pid_hash)
{
    tommy_hashdyn_node *proc_node = tommy_list_tail(&ps_tree->free_procs);

    if (proc_node != NULL) {
        tommy_list_remove_existing(&ps_tree->free_procs, proc_node);
    } else {
        tommy_arrayof *procs = &ps_tree->procs;
        const UINT16 index = ps_tree->procs_n;

        tommy_arrayof_grow(procs, index + 1);

        proc_node = tommy_arrayof_ref(procs, index);
    }

    tommy_hashdyn_insert(&ps_tree->procs_map, proc_node, ps_name, pid_hash);

    ++ps_tree->procs_n;

    return (PFORT_PSNODE) proc_node;
}

static void fort_pstree_proc_del(PFORT_PSTREE ps_tree, PFORT_PSNODE proc)
{
    --ps_tree->procs_n;

    /* Delete from pool */
    fort_pstree_name_del(ps_tree, proc->ps_name);

    proc->ps_name = NULL;
    proc->process_id = 0;

    /* Delete from procs map */
    tommy_hashdyn_remove_existing(&ps_tree->procs_map, (tommy_hashdyn_node *) proc);

    tommy_list_insert_tail_check(&ps_tree->free_procs, (tommy_node *) proc);
}

static PFORT_PSNODE fort_pstree_find_proc_hash(
        PFORT_PSTREE ps_tree, DWORD processId, tommy_key_t pid_hash)
{
    PFORT_PSNODE node = (PFORT_PSNODE) tommy_hashdyn_bucket(&ps_tree->procs_map, pid_hash);

    while (node != NULL) {
        if (node->process_id == processId)
            return node;

        node = node->next;
    }

    return NULL;
}

static PFORT_PSNODE fort_pstree_find_proc(PFORT_PSTREE ps_tree, DWORD processId)
{
    if (processId == 0)
        return NULL;

    const tommy_key_t pid_hash = fort_pstree_proc_hash(processId);

    return fort_pstree_find_proc_hash(ps_tree, processId, pid_hash);
}

static void fort_pstree_set_proc_inherited(PFORT_PSNODE proc, PFORT_PSNODE parent)
{
    if ((parent->flags & (FORT_PSNODE_NAME_INHERIT | FORT_PSNODE_NAME_INHERITED)) == 0)
        return;

    PFORT_PSNAME ps_name = parent->ps_name;
    assert(ps_name != NULL);

    ++ps_name->refcount;
    proc->ps_name = ps_name;
    proc->flags |= FORT_PSNODE_NAME_INHERIT_CHECKED | FORT_PSNODE_NAME_INHERITED;
}

static void fort_pstree_check_proc_conf_exe(
        PFORT_PSTREE ps_tree, PFORT_CONF_REF conf_ref, PFORT_PSNODE proc, PCUNICODE_STRING path)
{
    const PFORT_CONF conf = &conf_ref->conf;
    const FORT_APP_FLAGS app_flags = conf->flags.group_apply_child
            ? fort_conf_app_find(conf, path->Buffer, path->Length, fort_conf_exe_find)
            : fort_conf_exe_find(conf, path->Buffer, path->Length);

    if (app_flags.apply_child) {
        PFORT_PSNAME ps_name = fort_pstree_name_new(ps_tree, path->Length);
        if (ps_name != NULL) {
            RtlCopyMemory(ps_name->data, path->Buffer, path->Length);

            proc->ps_name = ps_name;
            proc->flags |= FORT_PSNODE_NAME_INHERIT;
        }
    }
}

static void fort_pstree_check_proc_conf(PFORT_PSTREE ps_tree, PFORT_CONF_REF conf_ref,
        PFORT_PSNODE proc, HANDLE processHandle, DWORD process_id)
{
    NTSTATUS status;

    const USHORT bufferSize = FORT_CONF_APP_PATH_MAX * sizeof(WCHAR);
    PWSTR buffer = fort_mem_alloc(bufferSize, FORT_PSTREE_POOL_TAG);
    if (buffer == NULL)
        return;

    UNICODE_STRING path = { .Length = 0, .MaximumLength = bufferSize, .Buffer = buffer };

    /* GetProcessImageName() must be called in PASSIVE level only! */
    status = GetProcessImageName(processHandle, &path);

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);

    if (NT_SUCCESS(status) && proc->process_id == process_id) {
        proc->flags |= FORT_PSNODE_NAME_INHERIT_CHECKED;

        fort_pstree_check_proc_conf_exe(ps_tree, conf_ref, proc, &path);
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    fort_mem_free(buffer, FORT_PSTREE_POOL_TAG);
}

static void fort_pstree_check_proc_parent_inheritance_conf(PFORT_PSTREE ps_tree,
        PFORT_CONF_REF conf_ref, PFORT_PSNODE proc, DWORD process_id, DWORD parent_process_id)
{
    KLOCK_QUEUE_HANDLE lock_queue;

    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);

    PFORT_PSNODE parent = fort_pstree_find_proc(ps_tree, parent_process_id);
    const UCHAR parent_flags = (parent != NULL) ? parent->flags : 0;

    PFORT_PSNAME ps_name = proc->ps_name;

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    if (parent == NULL)
        return;

    if ((parent_flags & FORT_PSNODE_NAME_INHERIT_CHECKED) == 0 && ps_name == NULL) {
        fort_pstree_check_proc_parent_inheritance_conf(
                ps_tree, conf_ref, parent, parent_process_id, parent->parent_process_id);

        const HANDLE parentHandle = OpenProcessById(parent_process_id);
        if (parentHandle != NULL) {
            fort_pstree_check_proc_conf(ps_tree, conf_ref, parent, parentHandle, parent_process_id);

            ZwClose(parentHandle);
        }
    }

    /* Set process inheritance by parent */
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);

    if (proc->process_id == process_id && parent->process_id == parent_process_id) {
        fort_pstree_set_proc_inherited(proc, parent);
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

inline static void fort_pstree_check_proc_inheritance(PFORT_PSTREE ps_tree, PFORT_PSNODE proc)
{
    PFORT_DEVICE_CONF device_conf = &fort_device()->conf;
    PFORT_CONF_REF conf_ref = fort_conf_ref_take(device_conf);
    if (conf_ref == NULL)
        return;

    fort_pstree_check_proc_parent_inheritance_conf(
            ps_tree, conf_ref, proc, proc->process_id, proc->parent_process_id);

    fort_conf_ref_put(device_conf, conf_ref);
}

static PFORT_PSNODE fort_pstree_handle_new_proc(PFORT_PSTREE ps_tree, PCUNICODE_STRING path,
        PCUNICODE_STRING commandLine, tommy_key_t pid_hash, DWORD processId, DWORD parentProcessId)
{
    FORT_CHECK_STACK();

    PFORT_PSNAME ps_name = fort_pstree_add_service_name(ps_tree, path, commandLine);

    PFORT_PSNODE proc = fort_pstree_proc_new(ps_tree, ps_name, pid_hash);
    if (proc == NULL) {
        fort_pstree_name_del(ps_tree, ps_name);
        return NULL;
    }

    proc->process_id = processId;
    proc->parent_process_id = parentProcessId;

    /* Service can't inherit parent's name */
    proc->flags = (ps_name != NULL) ? FORT_PSNODE_NAME_CUSTOM : 0;

    return proc;
}

inline static BOOL fort_pstree_wait_enum_processes(PFORT_PSTREE ps_tree)
{
    const UCHAR flags =
            (fort_pstree_flags(ps_tree) & (FORT_PSTREE_ENUM_STARTED | FORT_PSTREE_ENUM_DONE));
    if (flags == 0)
        return FALSE;

    if (flags == FORT_PSTREE_ENUM_STARTED) {
        KeWaitForSingleObject(&ps_tree->enum_event, Executive, KernelMode, FALSE, NULL);
    }

    return TRUE;
}

inline static PFORT_PSNODE fort_pstree_notify_process(PFORT_PSTREE ps_tree, PEPROCESS process,
        HANDLE processHandle, PPS_CREATE_NOTIFY_INFO createInfo)
{
    const DWORD processId = (DWORD) (ptrdiff_t) processHandle;

    const tommy_key_t pid_hash = fort_pstree_proc_hash(processId);

#ifdef FORT_DEBUG
    if (createInfo == NULL) {
        LOG("PsTree: CLOSED pid=%d\n", processId);
    } else {
        const DWORD parentProcessId = (DWORD) (ptrdiff_t) createInfo->ParentProcessId;

        LOG("PsTree: NEW pid=%d ppid=%d IMG=[%wZ] CMD=[%wZ]\n", processId, parentProcessId,
                createInfo->ImageFileName, createInfo->CommandLine);
    }
#endif

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);

    PFORT_PSNODE proc = fort_pstree_find_proc_hash(ps_tree, processId, pid_hash);

    if (createInfo == NULL) {
        /* Process Closed */
        if (proc != NULL) {
            fort_pstree_proc_del(ps_tree, proc);

            proc = NULL;
        }
    }
    /* Process Created */
    else if (createInfo->ImageFileName != NULL && createInfo->CommandLine != NULL) {
        if (proc == NULL) {
            const DWORD parentProcessId = (DWORD) (ptrdiff_t) createInfo->ParentProcessId;

            UNICODE_STRING path = *createInfo->ImageFileName;
            fort_path_prefix_adjust(&path);

            proc = fort_pstree_handle_new_proc(
                    ps_tree, &path, createInfo->CommandLine, pid_hash, processId, parentProcessId);
        } else {
            proc = NULL;
        }
    }

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return proc;
}

static void NTAPI fort_pstree_notify(
        PEPROCESS process, HANDLE processHandle, PPS_CREATE_NOTIFY_INFO createInfo)
{
    UNUSED(process);

    PFORT_PSTREE ps_tree = &fort_device()->ps_tree;

    if (!fort_pstree_wait_enum_processes(ps_tree))
        return;

    PFORT_PSNODE proc = fort_pstree_notify_process(ps_tree, process, processHandle, createInfo);

    /* Check the inheritance */
    if (proc != NULL) {
        fort_pstree_check_proc_inheritance(ps_tree, proc);
    }
}

static void fort_pstree_update(PFORT_PSTREE ps_tree, BOOL active)
{
    const UCHAR flags = fort_pstree_flags_set(ps_tree, FORT_PSTREE_ACTIVE, active);
    const BOOL was_active = (flags & FORT_PSTREE_ACTIVE) != 0;

    if (was_active == active)
        return;

    const NTSTATUS status = PsSetCreateProcessNotifyRoutineEx(
            FORT_CALLBACK(
                    FORT_PSTREE_NOTIFY, PCREATE_PROCESS_NOTIFY_ROUTINE_EX, &fort_pstree_notify),
            /*remove=*/!active);

    if (!NT_SUCCESS(status)) {
        LOG("PsTree: Update Error: %x\n", status);
        TRACE(FORT_PSTREE_UPDATE_ERROR, status, 0, 0);
    }
}

FORT_API void fort_pstree_open(PFORT_PSTREE ps_tree)
{
    fort_pool_list_init(&ps_tree->pool_list);
    fort_pool_init(&ps_tree->pool_list, FORT_PSTREE_NAMES_POOL_SIZE);

    tommy_list_init(&ps_tree->free_procs);

    tommy_arrayof_init(&ps_tree->procs, sizeof(FORT_PSNODE));
    tommy_hashdyn_init(&ps_tree->procs_map);

    KeInitializeEvent(&ps_tree->enum_event, NotificationEvent, FALSE);

    KeInitializeSpinLock(&ps_tree->lock);

    fort_pstree_update(ps_tree, /*active=*/TRUE); /* Start process monitor */
}

FORT_API void fort_pstree_close(PFORT_PSTREE ps_tree)
{
    fort_pstree_update(ps_tree, /*active=*/FALSE); /* Stop process monitor */

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);
    {
        fort_pool_done(&ps_tree->pool_list);

        tommy_arrayof_done(&ps_tree->procs);
        tommy_hashdyn_done(&ps_tree->procs_map);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static NTSTATUS fort_pstree_enum_process(PFORT_PSTREE ps_tree, PSYSTEM_PROCESSES processEntry,
        HANDLE processHandle, PUNICODE_STRING path, PUNICODE_STRING commandLine)
{
    NTSTATUS status;

    status = GetProcessImageName(processHandle, path);
    if (!NT_SUCCESS(status))
        return status;

    if (fort_pstree_svchost_path_check(path)) {
        status = GetProcessPathArgs(processHandle, path, commandLine);
        if (!NT_SUCCESS(status)) {
            LOG("PsTree: Process Args Error: pid=%d %x\n", (DWORD) processEntry->ProcessId, status);
            return status;
        }
    } else {
        path->Length = 0;
    }

    const DWORD processId = (DWORD) processEntry->ProcessId;
    const DWORD parentProcessId = (DWORD) processEntry->ParentProcessId;

    const tommy_key_t pid_hash = fort_pstree_proc_hash(processId);

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);

    fort_pstree_handle_new_proc(ps_tree, path, commandLine, pid_hash, processId, parentProcessId);

    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return STATUS_SUCCESS;
}

static void fort_pstree_enum_processes_loop(
        PFORT_PSTREE ps_tree, PSYSTEM_PROCESSES processEntry, PFORT_PSCOMMAND pathBuffer)
{
    UNICODE_STRING path = {
        .Length = 0, .MaximumLength = FORT_COMMAND_PATH_SIZE, .Buffer = pathBuffer->path
    };
    UNICODE_STRING commandLine = {
        .Length = 0, .MaximumLength = FORT_COMMAND_LINE_SIZE, .Buffer = pathBuffer->commandLine
    };

    for (;;) {
        const DWORD processId = (DWORD) processEntry->ProcessId;
        const DWORD parentProcessId = (DWORD) processEntry->ParentProcessId;

        if (processId == 0 || processId == 4 || parentProcessId == 4) {
            /* skip System (sub)processes */
        } else {
            const HANDLE processHandle = OpenProcessById(processId);
            if (processHandle != NULL) {
                fort_pstree_enum_process(ps_tree, processEntry, processHandle, &path, &commandLine);

                ZwClose(processHandle);
            }
        }

        const ULONG nextEntryOffset = processEntry->NextEntryOffset;
        if (nextEntryOffset == 0)
            break;

        processEntry = (PSYSTEM_PROCESSES) ((PUCHAR) processEntry + nextEntryOffset);
    }
}

FORT_API void NTAPI fort_pstree_enum_processes(PVOID worker)
{
    UNUSED(worker);

    NTSTATUS status;

    ULONG bufferSize;
    status = ZwQuerySystemInformation(SystemProcessInformation, NULL, 0, &bufferSize);
    if (status != STATUS_INFO_LENGTH_MISMATCH)
        return;

    bufferSize *= 3; /* for possibly new created processes/threads */

    PVOID buffer = fort_mem_alloc(bufferSize + sizeof(FORT_PSCOMMAND), FORT_PSTREE_POOL_TAG);
    if (buffer == NULL)
        return;

    PFORT_PSCOMMAND pathBuffer = (PFORT_PSCOMMAND) ((PCHAR) buffer + bufferSize);

    PFORT_PSTREE ps_tree = &fort_device()->ps_tree;

    fort_pstree_flags_set(ps_tree, FORT_PSTREE_ENUM_STARTED, TRUE);

    status = ZwQuerySystemInformation(SystemProcessInformation, buffer, bufferSize, &bufferSize);
    if (NT_SUCCESS(status)) {
        fort_pstree_enum_processes_loop(ps_tree, buffer, pathBuffer);
    } else {
        LOG("PsTree: Enum Processes Error: %x\n", status);
        TRACE(FORT_PSTREE_ENUM_PROCESSES_ERROR, status, 0, 0);
    }

    fort_pstree_flags_set(ps_tree, FORT_PSTREE_ENUM_DONE, TRUE);

    KeSetEvent(&ps_tree->enum_event, 0, FALSE);

    fort_mem_free(buffer, FORT_PSTREE_POOL_TAG);
}

static BOOL fort_pstree_get_proc_name_locked(
        PFORT_PSTREE ps_tree, DWORD processId, PUNICODE_STRING path, BOOL *inherited)
{
    PFORT_PSNODE proc = fort_pstree_find_proc(ps_tree, processId);
    if (proc == NULL)
        return FALSE;

    PFORT_PSNAME ps_name = proc->ps_name;
    if (ps_name == NULL)
        return FALSE;

    const UINT16 procFlags = proc->flags;
    if ((procFlags & (FORT_PSNODE_NAME_INHERIT | FORT_PSNODE_NAME_CUSTOM))
            == FORT_PSNODE_NAME_INHERIT)
        return FALSE;

    path->Length = ps_name->size;
    path->MaximumLength = ps_name->size;
    path->Buffer = ps_name->data;

    *inherited = (procFlags & FORT_PSNODE_NAME_INHERITED) != 0;

    return TRUE;
}

FORT_API BOOL fort_pstree_get_proc_name(
        PFORT_PSTREE ps_tree, DWORD processId, PUNICODE_STRING path, BOOL *inherited)
{
    BOOL res;

    KLOCK_QUEUE_HANDLE lock_queue;
    KeAcquireInStackQueuedSpinLock(&ps_tree->lock, &lock_queue);
    {
        res = fort_pstree_get_proc_name_locked(ps_tree, processId, path, inherited);
    }
    KeReleaseInStackQueuedSpinLock(&lock_queue);

    return res;
}
