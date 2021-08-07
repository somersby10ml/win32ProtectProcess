#include <wdm.h>
#include "hook.h"

// My Driver Name
const WCHAR deviceNameBuffer[] = L"\\Device\\MYDEVICE";
const WCHAR deviceSymLinkBuffer[] = L"\\DosDevices\\MyDevice";


#define SIOCTL_TYPE 40000
#define IOCTL_PROTECT CTL_CODE(SIOCTL_TYPE, 0x800, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_UNPROTECT CTL_CODE(SIOCTL_TYPE, 0x801, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

PDEVICE_OBJECT g_MyDevice; // Global pointer to our device object


NTSTATUS Function_IRP_MJ_CREATE(PDEVICE_OBJECT pDeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DbgPrint("IRP MJ CREATE received.");
    return STATUS_SUCCESS;
}

NTSTATUS Function_IRP_MJ_CLOSE(PDEVICE_OBJECT pDeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DbgPrint("IRP MJ CLOSE received.");
    return STATUS_SUCCESS;
}

NTSTATUS Function_IRP_DEVICE_CONTROL(PDEVICE_OBJECT pDeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    PIO_STACK_LOCATION pIoStackLocation;
    pIoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    
    switch (pIoStackLocation->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_PROTECT:
    {
        PVOID inBuf = Irp->AssociatedIrp.SystemBuffer;
        //unsigned long dwInputSize = pIoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
        //unsigned long dwOutputSize = pIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

        unsigned int* pid = (unsigned int*)inBuf;
        setProcessId(*pid);
        break;
    }
    case IOCTL_UNPROTECT:
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "IOCTL_UNPROTECT");
        setProcessId(0);
        break;
    }

    // Finish the I/O operation by simply completing the packet and returning
    // the same status as in the packet itself.
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

VOID OnUnload(IN PDRIVER_OBJECT pDriverObject)
{
    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, deviceSymLinkBuffer);
    IoDeleteSymbolicLink(&symLink);
    IoDeleteDevice(pDriverObject->DeviceObject);
    unhook();
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath) {
    UNREFERENCED_PARAMETER(pDriverObject);
    UNREFERENCED_PARAMETER(pRegistryPath);

    //DbgPrint("DbgPrint DriverEntry");
    //DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DbgPrintEx DriverEntry\n");

    NTSTATUS ntStatus = 0;
    UNICODE_STRING deviceNameUnicodeString, deviceSymLinkUnicodeString;

    // Normalize name and symbolic link.
    RtlInitUnicodeString(&deviceNameUnicodeString, deviceNameBuffer);
    RtlInitUnicodeString(&deviceSymLinkUnicodeString,deviceSymLinkBuffer);

    // Create the device.
    ntStatus = IoCreateDevice(pDriverObject,
        0, // For driver extension
        &deviceNameUnicodeString,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_UNKNOWN,
        FALSE,
        &g_MyDevice);

    // Create the symbolic link
    ntStatus = IoCreateSymbolicLink(&deviceSymLinkUnicodeString,&deviceNameUnicodeString);

    testHook();

    pDriverObject->DriverUnload = OnUnload;
    pDriverObject->MajorFunction[IRP_MJ_CREATE] = Function_IRP_MJ_CREATE;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = Function_IRP_MJ_CLOSE;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Function_IRP_DEVICE_CONTROL;
    DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, "Driver Load\n");
    return STATUS_SUCCESS;
}
