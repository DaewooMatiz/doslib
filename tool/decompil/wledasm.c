/* NOTES:
 *
 * This isn't documented anywhere, but in some VXDs that have a RCODE segment
 * (16-bit real-mode initialization code) the LE entry point fields point into
 * that 16-bit code segment. That's how you find the entry point of the 16-bit
 * real-mode code of a VXD.
 */

#include "minx86dec/types.h"
#include "minx86dec/state.h"
#include "minx86dec/opcodes.h"
#include "minx86dec/coreall.h"
#include "minx86dec/opcodes_str.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stddef.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include <hw/dos/exehdr.h>

const char *vxd_device_to_name(const uint16_t id) {
    switch (id) {
        case 0x0000:    return "Undefined";             // Undefined_Device_ID             EQU     00000h
        case 0x0001:    return "VMM";                   // VMM_Device_ID                   EQU     00001h  ; Used for dynalink table
        case 0x0002:    return "Debug";                 // Debug_Device_ID                 EQU     00002h
        case 0x0003:    return "VPICD";                 // VPICD_Device_ID                 EQU     00003h
        case 0x0004:    return "VDMAD";                 // VDMAD_Device_ID                 EQU     00004h
        case 0x0005:    return "VTD";                   // VTD_Device_ID                   EQU     00005h
        case 0x0006:    return "V86MMGR";               // V86MMGR_Device_ID               EQU     00006h
        case 0x0007:    return "PageSwap";              // PageSwap_Device_ID              EQU     00007h
#if 0
Parity_Device_ID                EQU     00008h
Reboot_Device_ID                EQU     00009h
#endif
        case 0x000A:    return "VDD";                   // VDD_Device_ID                   EQU     0000Ah
#if 0
VSD_Device_ID                   EQU     0000Bh
VMD_Device_ID                   EQU     0000Ch
VKD_Device_ID                   EQU     0000Dh
VCD_Device_ID                   EQU     0000Eh
VPD_Device_ID                   EQU     0000Fh
BlockDev_Device_ID              EQU     00010h
VMCPD_Device_ID                 EQU     00011h
EBIOS_Device_ID                 EQU     00012h
BIOSXlat_Device_ID              EQU     00013h
VNETBIOS_Device_ID              EQU     00014h
#endif
        case 0x0015:    return "DOSMGR";                // DOSMGR_Device_ID                EQU     00015h
#if 0
WINLOAD_Device_ID               EQU     00016h
#endif
        case 0x0017:    return "SHELL";                 // SHELL_Device_ID                 EQU     00017h
#if 0
VMPoll_Device_ID                EQU     00018h
VPROD_Device_ID                 EQU     00019h
DOSNET_Device_ID                EQU     0001Ah
VFD_Device_ID                   EQU     0001Bh
#endif
        case 0x001C:    return "VDD2";                  // VDD2_Device_ID                  EQU     0001Ch  ; Secondary display adapter
#if 0
WINDEBUG_Device_ID              EQU     0001Dh
TSRLoad_Device_ID               EQU     0001Eh  ; TSR instance utility ID
BiosHook_Device_ID              EQU     0001Fh  ; Bios interrupt hooker VxD
Int13_Device_ID                 EQU     00020h
PageFile_Device_ID              EQU     00021h  ; Paging File device
SCSI_Device_ID                  EQU     00022h  ; SCSI device
MCA_POS_Device_ID               EQU     00023h  ; MCA_POS device
SCSIFD_Device_ID                EQU     00024h  ; SCSI FastDisk device
VPEND_Device_ID                 EQU     00025h  ; Pen device
APM_Device_ID                   EQU     00026h  ; Power Management device
VPOWERD_DEVICE_ID       EQU     APM_DEVICE_ID
VXDLDR_DEVICE_ID        EQU     00027H
NDIS_DEVICE_ID  EQU     00028H
BIOS_EXT_DEVICE_ID      EQU     00029H
#endif
        case 0x002A:    return "VWIN32";                // VWIN32_DEVICE_ID        EQU     0002AH
#if 0
VCOMM_DEVICE_ID EQU     0002BH
SPOOLER_DEVICE_ID       EQU     0002CH
WIN32S_DEVICE_ID        EQU     0002DH
DEBUGCMD_DEVICE_ID      EQU     0002EH
CONFIGMG_DEVICE_ID      EQU     00033H
DWCFGMG_DEVICE_ID       EQU     00034H
SCSIPORT_DEVICE_ID      EQU     00035H
VFBACKUP_DEVICE_ID      EQU     00036H
ENABLE_DEVICE_ID        EQU     00037H
VCOND_DEVICE_ID EQU     00038H
ISAPNP_DEVICE_ID        EQU     0003CH
BIOS_DEVICE_ID  EQU     0003DH
IFSMgr_Device_ID        EQU     00040H
VCDFSD_DEVICE_ID        EQU     00041H
MRCI2_DEVICE_ID EQU     00042H
PCI_DEVICE_ID   EQU     00043H
PELOADER_DEVICE_ID      EQU     00044H
EISA_DEVICE_ID  EQU     00045H
DRAGCLI_DEVICE_ID       EQU     00046H
DRAGSRV_DEVICE_ID       EQU     00047H
PERF_DEVICE_ID  EQU     00048H
AWREDIR_DEVICE_ID       EQU     00049H
ETEN_Device_ID  EQU     00060H
CHBIOS_Device_ID        EQU     00061H
VMSGD_Device_ID EQU     00062H
VPPID_Device_ID EQU     00063H
VIME_Device_ID  EQU     00064H
VHBIOSD_Device_ID       EQU     00065H
BASEID_FOR_NAMEBASEDVXD EQU     0f000H
BASEID_FOR_NAMEBASEDVXD_MASK    EQU     0fffH
VMM_INIT_ORDER  EQU     000000000H
DEBUG_INIT_ORDER        EQU     000000000H
DEBUGCMD_INIT_ORDER     EQU     000000000H
PERF_INIT_ORDER EQU     000900000H
APM_INIT_ORDER  EQU     001000000H
VPOWERD_INIT_ORDER      EQU     APM_INIT_ORDER
BIOSHOOK_INIT_ORDER     EQU     006000000H
VPROD_INIT_ORDER        EQU     008000000H
VPICD_INIT_ORDER        EQU     00C000000H
VTD_INIT_ORDER  EQU     014000000H
VXDLDR_INIT_ORDER       EQU     016000000H
ENUMERATOR_INIT_ORDER   EQU     016800000H
ISAPNP_INIT_ORDER       EQU     ENUMERATOR_INIT_ORDER
EISA_INIT_ORDER EQU     ENUMERATOR_INIT_ORDER
PCI_INIT_ORDER  EQU     ENUMERATOR_INIT_ORDER
BIOS_INIT_ORDER EQU     ENUMERATOR_INIT_ORDER+1
CONFIGMG_INIT_ORDER     EQU     ENUMERATOR_INIT_ORDER+0FFFFH
VCDFSD_INIT_ORDER       EQU     016F00000H
IOS_INIT_ORDER  EQU     017000000H
PAGEFILE_INIT_ORDER     EQU     018000000H
PAGESWAP_INIT_ORDER     EQU     01C000000H
PARITY_INIT_ORDER       EQU     020000000H
REBOOT_INIT_ORDER       EQU     024000000H
EBIOS_INIT_ORDER        EQU     026000000H
VDD_INIT_ORDER  EQU     028000000H
VSD_INIT_ORDER  EQU     02C000000H
VCD_INIT_ORDER  EQU     030000000H
COMMDRVR_INIT_ORDER     EQU     (VCD_INIT_ORDER-1)
PRTCL_INIT_ORDER        EQU     (COMMDRVR_INIT_ORDER-2)
MODEM_INIT_ORDER        EQU     (COMMDRVR_INIT_ORDER-3)
PORT_INIT_ORDER EQU     (COMMDRVR_INIT_ORDER-4)
VMD_INIT_ORDER  EQU     034000000H
VKD_INIT_ORDER  EQU     038000000H
VPD_INIT_ORDER  EQU     03C000000H
BLOCKDEV_INIT_ORDER     EQU     040000000H
MCA_POS_INIT_ORDER      EQU     041000000H
SCSIFD_INIT_ORDER       EQU     041400000H
SCSIMASTER_INIT_ORDER   EQU     041800000H
INT13_INIT_ORDER        EQU     042000000H
VMCPD_INIT_ORDER        EQU     048000000H
BIOSXLAT_INIT_ORDER     EQU     050000000H
VNETBIOS_INIT_ORDER     EQU     054000000H
DOSMGR_INIT_ORDER       EQU     058000000H
DOSNET_INIT_ORDER       EQU     05C000000H
WINLOAD_INIT_ORDER      EQU     060000000H
VMPOLL_INIT_ORDER       EQU     064000000H
UNDEFINED_INIT_ORDER    EQU     080000000H
WIN32_INIT_ORDER        EQU     UNDEFINED_INIT_ORDER
VCOND_INIT_ORDER        EQU     UNDEFINED_INIT_ORDER
WINDEBUG_INIT_ORDER     EQU     081000000H
VDMAD_INIT_ORDER        EQU     090000000H
V86MMGR_INIT_ORDER      EQU     0A0000000H
IFSMgr_Init_Order       EQU     10000H+V86MMGR_Init_Order
FSD_Init_Order  EQU     00100H+IFSMgr_Init_Order
VFD_INIT_ORDER  EQU     50000H+IFSMgr_Init_Order
UNDEF_TOUCH_MEM_INIT_ORDER      EQU     0A8000000H
SHELL_INIT_ORDER        EQU     0B0000000H
#endif
        default: break;
    };

    return "";
}

const char *vxd_device_VMM_service_names[] = {
    "Get_VMM_Version", /* 0x0000 */
    "Get_Cur_VM_Handle", /* 0x0001 */
    "Test_Cur_VM_Handle", /* 0x0002 */
    "Get_Sys_VM_Handle", /* 0x0003 */
    "Test_Sys_VM_Handle", /* 0x0004 */
    "Validate_VM_Handle", /* 0x0005 */
    "Get_VMM_Reenter_Count", /* 0x0006 */
    "Begin_Reentrant_Execution", /* 0x0007 */
    "End_Reentrant_Execution", /* 0x0008 */
    "Install_V86_Break_Point", /* 0x0009 */
    "Remove_V86_Break_Point", /* 0x000A */
    "Allocate_V86_Call_Back", /* 0x000B */
    "Allocate_PM_Call_Back", /* 0x000C */
    "Call_When_VM_Returns", /* 0x000D */
    "Schedule_Global_Event", /* 0x000E */
    "Schedule_VM_Event", /* 0x000F */
    "Call_Global_Event", /* 0x0010 */
    "Call_VM_Event", /* 0x0011 */
    "Cancel_Global_Event", /* 0x0012 */
    "Cancel_VM_Event", /* 0x0013 */
    "Call_Priority_VM_Event", /* 0x0014 */
    "Cancel_Priority_VM_Event", /* 0x0015 */
    "Get_NMI_Handler_Addr", /* 0x0016 */
    "Set_NMI_Handler_Addr", /* 0x0017 */
    "Hook_NMI_Event", /* 0x0018 */
    "Call_When_VM_Ints_Enabled", /* 0x0019 */
    "Enable_VM_Ints", /* 0x001A */
    "Disable_VM_Ints", /* 0x001B */
    "Map_Flat", /* 0x001C */
    "Map_Lin_To_VM_Addr", /* 0x001D */
    "Adjust_Exec_Priority", /* 0x001E */
    "Begin_Critical_Section", /* 0x001F */
    "End_Critical_Section", /* 0x0020 */
    "End_Crit_And_Suspend", /* 0x0021 */
    "Claim_Critical_Section", /* 0x0022 */
    "Release_Critical_Section", /* 0x0023 */
    "Call_When_Not_Critical", /* 0x0024 */
    "Create_Semaphore", /* 0x0025 */
    "Destroy_Semaphore", /* 0x0026 */
    "Wait_Semaphore", /* 0x0027 */
    "Signal_Semaphore", /* 0x0028 */
    "Get_Crit_Section_Status", /* 0x0029 */
    "Call_When_Task_Switched", /* 0x002A */
    "Suspend_VM", /* 0x002B */
    "Resume_VM", /* 0x002C */
    "No_Fail_Resume_VM", /* 0x002D */
    "Nuke_VM", /* 0x002E */
    "Crash_Cur_VM", /* 0x002F */
    "Get_Execution_Focus", /* 0x0030 */
    "Set_Execution_Focus", /* 0x0031 */
    "Get_Time_Slice_Priority", /* 0x0032 */
    "Set_Time_Slice_Priority", /* 0x0033 */
    "Get_Time_Slice_Granularity", /* 0x0034 */
    "Set_Time_Slice_Granularity", /* 0x0035 */
    "Get_Time_Slice_Info", /* 0x0036 */
    "Adjust_Execution_Time", /* 0x0037 */
    "Release_Time_Slice", /* 0x0038 */
    "Wake_Up_VM", /* 0x0039 */
    "Call_When_Idle", /* 0x003A */
    "Get_Next_VM_Handle", /* 0x003B */
    "Set_Global_Time_Out", /* 0x003C */
    "Set_VM_Time_Out", /* 0x003D */
    "Cancel_Time_Out", /* 0x003E */
    "Get_System_Time", /* 0x003F */
    "Get_VM_Exec_Time", /* 0x0040 */
    "Hook_V86_Int_Chain", /* 0x0041 */
    "Get_V86_Int_Vector", /* 0x0042 */
    "Set_V86_Int_Vector", /* 0x0043 */
    "Get_PM_Int_Vector", /* 0x0044 */
    "Set_PM_Int_Vector", /* 0x0045 */
    "Simulate_Int", /* 0x0046 */
    "Simulate_Iret", /* 0x0047 */
    "Simulate_Far_Call", /* 0x0048 */
    "Simulate_Far_Jmp", /* 0x0049 */
    "Simulate_Far_Ret", /* 0x004A */
    "Simulate_Far_Ret_N", /* 0x004B */
    "Build_Int_Stack_Frame", /* 0x004C */
    "Simulate_Push", /* 0x004D */
    "Simulate_Pop", /* 0x004E */
    "_HeapAllocate", /* 0x004F */
    "_HeapReAllocate", /* 0x0050 */
    "_HeapFree", /* 0x0051 */
    "_HeapGetSize", /* 0x0052 */
    "_PageAllocate", /* 0x0053 */
    "_PageReAllocate", /* 0x0054 */
    "_PageFree", /* 0x0055 */
    "_PageLock", /* 0x0056 */
    "_PageUnLock", /* 0x0057 */
    "_PageGetSizeAddr", /* 0x0058 */
    "_PageGetAllocInfo", /* 0x0059 */
    "_GetFreePageCount", /* 0x005A */
    "_GetSysPageCount", /* 0x005B */
    "_GetVMPgCount", /* 0x005C */
    "_MapIntoV86", /* 0x005D */
    "_PhysIntoV86", /* 0x005E */
    "_TestGlobalV86Mem", /* 0x005F */
    "_ModifyPageBits", /* 0x0060 */
    "_CopyPageTable", /* 0x0061 */
    "_LinMapIntoV86", /* 0x0062 */
    "_LinPageLock", /* 0x0063 */
    "_LinPageUnLock", /* 0x0064 */
    "_SetResetV86Pageable", /* 0x0065 */
    "_GetV86PageableArray", /* 0x0066 */
    "_PageCheckLinRange", /* 0x0067 */
    "_PageOutDirtyPages", /* 0x0068 */
    "_PageDiscardPages", /* 0x0069 */
    "_GetNulPageHandle", /* 0x006A */
    "_GetFirstV86Page", /* 0x006B */
    "_MapPhysToLinear", /* 0x006C */
    "_GetAppFlatDSAlias", /* 0x006D */
    "_SelectorMapFlat", /* 0x006E */
    "_GetDemandPageInfo", /* 0x006F */
    "_GetSetPageOutCount", /* 0x0070 */
    "Hook_V86_Page", /* 0x0071 */
    "_Assign_Device_V86_Pages", /* 0x0072 */
    "_DeAssign_Device_V86_Pages", /* 0x0073 */
    "_Get_Device_V86_Pages_Array", /* 0x0074 */
    "MMGR_SetNULPageAddr", /* 0x0075 */
    "_Allocate_GDT_Selector", /* 0x0076 */
    "_Free_GDT_Selector", /* 0x0077 */
    "_Allocate_LDT_Selector", /* 0x0078 */
    "_Free_LDT_Selector", /* 0x0079 */
    "_BuildDescriptorDWORDs", /* 0x007A */
    "_GetDescriptor", /* 0x007B */
    "_SetDescriptor", /* 0x007C */
    "_MMGR_Toggle_HMA", /* 0x007D */
    "Get_Fault_Hook_Addrs", /* 0x007E */
    "Hook_V86_Fault", /* 0x007F */
    "Hook_PM_Fault", /* 0x0080 */
    "Hook_VMM_Fault", /* 0x0081 */
    "Begin_Nest_V86_Exec", /* 0x0082 */
    "Begin_Nest_Exec", /* 0x0083 */
    "Exec_Int", /* 0x0084 */
    "Resume_Exec", /* 0x0085 */
    "End_Nest_Exec", /* 0x0086 */
    "Allocate_PM_App_CB_Area", /* 0x0087 */
    "Get_Cur_PM_App_CB", /* 0x0088 */
    "Set_V86_Exec_Mode", /* 0x0089 */
    "Set_PM_Exec_Mode", /* 0x008A */
    "Begin_Use_Locked_PM_Stack", /* 0x008B */
    "End_Use_Locked_PM_Stack", /* 0x008C */
    "Save_Client_State", /* 0x008D */
    "Restore_Client_State", /* 0x008E */
    "Exec_VxD_Int", /* 0x008F */
    "Hook_Device_Service", /* 0x0090 */
    "Hook_Device_V86_API", /* 0x0091 */
    "Hook_Device_PM_API", /* 0x0092 */
    "System_Control", /* 0x0093 */
    "Simulate_IO", /* 0x0094 */
    "Install_Mult_IO_Handlers", /* 0x0095 */
    "Install_IO_Handler", /* 0x0096 */
    "Enable_Global_Trapping", /* 0x0097 */
    "Enable_Local_Trapping", /* 0x0098 */
    "Disable_Global_Trapping", /* 0x0099 */
    "Disable_Local_Trapping", /* 0x009A */
    "List_Create", /* 0x009B */
    "List_Destroy", /* 0x009C */
    "List_Allocate", /* 0x009D */
    "List_Attach", /* 0x009E */
    "List_Attach_Tail", /* 0x009F */
    "List_Insert", /* 0x00A0 */
    "List_Remove", /* 0x00A1 */
    "List_Deallocate", /* 0x00A2 */
    "List_Get_First", /* 0x00A3 */
    "List_Get_Next", /* 0x00A4 */
    "List_Remove_First", /* 0x00A5 */
    "_AddInstanceItem", /* 0x00A6 */
    "_Allocate_Device_CB_Area", /* 0x00A7 */
    "_Allocate_Global_V86_Data_Area", /* 0x00A8 */
    "_Allocate_Temp_V86_Data_Area", /* 0x00A9 */
    "_Free_Temp_V86_Data_Area", /* 0x00AA */
    "Get_Profile_Decimal_Int", /* 0x00AB */
    "Convert_Decimal_String", /* 0x00AC */
    "Get_Profile_Fixed_Point", /* 0x00AD */
    "Convert_Fixed_Point_String", /* 0x00AE */
    "Get_Profile_Hex_Int", /* 0x00AF */
    "Convert_Hex_String", /* 0x00B0 */
    "Get_Profile_Boolean", /* 0x00B1 */
    "Convert_Boolean_String", /* 0x00B2 */
    "Get_Profile_String", /* 0x00B3 */
    "Get_Next_Profile_String", /* 0x00B4 */
    "Get_Environment_String", /* 0x00B5 */
    "Get_Exec_Path", /* 0x00B6 */
    "Get_Config_Directory", /* 0x00B7 */
    "OpenFile", /* 0x00B8 */
    "Get_PSP_Segment", /* 0x00B9 */
    "GetDOSVectors", /* 0x00BA */
    "Get_Machine_Info", /* 0x00BB */
    "GetSet_HMA_Info", /* 0x00BC */
    "Set_System_Exit_Code", /* 0x00BD */
    "Fatal_Error_Handler", /* 0x00BE */
    "Fatal_Memory_Error", /* 0x00BF */
    "Update_System_Clock", /* 0x00C0 */
    "Test_Debug_Installed", /* 0x00C1 */
    "Out_Debug_String", /* 0x00C2 */
    "Out_Debug_Chr", /* 0x00C3 */
    "In_Debug_Chr", /* 0x00C4 */
    "Debug_Convert_Hex_Binary", /* 0x00C5 */
    "Debug_Convert_Hex_Decimal", /* 0x00C6 */
    "Debug_Test_Valid_Handle", /* 0x00C7 */
    "Validate_Client_Ptr", /* 0x00C8 */
    "Test_Reenter", /* 0x00C9 */
    "Queue_Debug_String", /* 0x00CA */
    "Log_Proc_Call", /* 0x00CB */
    "Debug_Test_Cur_VM", /* 0x00CC */
    "Get_PM_Int_Type", /* 0x00CD */
    "Set_PM_Int_Type", /* 0x00CE */
    "Get_Last_Updated_System_Time", /* 0x00CF */
    "Get_Last_Updated_VM_Exec_Time", /* 0x00D0 */
    "Test_DBCS_Lead_Byte", /* 0x00D1 */
    "_AddFreePhysPage", /* 0x00D2 */
    "_PageResetHandlePAddr", /* 0x00D3 */
    "_SetLastV86Page", /* 0x00D4 */
    "_GetLastV86Page", /* 0x00D5 */
    "_MapFreePhysReg", /* 0x00D6 */
    "_UnmapFreePhysReg", /* 0x00D7 */
    "_XchgFreePhysReg", /* 0x00D8 */
    "_SetFreePhysRegCalBk", /* 0x00D9 */
    "Get_Next_Arena", /* 0x00DA */
    "Get_Name_Of_Ugly_TSR", /* 0x00DB */
    "Get_Debug_Options", /* 0x00DC */
    "Set_Physical_HMA_Alias", /* 0x00DD */
    "_GetGlblRng0V86IntBase", /* 0x00DE */
    "_Add_Global_V86_Data_Area", /* 0x00DF */
    "GetSetDetailedVMError", /* 0x00E0 */
    "Is_Debug_Chr", /* 0x00E1 */
    "Clear_Mono_Screen", /* 0x00E2 */
    "Out_Mono_Chr", /* 0x00E3 */
    "Out_Mono_String", /* 0x00E4 */
    "Set_Mono_Cur_Pos", /* 0x00E5 */
    "Get_Mono_Cur_Pos", /* 0x00E6 */
    "Get_Mono_Chr", /* 0x00E7 */
    "Locate_Byte_In_ROM", /* 0x00E8 */
    "Hook_Invalid_Page_Fault", /* 0x00E9 */
    "Unhook_Invalid_Page_Fault", /* 0x00EA */
    "Set_Delete_On_Exit_File", /* 0x00EB */
    "Close_VM", /* 0x00EC */
    "Enable_Touch_1st_Meg", /* 0x00ED */
    "Disable_Touch_1st_Meg", /* 0x00EE */
    "Install_Exception_Handler", /* 0x00EF */
    "Remove_Exception_Handler", /* 0x00F0 */
    "Get_Crit_Status_No_Block", /* 0x00F1 <- end of Windows 3.1 DDK */
    "_GetLastUpdatedThreadExecTime", /* 0x00F2 <- begin Windows 95 DDK */
    "_Trace_Out_Service", /* 0x00F3 */
    "_Debug_Out_Service", /* 0x00F4 */
    "_Debug_Flags_Service", /* 0x00F5 */
    "VMMAddImportModuleName", /* 0x00F6 */
    "VMM_Add_DDB", /* 0x00F7 */
    "VMM_Remove_DDB", /* 0x00F8 */
    "Test_VM_Ints_Enabled", /* 0x00F9 */
    "_BlockOnID", /* 0x00FA */
    "Schedule_Thread_Event", /* 0x00FB */
    "Cancel_Thread_Event", /* 0x00FC */
    "Set_Thread_Time_Out", /* 0x00FD */
    "Set_Async_Time_Out", /* 0x00FE */
    "_AllocateThreadDataSlot", /* 0x00FF */
    "_FreeThreadDataSlot", /* 0x0100 */
    "_CreateMutex", /* 0x0101 */
    "_DestroyMutex", /* 0x0102 */
    "_GetMutexOwner", /* 0x0103 */
    "Call_When_Thread_Switched", /* 0x0104 */
    "VMMCreateThread", /* 0x0105 */
    "_GetThreadExecTime", /* 0x0106 */
    "VMMTerminateThread", /* 0x0107 */
    "Get_Cur_Thread_Handle", /* 0x0108 */
    "Test_Cur_Thread_Handle", /* 0x0109 */
    "Get_Sys_Thread_Handle", /* 0x010A */
    "Test_Sys_Thread_Handle", /* 0x010B */
    "Validate_Thread_Handle", /* 0x010C */
    "Get_Initial_Thread_Handle", /* 0x010D */
    "Test_Initial_Thread_Handle", /* 0x010E */
    "Debug_Test_Valid_Thread_Handle", /* 0x010F */
    "Debug_Test_Cur_Thread", /* 0x0110 */
    "VMM_GetSystemInitState", /* 0x0111 */
    "Cancel_Call_When_Thread_Switched", /* 0x0112 */
    "Get_Next_Thread_Handle", /* 0x0113 */
    "Adjust_Thread_Exec_Priority", /* 0x0114 */
    "_Deallocate_Device_CB_Area", /* 0x0115 */
    "Remove_IO_Handler", /* 0x0116 */
    "Remove_Mult_IO_Handlers", /* 0x0117 */
    "Unhook_V86_Int_Chain", /* 0x0118 */
    "Unhook_V86_Fault", /* 0x0119 */
    "Unhook_PM_Fault", /* 0x011A */
    "Unhook_VMM_Fault", /* 0x011B */
    "Unhook_Device_Service", /* 0x011C */
    "_PageReserve", /* 0x011D */
    "_PageCommit", /* 0x011E */
    "_PageDecommit", /* 0x011F */
    "_PagerRegister", /* 0x0120 */
    "_PagerQuery", /* 0x0121 */
    "_PagerDeregister", /* 0x0122 */
    "_ContextCreate", /* 0x0123 */
    "_ContextDestroy", /* 0x0124 */
    "_PageAttach", /* 0x0125 */
    "_PageFlush", /* 0x0126 */
    "_SignalID", /* 0x0127 */
    "_PageCommitPhys", /* 0x0128 */
    "_Register_Win32_Services", /* 0x0129 */
    "Cancel_Call_When_Not_Critical", /* 0x012A */
    "Cancel_Call_When_Idle", /* 0x012B */
    "Cancel_Call_When_Task_Switched", /* 0x012C */
    "_Debug_Printf_Service", /* 0x012D */
    "_EnterMutex", /* 0x012E */
    "_LeaveMutex", /* 0x012F */
    "Simulate_VM_IO", /* 0x0130 */
    "Signal_Semaphore_No_Switch", /* 0x0131 */
    "_ContextSwitch", /* 0x0132 */
    "_PageModifyPermissions", /* 0x0133 */
    "_PageQuery", /* 0x0134 */
    "_EnterMustComplete", /* 0x0135 */
    "_LeaveMustComplete", /* 0x0136 */
    "_ResumeExecMustComplete", /* 0x0137 */
    "_GetThreadTerminationStatus", /* 0x0138 */
    "_GetInstanceInfo", /* 0x0139 */
    "_ExecIntMustComplete", /* 0x013A */
    "_ExecVxDIntMustComplete", /* 0x013B */
    "Begin_V86_Serialization", /* 0x013C */
    "Unhook_V86_Page", /* 0x013D */
    "VMM_GetVxDLocationList", /* 0x013E */
    "VMM_GetDDBList", /* 0x013F */
    "Unhook_NMI_Event", /* 0x0140 */
    "Get_Instanced_V86_Int_Vector", /* 0x0141 */
    "Get_Set_Real_DOS_PSP", /* 0x0142 */
    "Call_Priority_Thread_Event", /* 0x0143 */
    "Get_System_Time_Address", /* 0x0144 */
    "Get_Crit_Status_Thread", /* 0x0145 */
    "Get_DDB", /* 0x0146 */
    "Directed_Sys_Control", /* 0x0147 */
    "_RegOpenKey", /* 0x0148 */
    "_RegCloseKey", /* 0x0149 */
    "_RegCreateKey", /* 0x014A */
    "_RegDeleteKey", /* 0x014B */
    "_RegEnumKey", /* 0x014C */
    "_RegQueryValue", /* 0x014D */
    "_RegSetValue", /* 0x014E */
    "_RegDeleteValue", /* 0x014F */
    "_RegEnumValue", /* 0x0150 */
    "_RegQueryValueEx", /* 0x0151 */
    "_RegSetValueEx", /* 0x0152 */
    "_CallRing3", /* 0x0153 */
    "Exec_PM_Int", /* 0x0154 */
    "_RegFlushKey", /* 0x0155 */
    "_PageCommitContig", /* 0x0156 */
    "_GetCurrentContext", /* 0x0157 */
    "_LocalizeSprintf", /* 0x0158 */
    "_LocalizeStackSprintf", /* 0x0159 */
    "Call_Restricted_Event", /* 0x015A */
    "Cancel_Restricted_Event", /* 0x015B */
    "Register_PEF_Provider", /* 0x015C */
    "_GetPhysPageInfo", /* 0x015D */
    "_RegQueryInfoKey", /* 0x015E */
    "MemArb_Reserve_Pages", /* 0x015F */
    "Time_Slice_Sys_VM_Idle", /* 0x0160 */
    "Time_Slice_Sleep", /* 0x0161 */
    "Boost_With_Decay", /* 0x0162 */
    "Set_Inversion_Pri", /* 0x0163 */
    "Reset_Inversion_Pri", /* 0x0164 */
    "Release_Inversion_Pri", /* 0x0165 */
    "Get_Thread_Win32_Pri", /* 0x0166 */
    "Set_Thread_Win32_Pri", /* 0x0167 */
    "Set_Thread_Static_Boost", /* 0x0168 */
    "Set_VM_Static_Boost", /* 0x0169 */
    "Release_Inversion_Pri_ID", /* 0x016A */
    "Attach_Thread_To_Group", /* 0x016B */
    "Detach_Thread_From_Group", /* 0x016C */
    "Set_Group_Static_Boost", /* 0x016D */
    "_GetRegistryPath", /* 0x016E */
    "_GetRegistryKey", /* 0x016F */
    "Cleanup_Thread_State", /* 0x0170 */
    "_RegRemapPreDefKey", /* 0x0171 */
    "End_V86_Serialization", /* 0x0172 */
    "_Assert_Range", /* 0x0173 */
    "_Sprintf", /* 0x0174 */
    "_PageChangePager", /* 0x0175 */
    "_RegCreateDynKey", /* 0x0176 */
    "_RegQueryMultipleValues", /* 0x0177 */
    "Boost_Thread_With_VM", /* 0x0178 */
    "Get_Boot_Flags", /* 0x0179 */
    "Set_Boot_Flags", /* 0x017A */
    "_lstrcpyn", /* 0x017B */
    "_lstrlen", /* 0x017C */
    "_lmemcpy", /* 0x017D */
    "_GetVxDName", /* 0x017E */
    "Force_Mutexes_Free", /* 0x017F */
    "Restore_Forced_Mutexes", /* 0x0180 */
    "_AddReclaimableItem", /* 0x0181 */
    "_SetReclaimableItem", /* 0x0182 */
    "_EnumReclaimableItem", /* 0x0183 */
    "Time_Slice_Wake_Sys_VM", /* 0x0184 */
    "VMM_Replace_Global_Environment", /* 0x0185 */
    "Begin_Non_Serial_Nest_V86_Exec", /* 0x0186 */
    "Get_Nest_Exec_Status", /* 0x0187 */
    "Open_Boot_Log", /* 0x0188 */
    "Write_Boot_Log", /* 0x0189 */
    "Close_Boot_Log", /* 0x018A */
    "EnableDisable_Boot_Log", /* 0x018B */
    "_Call_On_My_Stack", /* 0x018C */
    "Get_Inst_V86_Int_Vec_Base", /* 0x018D */
    "_lstrcmpi", /* 0x018E */
    "_strupr", /* 0x018F */
    "Log_Fault_Call_Out", /* 0x0190 */
    "_AtEventTime", /* 0x0191 <- end of Windows 95 DDK */
    "_PageOutPages", /* 0x0192 <- begin Windows 98 DDK */
    "_Call_On_My_Not_Flat_Stack", /* 0x0193 */
    "_LinRegionLock", /* 0x0194 */
    "_LinRegionUnLock", /* 0x0195 */
    "_AttemptingSomethingDangerous", /* 0x0196 */
    "_Vsprintf", /* 0x0197 */
    "_Vsprintfw", /* 0x0198 */
    "Load_FS_Service", /* 0x0199 */
    "Assert_FS_Service", /* 0x019A */
    "_Begin_Preemptable_Code", /* 0x019B */
    "_End_Preemptable_Code", /* 0x019C */
    "_Get_CPUID_Flags", /* 0x019D */
    "_RegisterGARTHandler", /* 0x019E */
    "_GARTReserve", /* 0x019F */
    "_GARTCommit", /* 0x01A0 */
    "_GARTUnCommit", /* 0x01A1 */
    "_GARTFree", /* 0x01A2 */
    "_GARTMemAttributes", /* 0x01A3 */
    "VMMCreateThreadEx", /* 0x01A4 */
    "_FlushCaches", /* 0x01A5 */
    "Set_Thread_Win32_Pri_NoYield", /* 0x01A6 */
    "_FlushMappedCacheBlock", /* 0x01A7 */
    "_ReleaseMappedCacheBlock", /* 0x01A8 */
    "Run_Preemptable_Events", /* 0x01A9 */
    "_MMPreSystemExit", /* 0x01AA */
    "_MMPageFileShutDown", /* 0x01AB */
    "_Set_Global_Time_Out_Ex", /* 0x01AC */
    "Query_Thread_Priority" /* 0x01AD */
};

const char *vxd_device_DEBUG_service_names[] = {
    "DEBUG_Get_Version", /* 0x0000 */
    "DEBUG_Fault", /* 0x0001 */
    "DEBUG_CheckFault", /* 0x0002 */
    "_DEBUG_LoadSyms" /* 0x0003 */
};

const char *vxd_device_VPICD_service_names[] = {
    "VPICD_Get_Version", /* 0x0000 */
    "VPICD_Virtualize_IRQ", /* 0x0001 */
    "VPICD_Set_Int_Request", /* 0x0002 */
    "VPICD_Clear_Int_Request", /* 0x0003 */
    "VPICD_Phys_EOI", /* 0x0004 */
    "VPICD_Get_Complete_Status", /* 0x0005 */
    "VPICD_Get_Status", /* 0x0006 */
    "VPICD_Test_Phys_Request", /* 0x0007 */
    "VPICD_Physically_Mask", /* 0x0008 */
    "VPICD_Physically_Unmask", /* 0x0009 */
    "VPICD_Set_Auto_Masking", /* 0x000A */
    "VPICD_Get_IRQ_Complete_Status", /* 0x000B */
    "VPICD_Convert_Handle_To_IRQ", /* 0x000C */
    "VPICD_Convert_IRQ_To_Int", /* 0x000D */
    "VPICD_Convert_Int_To_IRQ", /* 0x000E */
    "VPICD_Call_When_Hw_Int", /* 0x000F */
    "VPICD_Force_Default_Owner", /* 0x0010 */
    "VPICD_Force_Default_Behavior", /* 0x0011 */
    "VPICD_Auto_Mask_At_Inst_Swap", /* 0x0012 */
    "VPICD_Begin_Inst_Page_Swap", /* 0x0013 */
    "VPICD_End_Inst_Page_Swap", /* 0x0014 <- end Windows 3.1 DDK */
    "VPICD_Virtual_EOI", /* 0x0015 <- begin Windows 95 DDK */
    "VPICD_Get_Virtualization_Count", /* 0x0016 */
    "VPICD_Post_Sys_Critical_Init", /* 0x0017 */
    "VPICD_VM_SlavePIC_Mask_Change" /* 0x0018 */
};

const char *vxd_device_VDMAD_service_names[] = {
    "VDMAD_Get_Version", /* 0x0000 */
    "VDMAD_Virtualize_Channel", /* 0x0001 */
    "VDMAD_Get_Region_Info", /* 0x0002 */
    "VDMAD_Set_Region_Info", /* 0x0003 */
    "VDMAD_Get_Virt_State", /* 0x0004 */
    "VDMAD_Set_Virt_State", /* 0x0005 */
    "VDMAD_Set_Phys_State", /* 0x0006 */
    "VDMAD_Mask_Channel", /* 0x0007 */
    "VDMAD_UnMask_Channel", /* 0x0008 */
    "VDMAD_Lock_DMA_Region", /* 0x0009 */
    "VDMAD_Unlock_DMA_Region", /* 0x000A */
    "VDMAD_Scatter_Lock", /* 0x000B */
    "VDMAD_Scatter_Unlock", /* 0x000C */
    "VDMAD_Reserve_Buffer_Space", /* 0x000D */
    "VDMAD_Request_Buffer", /* 0x000E */
    "VDMAD_Release_Buffer", /* 0x000F */
    "VDMAD_Copy_To_Buffer", /* 0x0010 */
    "VDMAD_Copy_From_Buffer", /* 0x0011 */
    "VDMAD_Default_Handler", /* 0x0012 */
    "VDMAD_Disable_Translation", /* 0x0013 */
    "VDMAD_Enable_Translation", /* 0x0014 */
    "VDMAD_Get_EISA_Adr_Mode", /* 0x0015 */
    "VDMAD_Set_EISA_Adr_Mode", /* 0x0016 */
    "VDMAD_Unlock_DMA_Region_No_Dirty", /* 0x0017 <- end Windows 3.1 DDK */
    "VDMAD_Phys_Mask_Channel", /* 0x0018 <- begin Windows 95 DDK */
    "VDMAD_Phys_Unmask_Channel", /* 0x0019 */
    "VDMAD_Unvirtualize_Channel", /* 0x001A */
    "VDMAD_Set_IO_Address", /* 0x001B */
    "VDMAD_Get_Phys_Count", /* 0x001C */
    "VDMAD_Get_Phys_Status", /* 0x001D */
    "VDMAD_Get_Max_Phys_Page", /* 0x001E */
    "VDMAD_Set_Channel_Callbacks", /* 0x001F */
    "VDMAD_Get_Virt_Count", /* 0x0020 */
    "VDMAD_Set_Virt_Count" /* 0x0021 */
};

const char *vxd_device_VTD_service_names[] = {
    "VTD_Get_Version",                  // 0x0000
    "VTD_Update_System_Clock",
    "VTD_Get_Interrupt_Period",
    "VTD_Begin_Min_Int_Period",
    "VTD_End_Min_Int_Period",           // 0x0004
    "VTD_Disable_Trapping",
    "VTD_Enable_Trapping",
    "VTD_Get_Real_Time",                // <- end of Windows 3.1 DDK
    "VTD_Get_Date_And_Time",            // 0x0008 <- begin Windows 95 DDK
    "VTD_Adjust_VM_Count",
    "VTD_Delay"
};

const char *vxd_device_V86MMGR_service_names[] = {
    "V86MMGR_Get_Version", /* 0x0000 */
    "V86MMGR_Allocate_V86_Pages", /* 0x0001 */
    "V86MMGR_Set_EMS_XMS_Limits", /* 0x0002 */
    "V86MMGR_Get_EMS_XMS_Limits", /* 0x0003 */
    "V86MMGR_Set_Mapping_Info", /* 0x0004 */
    "V86MMGR_Get_Mapping_Info", /* 0x0005 */
    "V86MMGR_Xlat_API", /* 0x0006 */
    "V86MMGR_Load_Client_Ptr", /* 0x0007 */
    "V86MMGR_Allocate_Buffer", /* 0x0008 */
    "V86MMGR_Free_Buffer", /* 0x0009 */
    "V86MMGR_Get_Xlat_Buff_State", /* 0x000A */
    "V86MMGR_Set_Xlat_Buff_State", /* 0x000B */
    "V86MMGR_Get_VM_Flat_Sel", /* 0x000C */
    "V86MMGR_Map_Pages", /* 0x000D */
    "V86MMGR_Free_Page_Map_Region", /* 0x000E */
    "V86MMGR_LocalGlobalReg", /* 0x000F */
    "V86MMGR_GetPgStatus", /* 0x0010 */
    "V86MMGR_SetLocalA20", /* 0x0011 */
    "V86MMGR_ResetBasePages", /* 0x0012 */
    "V86MMGR_SetAvailMapPgs", /* 0x0013 */
    "V86MMGR_NoUMBInitCalls", /* 0x0014 <- end of Windows 3.1 DDK */
    "V86MMGR_Get_EMS_XMS_Avail", /* 0x0015 <- begin Windows 95 DDK */
    "V86MMGR_Toggle_HMA", /* 0x0016 */
    "V86MMGR_Dev_Init", /* 0x0017 */
    "V86MMGR_Alloc_UM_Page" /* 0x0018 */
};

const char *vxd_device_PAGESWAP_service_names[] = {
    "PageSwap_Get_Version", /* 0x0000 */
    "PageSwap_Test_Create", /* 0x0001 invalid in Windows 95 DDK */
    "PageSwap_Create", /* 0x0002 invalid in Windows 95 DDK */
    "PageSwap_Destroy", /* 0x0003 invalid in Windows 95 DDK */
    "PageSwap_In", /* 0x0004 invalid in Windows 95 DDK */
    "PageSwap_Out", /* 0x0005 invalid in Windows 95 DDK */
    "PageSwap_Test_IO_Valid", /* 0x0006 <- end Windows 3.1 DDK */
    "PageSwap_Read_Or_Write", /* 0x0007 <- begin Windows 95 DDK */
    "PageSwap_Grow_File", /* 0x0008 */
    "PageSwap_Init_File" /* 0x0009 */
};

const char *vxd_device_VDD_service_name[] = {
    "VDD_Get_Version", /* 0x0000 */
    "VDD_PIF_State", /* 0x0001 */
    "VDD_Get_GrabRtn", /* 0x0002 */
    "VDD_Hide_Cursor", /* 0x0003 */
    "VDD_Set_VMType", /* 0x0004 */
    "VDD_Get_ModTime", /* 0x0005 */
    "VDD_Set_HCurTrk", /* 0x0006 */
    "VDD_Msg_ClrScrn", /* 0x0007 */
    "VDD_Msg_ForColor", /* 0x0008 */
    "VDD_Msg_BakColor", /* 0x0009 */
    "VDD_Msg_TextOut", /* 0x000A */
    "VDD_Msg_SetCursPos", /* 0x000B */
    "VDD_Query_Access", /* 0x000C */
    "VDD_Check_Update_Soon", /* 0x000D <- end of Windows 3.1 DDK */
    "VDD_Get_Mini_Dispatch_Table", /* 0x000E <- begin Windows 95 DDK */
    "VDD_Register_Virtual_Port", /* 0x000F */
    "VDD_Get_VM_Info", /* 0x0010 */
    "VDD_Get_Special_VM_IDs", /* 0x0011 */
    "VDD_Register_Extra_Screen_Selector", /* 0x0012 */
    "VDD_Takeover_VGA_Port", /* 0x0013 */
    "VDD_Get_DISPLAYINFO", /* 0x0014 */
    "VDD_Do_Physical_IO", /* 0x0015 */
    "VDD_Set_Sleep_Flag_Addr" /* 0x0016 */
};

const char *vxd_device_SHELL_service_name[] = {
    "SHELL_Get_Version", /* 0x0000 */
    "SHELL_Resolve_Contention", /* 0x0001 */
    "SHELL_Event", /* 0x0002 */
    "SHELL_SYSMODAL_Message", /* 0x0003 */
    "SHELL_Message", /* 0x0004 */
    "SHELL_GetVMInfo", /* 0x0005 <- end of Windows 3.1 DDK */
    "_SHELL_PostMessage", /* 0x0006 <- begin Windows 95 DDK */
    "_SHELL_ShellExecute", /* 0x0007 */
    "_SHELL_PostShellMessage", /* 0x0008 */
    "SHELL_DispatchRing0AppyEvents", /* 0x0009 */
    "SHELL_Hook_Properties", /* 0x000A */
    "SHELL_Unhook_Properties", /* 0x000B */
    "SHELL_Update_User_Activity", /* 0x000C */
    "_SHELL_QueryAppyTimeAvailable", /* 0x000D */
    "_SHELL_CallAtAppyTime", /* 0x000E */
    "_SHELL_CancelAppyTimeEvent", /* 0x000F */
    "_SHELL_BroadcastSystemMessage", /* 0x0010 */
    "_SHELL_HookSystemBroadcast", /* 0x0011 */
    "_SHELL_UnhookSystemBroadcast", /* 0x0012 */
    "_SHELL_LocalAllocEx", /* 0x0013 */
    "_SHELL_LocalFree", /* 0x0014 */
    "_SHELL_LoadLibrary", /* 0x0015 */
    "_SHELL_FreeLibrary", /* 0x0016 */
    "_SHELL_GetProcAddress", /* 0x0017 */
    "_SHELL_CallDll", /* 0x0018 */
    "_SHELL_SuggestSingleMSDOSMode", /* 0x0019 */
    "SHELL_CheckHotkeyAllowed", /* 0x001A */
    "_SHELL_GetDOSAppInfo" /* 0x001B */
};

const char *vxd_device_DOSMGR_service_name[] = {
    "DOSMGR_Get_Version", /* 0x0000 */
    "_DOSMGR_Set_Exec_VM_Data", /* 0x0001 */
    "DOSMGR_Copy_VM_Drive_State", /* 0x0002 */
    "_DOSMGR_Exec_VM", /* 0x0003 */
    "DOSMGR_Get_IndosPtr", /* 0x0004 */
    "DOSMGR_Add_Device", /* 0x0005 */
    "DOSMGR_Remove_Device", /* 0x0006 */
    "DOSMGR_Instance_Device", /* 0x0007 */
    "DOSMGR_Get_DOS_Crit_Status", /* 0x0008 */
    "DOSMGR_Enable_Indos_Polling", /* 0x0009 */
    "DOSMGR_BackFill_Allowed", /* 0x000A */
    "DOSMGR_LocalGlobalReg", /* 0x000B <- end Windows 3.1 DDK */
    "DOSMGR_Init_UMB_Area", /* 0x000C <- begin Windows 95 DDK */
    "DOSMGR_Begin_V86_App", /* 0x000D */
    "DOSMGR_End_V86_App", /* 0x000E */
    "DOSMGR_Alloc_Local_Sys_VM_Mem", /* 0x000F */
    "DOSMGR_Grow_CDSs", /* 0x0010 */
    "DOSMGR_Translate_Server_DOS_Call", /* 0x0011 */
    "DOSMGR_MMGR_PSP_Change_Notifier" /* 0x0012 */
};

const char *vxd_device_VDD2_service_name[] = {
    "VDD2_Get_Version" /* 0x0000 */
};

const char *vxd_device_VWIN32_service_name[] = {
    "VWIN32_Get_Version", /* 0x0000 */
    "VWIN32_DIOCCompletionRoutine", /* 0x0001 */
    "_VWIN32_QueueUserApc", /* 0x0002 */
    "_VWIN32_Get_Thread_Context", /* 0x0003 */
    "_VWIN32_Set_Thread_Context", /* 0x0004 */
    "_VWIN32_CopyMem", /* 0x0005 */
    "_VWIN32_Npx_Exception", /* 0x0006 */
    "_VWIN32_Emulate_Npx", /* 0x0007 */
    "_VWIN32_CheckDelayedNpxTrap", /* 0x0008 */
    "VWIN32_EnterCrstR0", /* 0x0009 */
    "VWIN32_LeaveCrstR0", /* 0x000A */
    "_VWIN32_FaultPopup", /* 0x000B */
    "VWIN32_GetContextHandle", /* 0x000C */
    "VWIN32_GetCurrentProcessHandle", /* 0x000D */
    "_VWIN32_SetWin32Event", /* 0x000E */
    "_VWIN32_PulseWin32Event", /* 0x000F */
    "_VWIN32_ResetWin32Event", /* 0x0010 */
    "_VWIN32_WaitSingleObject", /* 0x0011 */
    "_VWIN32_WaitMultipleObjects", /* 0x0012 */
    "_VWIN32_CreateRing0Thread", /* 0x0013 */
    "_VWIN32_CloseVxDHandle", /* 0x0014 */
    "VWIN32_ActiveTimeBiasSet", /* 0x0015 */
    "VWIN32_GetCurrentDirectory", /* 0x0016 */
    "VWIN32_BlueScreenPopup", /* 0x0017 */
    "VWIN32_TerminateApp", /* 0x0018 */
    "_VWIN32_QueueKernelAPC", /* 0x0019 */
    "VWIN32_SysErrorBox", /* 0x001A */
    "_VWIN32_IsClientWin32", /* 0x001B */
    "VWIN32_IFSRIPWhenLev2Taken" /* 0x001C */
};

const char *vxd_service_to_name(const uint16_t vid,const uint16_t sid) {
#define X(x) (sid < (sizeof(x)/sizeof(x[0]))) ? x[sid] : "";
    switch (vid) {
        case 0x0001: return X(vxd_device_VMM_service_names);    /* VMM */
        case 0x0002: return X(vxd_device_DEBUG_service_names);  /* DEBUG */
        case 0x0003: return X(vxd_device_VPICD_service_names);  /* VPICD */
        case 0x0004: return X(vxd_device_VDMAD_service_names);  /* VDMAD */
        case 0x0005: return X(vxd_device_VTD_service_names);    /* VTD */
        case 0x0006: return X(vxd_device_V86MMGR_service_names);/* V86MMGR */
        case 0x0007: return X(vxd_device_PAGESWAP_service_names);/*PAGESWAP */

        case 0x000A: return X(vxd_device_VDD_service_name);     /* VDD */

        case 0x0015: return X(vxd_device_DOSMGR_service_name);  /* DOSMGR */

        case 0x0017: return X(vxd_device_SHELL_service_name);   /* SHELL */

        case 0x001C: return X(vxd_device_VDD2_service_name);    /* VDD2 */

        case 0x002A: return X(vxd_device_VWIN32_service_name);  /* VWIN32 */

        default: break;
    };
#undef X

    return "";
}

/* re-use a little code from the NE parser. */
#include <hw/dos/exenehdr.h>
#include <hw/dos/exenepar.h>
#include <hw/dos/exelehdr.h>
#include <hw/dos/exelepar.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

struct mod_symbol_table {
    char**                  table;      // ordinal i is entry i - 1
    size_t                  length;
    size_t                  alloc;
};

struct mod_symbols_list {
    struct mod_symbol_table*    table;
    size_t                      length;
};

struct dec_label {
    uint16_t                    seg_v;
    uint32_t                    ofs_v;
    char*                       name;
};

void cstr_free(char **l) {
    if (l != NULL) {
        if (*l != NULL) free(*l);
        *l = NULL;
    }
}

void cstr_copy(char **l,const char *s) {
    cstr_free(l);

    if (s != NULL) {
        const size_t len = strlen(s);
        *l = malloc(len+1);
        if (*l != NULL)
            strcpy(*l,s);
    }
}

void mod_symbol_table_free(struct mod_symbol_table *t) {
    unsigned int i;

    if (t->table) {
        for (i=0;i < t->length;i++) cstr_free(&(t->table[i]));
        free(t->table);
    }

    t->table = NULL;
    t->length = 0;
    t->alloc = 0;
}

void mod_symbols_list_free(struct mod_symbols_list *t) {
    unsigned int i;

    if (t->table) {
        for (i=0;i < t->length;i++) mod_symbol_table_free(&(t->table[i]));
        free(t->table);
    }

    t->table = NULL;
    t->length = 0;
}

void dec_label_set_name(struct dec_label *l,const char *s) {
    cstr_copy(&l->name,s);
}

struct dec_label*               dec_label = NULL;
size_t                          dec_label_count = 0;
size_t                          dec_label_alloc = 0;
unsigned long                   dec_ofs;
uint16_t                        dec_cs;

char                            name_tmp[255+1];

uint8_t                         dec_buffer[512];
uint8_t*                        dec_read;
uint8_t*                        dec_end;
char                            arg_c[101];
struct minx86dec_state          dec_st;
struct minx86dec_instruction    dec_i;
minx86_read_ptr_t               iptr;
uint16_t                        entry_cs,entry_ip;
uint16_t                        start_cs,start_ip;
uint32_t                        start_decom,end_decom,entry_ofs;
uint32_t                        current_offset;
unsigned char                   is_vxd = 0;

struct exe_dos_header           exehdr;

uint32_t                        load_base = 0x00400000;

char*                           sym_file = NULL;
char*                           label_file = NULL;

char*                           src_file = NULL;
int                             src_fd = -1;

void dec_free_labels() {
    unsigned int i=0;

    if (dec_label == NULL)
        return;

    while (i < dec_label_count) {
        struct dec_label *l = dec_label + i;
        cstr_free(&(l->name));
        i++;
    }

    free(dec_label);
    dec_label = NULL;
}

uint32_t current_offset_minus_buffer() {
    return current_offset - (uint32_t)(dec_end - dec_buffer);
}

static void minx86dec_init_state(struct minx86dec_state *st) {
	memset(st,0,sizeof(*st));
}

static void minx86dec_set_buffer(struct minx86dec_state *st,uint8_t *buf,int sz) {
	st->fence = buf + sz;
	st->prefetch_fence = dec_buffer + sizeof(dec_buffer) - 16;
	st->read_ip = buf;
}

void help() {
    fprintf(stderr,"dosdasm [options]\n");
    fprintf(stderr,"MS-DOS COM/EXE/SYS decompiler\n");
    fprintf(stderr,"    -i <file>        File to decompile\n");
    fprintf(stderr,"    -lf <file>       Text file to define labels\n");
    fprintf(stderr,"    -sym <file>      Module symbols file\n");
    fprintf(stderr,"    -b <a>           Load base\n");
}

void print_entry_table_locate_name_by_ordinal(const struct exe_ne_header_name_entry_table * const nonresnames,const struct exe_ne_header_name_entry_table *resnames,const unsigned int ordinal) {
    char tmp[255+1];
    unsigned int i;

    if (resnames->table != NULL) {
        for (i=0;i < resnames->length;i++) {
            struct exe_ne_header_name_entry *ent = resnames->table + i;

            if (ne_name_entry_get_ordinal(resnames,ent) == ordinal) {
                ne_name_entry_get_name(tmp,sizeof(tmp),resnames,ent);
                printf(" RESIDENT NAME '%s' ",tmp);
                return;
            }
        }
    }

    if (nonresnames->table != NULL) {
        for (i=0;i < nonresnames->length;i++) {
            struct exe_ne_header_name_entry *ent = nonresnames->table + i;

            if (ne_name_entry_get_ordinal(nonresnames,ent) == ordinal) {
                ne_name_entry_get_name(tmp,sizeof(tmp),nonresnames,ent);
                printf(" NONRESIDENT NAME '%s' ",tmp);
                return;
            }
        }
    }
}

int parse_argv(int argc,char **argv) {
    char *a;
    int i=1;

    while (i < argc) {
        a = argv[i++];

        if (*a == '-') {
            do { a++; } while (*a == '-');

            if (!strcmp(a,"i")) {
                src_file = argv[i++];
                if (src_file == NULL) return 1;
            }
            else if (!strcmp(a,"lf")) {
                label_file = argv[i++];
                if (label_file == NULL) return 1;
            }
            else if (!strcmp(a,"sym")) {
                sym_file = argv[i++];
                if (sym_file == NULL) return 1;
            }
            else if (!strcmp(a,"h") || !strcmp(a,"help")) {
                help();
                return 1;
            }
            else if (!strcmp(a,"b")) {
                a = argv[i++];
                if (a == NULL) return 1;
                load_base = (uint32_t)strtoul(a,NULL,0);
            }
            else {
                fprintf(stderr,"Unknown switch %s\n",a);
                return 1;
            }
        }
        else {
            fprintf(stderr,"Unknown arg %s\n",a);
            return 1;
        }
    }

    if (src_file == NULL) {
        fprintf(stderr,"Must specify -i source file\n");
        return 1;
    }

    return 0;
}

void reset_buffer() {
    current_offset = 0;
    dec_read = dec_end = dec_buffer;
}

int refill(struct le_vmap_trackio * const io,struct le_header_parseinfo * const p) {
    const size_t flush = sizeof(dec_buffer) / 2;
    const size_t padding = 16;
    size_t dlen;

    if (dec_end > dec_read)
        dlen = (size_t)(dec_end - dec_read);
    else
        dlen = 0;

    if (dec_read >= (dec_buffer+flush)) {
        assert((dec_read+dlen) <= (dec_buffer+sizeof(dec_buffer)-padding));
        if (dlen != 0) memmove(dec_buffer,dec_read,dlen);
        dec_read = dec_buffer;
        dec_end = dec_buffer + dlen;
    }
    {
        unsigned char *e = dec_buffer + sizeof(dec_buffer) - padding;

        if (dec_end < e) {
            unsigned long clen = end_decom - current_offset;
            dlen = (size_t)(e - dec_end);
            if ((unsigned long)dlen > clen)
                dlen = (size_t)clen;

            if (dlen != 0) {
                int rd = le_trackio_read(dec_end,dlen,src_fd,io,p);
                if (rd > 0) {
                    dec_end += rd;
                    current_offset += (unsigned long)rd;
                }
            }
        }
    }

    return (dec_read < dec_end);
}

void dec_label_xlate_32flat(struct dec_label *l,struct le_header_parseinfo *p) {
    struct exe_le_header_object_table_entry *objent;

    if (l->seg_v == 0 || l->seg_v > p->le_header.object_table_entries)
        return;
    if (l->seg_v == p->le_object_flat_32bit)
        return;

    /* this transformation is only applied if object is 32-bit */
    objent = p->le_object_table + l->seg_v - 1;
    if (!(objent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_386_BIG_DEFAULT))
        return;

    /* convert to linear address FROM offset value relative to 32-bit address. */
    l->ofs_v += p->le_object_table_loaded_linear[l->seg_v - 1];
    l->ofs_v -= p->le_object_table_loaded_linear[p->le_object_flat_32bit - 1];
    l->seg_v = p->le_object_flat_32bit;
}

/* caller is giving us a label based on the non-relocated seg:off even if it happens to refer to 32-bit flat segment */
void dec_label_xlate_32flat_always(struct dec_label *l,struct le_header_parseinfo *p) {
    if (l->seg_v == p->le_object_flat_32bit) {
        struct exe_le_header_object_table_entry *objent;

        /* this transformation is only applied if object is 32-bit */
        objent = p->le_object_table + l->seg_v - 1;
        if (!(objent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_386_BIG_DEFAULT))
            return;

        /* adjust */
        l->ofs_v += p->le_object_table_loaded_linear[l->seg_v - 1];
    }
    else {
        dec_label_xlate_32flat(l,p);
    }
}

struct dec_label *dec_find_label(const uint16_t so,const uint32_t oo) {
    unsigned int i=0;

    if (dec_label == NULL)
        return NULL;

    while (i < dec_label_count) {
        struct dec_label *l = dec_label + i;

        if (l->seg_v == so && l->ofs_v == oo)
            return l;

        i++;
    }

    return NULL;
}

struct dec_label *dec_label_malloc() {
    if (dec_label == NULL)
        return NULL;

    if (dec_label_count >= dec_label_alloc)
        return NULL;

    return dec_label + (dec_label_count++);
}

int dec_label_qsortcb(const void *a,const void *b) {
    const struct dec_label *as = (const struct dec_label*)a;
    const struct dec_label *bs = (const struct dec_label*)b;

    if (as->seg_v < bs->seg_v)
        return -1;
    else if (as->seg_v > bs->seg_v)
        return 1;

    if (as->ofs_v < bs->ofs_v)
        return -1;
    else if (as->ofs_v > bs->ofs_v)
        return 1;

    return 0;
}

int ne_segment_relocs_table_qsort(const void *a,const void *b) {
    const union exe_ne_header_segment_relocation_entry *as =
        (const union exe_ne_header_segment_relocation_entry *)a;
    const union exe_ne_header_segment_relocation_entry *bs =
        (const union exe_ne_header_segment_relocation_entry *)b;

    if (as->r.seg_offset < bs->r.seg_offset)
        return -1;
    else if (as->r.seg_offset > bs->r.seg_offset)
        return 1;

    return 0;
}

void dec_label_sort() {
    if (dec_label == NULL || dec_label_count == 0)
        return;

    qsort(dec_label,dec_label_count,sizeof(*dec_label),dec_label_qsortcb);
}

struct fixup_tracking_window_ent {
    uint32_t                                    fixup_rec_page;     // page it belongs to
    uint32_t                                    fixup_rec_index;    // index within page record array
    uint32_t                                    linear_address;     // linear memory address that relocation is applied to
};

// table:
//    0 <= read <= length <= alloc
struct fixup_tracking_window {
    struct fixup_tracking_window_ent*           table;
    size_t                                      read;
    size_t                                      length;
    size_t                                      alloc;
};

void fixup_tracking_window_init(struct fixup_tracking_window *w) {
    memset(w,0,sizeof(*w));
}

void fixup_tracking_window_free(struct fixup_tracking_window *w) {
    if (w->table) free(w->table);
    w->table = NULL;
    w->length = 0;
    w->alloc = 0;
    w->read = 0;
}

void fixup_tracking_window_flush(struct fixup_tracking_window *w) {
    size_t mc;

    if (w->table == NULL) return;
    if (w->read == 0) return;
    if (w->read >= w->length) return;

    mc = w->length - w->read;
    memmove(w->table,w->table + w->read,mc * sizeof(struct fixup_tracking_window_ent));
    w->length = mc;
    w->read = 0;
}

void fixup_tracking_window_lazy_flush(struct fixup_tracking_window *w) {
    if (w->read >= (w->alloc / 2))
        fixup_tracking_window_flush(w);
}

void fixup_tracking_window_alloc(struct fixup_tracking_window *w) {
    if (w->table == NULL) {
        w->alloc = 2048;
        w->table = malloc(w->alloc * sizeof(struct fixup_tracking_window_ent));
        if (w->table == NULL) return;
        w->length = 0;
        w->read = 0;
    }
}

struct fixup_tracking_window_ent *fixup_tracking_window_alloc_entry(struct fixup_tracking_window *w) {
    size_t entry;

    if (w->table == NULL) {
        fixup_tracking_window_alloc(w);
        if (w->table == NULL) return NULL;
    }

    if (w->length >= w->alloc) {
        fixup_tracking_window_flush(w);
        if (w->length >= w->alloc) return NULL;
    }
    else {
        fixup_tracking_window_lazy_flush(w);
    }

    entry = w->length++;
    return w->table + entry;
}

int fixup_window_sort(const void *a,const void *b) {
    const struct fixup_tracking_window_ent *fa =
        (const struct fixup_tracking_window_ent *)a;
    const struct fixup_tracking_window_ent *fb =
        (const struct fixup_tracking_window_ent *)b;

    if (fa->linear_address < fb->linear_address)
        return -1;
    if (fa->linear_address > fb->linear_address)
        return 1;

    return 0;;
}

int main(int argc,char **argv) {
    struct fixup_tracking_window fixup_window;
    struct le_header_parseinfo le_parser;
    struct exe_le_header le_header;
    struct le_vmap_trackio io;
    uint32_t le_header_offset;
    struct dec_label *label;
    unsigned int labeli;
    uint32_t file_size;

    fixup_tracking_window_init(&fixup_window);
    assert(sizeof(le_parser.le_header) == EXE_HEADER_LE_HEADER_SIZE);
    le_header_parseinfo_init(&le_parser);
    memset(&exehdr,0,sizeof(exehdr));

    if (parse_argv(argc,argv))
        return 1;

    assert(sizeof(exehdr) == 0x1C);

#if defined(TARGET_MSDOS) && TARGET_MSDOS == 16
    dec_label_alloc = 4096;
#else
    dec_label_alloc = 65536;
#endif
    dec_label_count = 0;
    dec_label = malloc(sizeof(*dec_label) * dec_label_alloc);
    if (dec_label == NULL) {
        fprintf(stderr,"Failed to alloc label array\n");
        return 1;
    }
    memset(dec_label,0,sizeof(*dec_label) * dec_label_alloc);

    if (src_file == NULL) {
        fprintf(stderr,"No source file specified\n");
        return 1;
    }

    src_fd = open(src_file,O_RDONLY|O_BINARY);
    if (src_fd < 0) {
        fprintf(stderr,"Unable to open '%s', %s\n",src_file,strerror(errno));
        return 1;
    }

    file_size = lseek(src_fd,0,SEEK_END);
    lseek(src_fd,0,SEEK_SET);

    if (read(src_fd,&exehdr,sizeof(exehdr)) != (int)sizeof(exehdr)) {
        fprintf(stderr,"EXE header read error\n");
        return 1;
    }

    if (exehdr.magic != 0x5A4DU/*MZ*/) {
        fprintf(stderr,"EXE header signature missing\n");
        return 1;
    }

    if (!exe_header_can_contain_exe_extension(&exehdr)) {
        fprintf(stderr,"EXE header cannot contain extension\n");
        return 1;
    }

    /* go read the extension */
    if (lseek(src_fd,EXE_HEADER_EXTENSION_OFFSET,SEEK_SET) != EXE_HEADER_EXTENSION_OFFSET ||
        read(src_fd,&le_header_offset,4) != 4) {
        fprintf(stderr,"Cannot read extension\n");
        return 1;
    }
    if ((le_header_offset+EXE_HEADER_LE_HEADER_SIZE) >= file_size) {
        printf("! NE header not present (offset out of range)\n");
        return 0;
    }

    /* go read the extended header */
    if (lseek(src_fd,le_header_offset,SEEK_SET) != le_header_offset ||
        read(src_fd,&le_header,sizeof(le_header)) != sizeof(le_header)) {
        fprintf(stderr,"Cannot read LE header\n");
        return 1;
    }
    if (le_header.signature != EXE_LE_SIGNATURE &&
        le_header.signature != EXE_LX_SIGNATURE) {
        fprintf(stderr,"Not an LE/LX executable\n");
        return 1;
    }
    le_parser.le_header_offset = le_header_offset;
    le_parser.load_base = load_base;
    le_parser.le_header = le_header;

    printf("  * Chosen 32-bit flat base:        0x%08lx (for this dump)\n",
            (unsigned long)le_parser.load_base);

    if (le_header.offset_of_object_table != 0 && le_header.object_table_entries != 0) {
        unsigned long ofs = le_header.offset_of_object_table + (unsigned long)le_parser.le_header_offset;
        unsigned char *base = le_header_parseinfo_alloc_object_table(&le_parser);
        size_t readlen = le_header_parseinfo_get_object_table_buffer_size(&le_parser);

        if ((unsigned long)lseek(src_fd,ofs,SEEK_SET) != ofs || (size_t)read(src_fd,base,readlen) != readlen)
            le_header_parseinfo_free_object_table(&le_parser);

        le_header_object_table_loaded_linear_generate(&le_parser);
    }

    if (le_header.object_page_map_offset != 0 && le_header.number_of_memory_pages != 0) {
        unsigned long ofs = le_header.object_page_map_offset + (unsigned long)le_parser.le_header_offset;
        unsigned char *base = le_header_parseinfo_alloc_object_page_map_table(&le_parser);
        size_t readlen = le_header_parseinfo_get_object_page_map_table_read_buffer_size(&le_parser);

        if ((unsigned long)lseek(src_fd,ofs,SEEK_SET) != ofs || (size_t)read(src_fd,base,readlen) != readlen)
            le_header_parseinfo_free_object_page_map_table(&le_parser);

        /* "finish" reading by having the library convert the data in-place */
        if (le_parser.le_object_page_map_table != NULL)
            le_header_parseinfo_finish_read_get_object_page_map_table(&le_parser);
    }

    if (le_header.fixup_page_table_offset != 0 && le_header.number_of_memory_pages != 0) {
        unsigned long ofs = le_header.fixup_page_table_offset + (unsigned long)le_header_offset;
        unsigned char *base = le_header_parseinfo_alloc_fixup_page_table(&le_parser);
        size_t readlen = le_header_parseinfo_get_fixup_page_table_buffer_size(&le_parser);

        if (base != NULL) {
            // NTS: This table has one extra entry, so that you can determine the size of each fixup record entry per segment
            //      by the difference between each entry. Entries in the fixup record table (and therefore the offsets in this
            //      table) numerically increase for this reason.
            if ((unsigned long)lseek(src_fd,ofs,SEEK_SET) != ofs || (size_t)read(src_fd,base,readlen) != (size_t)readlen)
                le_header_parseinfo_free_fixup_page_table(&le_parser);

            le_header_parseinfo_fixup_record_list_setup_prepare_from_page_table(&le_parser);
        }
    }

    if (le_parser.le_fixup_records.table != NULL && le_parser.le_fixup_records.length != 0) {
        struct le_header_fixup_record_table *frtable;
        unsigned char *base;
        unsigned int i;

        for (i=0;i < le_header.number_of_memory_pages;i++) {
            frtable = le_parser.le_fixup_records.table + i;

            if (frtable->file_length == 0) continue;

            base = le_header_fixup_record_table_alloc_raw(frtable,frtable->file_length);
            if (base == NULL) continue;

            if (!((unsigned long)lseek(src_fd,frtable->file_offset,SEEK_SET) == (unsigned long)frtable->file_offset &&
                (unsigned long)read(src_fd,base,frtable->file_length) == (unsigned long)frtable->file_length))
                le_header_fixup_record_table_free_raw(frtable);

            if (frtable->raw != NULL)
                le_header_fixup_record_table_parse(frtable);
        }
    }

    /* load resident name table */
    if (le_header.resident_names_table_offset != (uint32_t)0 && le_header.entry_table_offset != (uint32_t)0 &&
        le_header.resident_names_table_offset < le_header.entry_table_offset &&
        (unsigned long)lseek(src_fd,le_header.resident_names_table_offset + le_header_offset,SEEK_SET) == (le_header.resident_names_table_offset + le_header_offset)) {
        uint32_t sz = le_header.entry_table_offset - le_header.resident_names_table_offset;
        unsigned char *base;

        base = exe_ne_header_name_entry_table_alloc_raw(&le_parser.le_resident_names,sz);
        if (base != NULL) {
            if ((unsigned long)read(src_fd,base,sz) != sz)
                exe_ne_header_name_entry_table_free_raw(&le_parser.le_resident_names);
        }

        exe_ne_header_name_entry_table_parse_raw(&le_parser.le_resident_names);
    }

    /* load nonresident name table */
    if (le_header.nonresident_names_table_offset != (uint32_t)0 &&
        le_header.nonresident_names_table_length != (uint32_t)0 &&
        (unsigned long)lseek(src_fd,le_header.nonresident_names_table_offset,SEEK_SET) == le_header.nonresident_names_table_offset) {
        unsigned char *base;

        base = exe_ne_header_name_entry_table_alloc_raw(&le_parser.le_nonresident_names,le_header.nonresident_names_table_length);
        if (base != NULL) {
            if ((unsigned long)read(src_fd,base,le_header.nonresident_names_table_length) != le_header.nonresident_names_table_length)
                exe_ne_header_name_entry_table_free_raw(&le_parser.le_nonresident_names);
        }

        exe_ne_header_name_entry_table_parse_raw(&le_parser.le_nonresident_names);
    }

    if (le_header.entry_table_offset != (uint32_t)0) {
        unsigned long ofs = le_parser.le_header.entry_table_offset + le_parser.le_header_offset;
        uint32_t readlen = le_exe_header_entry_table_size(&le_parser.le_header);
        unsigned char *base = le_header_entry_table_alloc(&le_parser.le_entry_table,readlen);

        if (base != NULL) {
            // NTS: This table has one extra entry, so that you can determine the size of each fixup record entry per segment
            //      by the difference between each entry. Entries in the fixup record table (and therefore the offsets in this
            //      table) numerically increase for this reason.
            if ((unsigned long)lseek(src_fd,ofs,SEEK_SET) != ofs || (size_t)read(src_fd,base,readlen) != (size_t)readlen)
                le_header_entry_table_free(&le_parser.le_entry_table);
        }

        if (le_parser.le_entry_table.raw != NULL)
            le_header_entry_table_parse(&le_parser.le_entry_table);
    }

    if (le_header.initial_object_cs_number != 0) {
        if ((label=dec_label_malloc()) != NULL) {
            dec_label_set_name(label,"LE entry point");

            label->seg_v =
                le_header.initial_object_cs_number;
            label->ofs_v =
                le_header.initial_eip;

            dec_label_xlate_32flat_always(label,&le_parser);
        }
    }

    if (le_parser.le_entry_table.table != NULL) {
        struct le_header_entry_table_entry *ent;
        unsigned char *raw;
        unsigned int i,mx;

        mx = le_parser.le_entry_table.length;
        for (i=0;i < mx;i++) {
            ent = le_parser.le_entry_table.table + i;
            raw = le_header_entry_table_get_raw_entry(&le_parser.le_entry_table,i); /* parser makes sure there is sufficient space for struct given type */
            if (raw == NULL) continue;

            if (ent->type == 2) {
                uint32_t offset;

                raw++; /*flags = *raw++ */
                offset = *((uint16_t*)raw); raw += 2;
                assert(raw <= (le_parser.le_entry_table.raw+le_parser.le_entry_table.raw_length));

                if ((label=dec_label_malloc()) != NULL) {
                    char tmp[256];

                    sprintf(tmp,"Entry ordinal #%u",i+1);
                    dec_label_set_name(label,tmp);

                    label->seg_v =
                        ent->object;
                    label->ofs_v =
                        offset;

                    dec_label_xlate_32flat(label,&le_parser);
                }
            }
            else if (ent->type == 3) {
                uint32_t offset;

                raw++; /*flags = *raw++ */
                offset = *((uint32_t*)raw); raw += 4;
                assert(raw <= (le_parser.le_entry_table.raw+le_parser.le_entry_table.raw_length));

                if ((label=dec_label_malloc()) != NULL) {
                    char tmp[256];

                    sprintf(tmp,"Entry ordinal #%u",i+1);
                    dec_label_set_name(label,tmp);

                    label->seg_v =
                        ent->object;
                    label->ofs_v =
                        offset;

                    dec_label_xlate_32flat(label,&le_parser);
                }
            }
        }
    }

    {
        /* if this is a Windows VXD, then we want to locate the DDB block for more fun */
        uint16_t object=0;
        uint32_t offset=0;

        if (le_parser_is_windows_vxd(&le_parser,&object,&offset)) {
            struct windows_vxd_ddb_win31 *ddb_31;
            unsigned char ddb[256];
            unsigned int i;
            char tmp[9];
            int rd;

            printf("* This appears to be a 32-bit Windows 386/VXD driver\n");
            printf("    VXD DDB block in Object #%u : 0x%08lx\n",
                (unsigned int)object,(unsigned long)offset);

            label = dec_find_label(object,offset);
            if (label != NULL) {
                label->seg_v = ~0;
                label->ofs_v = ~0;
                dec_label_set_name(label,"VXD DDB entry point");
            }

            if (le_segofs_to_trackio(&io,object,offset,&le_parser)) {
                printf("        File offset %lu (0x%lX) (page #%lu at %lu + page offset 0x%lX / 0x%lX)\n",
                        (unsigned long)io.file_ofs + (unsigned long)io.page_ofs,
                        (unsigned long)io.file_ofs + (unsigned long)io.page_ofs,
                        (unsigned long)io.page_number,
                        (unsigned long)io.file_ofs,
                        (unsigned long)io.page_ofs,
                        (unsigned long)io.page_size);

                // now read it
                rd = le_trackio_read(ddb,sizeof(ddb),src_fd,&io,&le_parser);
                if (rd >= (int)sizeof(*ddb_31)) {
                    ddb_31 = (struct windows_vxd_ddb_win31*)ddb;

                    /* the DDB like anything else within the VXD can be patched by LE fixups.
                     * if we don't do this the DDB will mysteriously show no entry points whatsoever.
                     * NTS: VXDs are loaded into a flat 32-bit address space, so we read as if flat 32-bit */
                    le_parser_apply_fixup(ddb,(size_t)rd,object,offset,&le_parser);

                    printf("        Windows 386/VXD DDB structure (with relocations applied, load base 0x%08lX):\n",
                            (unsigned long)le_parser.load_base);
                    printf("            DDB_Next:               0x%08lX\n",(unsigned long)ddb_31->DDB_Next);
                    printf("            DDB_SDK_Version:        %u.%u (0x%04X)\n",
                            ddb_31->DDB_SDK_Version>>8,
                            ddb_31->DDB_SDK_Version&0xFFU,
                            ddb_31->DDB_SDK_Version);
                    printf("            DDB_Req_Device_Number:  0x%04x\n",ddb_31->DDB_Req_Device_Number);
                    printf("            DDB_Dev_*_Version:      %u.%u\n",ddb_31->DDB_Dev_Major_Version,ddb_31->DDB_Dev_Minor_Version);
                    printf("            DDB_Flags:              0x%04x\n",ddb_31->DDB_Flags);

                    memcpy(tmp,ddb_31->DDB_Name,8); tmp[8] = 0;
                    printf("            DDB_Name:               \"%s\"\n",tmp);

                    printf("            DDB_Init_Order:         0x%08lx\n",(unsigned long)ddb_31->DDB_Init_Order);
                    printf("            DDB_Control_Proc:       0x%08lx\n",(unsigned long)ddb_31->DDB_Control_Proc);
                    printf("            DDB_V86_API_Proc:       0x%08lx\n",(unsigned long)ddb_31->DDB_V86_API_Proc);
                    printf("            DDB_PM_API_Proc:        0x%08lx\n",(unsigned long)ddb_31->DDB_PM_API_Proc);
                    printf("            DDB_V86_API_CSIP:       %04X:%04X\n",
                            (unsigned int)(ddb_31->DDB_V86_API_CSIP >> 16UL),
                            (unsigned int)(ddb_31->DDB_V86_API_CSIP & 0xFFFFUL));
                    printf("            DDB_PM_API_CSIP:        %04X:%04X\n",
                            (unsigned int)(ddb_31->DDB_PM_API_CSIP >> 16UL),
                            (unsigned int)(ddb_31->DDB_PM_API_CSIP & 0xFFFFUL));
                    printf("            DDB_Reference_Data:     0x%08lx\n",(unsigned long)ddb_31->DDB_Reference_Data);
                    printf("            DDB_Service_Table_Ptr:  0x%08lx\n",(unsigned long)ddb_31->DDB_Service_Table_Ptr);
                    printf("            DDB_Service_Table_Size: 0x%08lx\n",(unsigned long)ddb_31->DDB_Service_Table_Size);

                    is_vxd = 1;

                    if (ddb_31->DDB_Control_Proc != 0 ||
                        le_parser_apply_fixup((unsigned char*)tmp,4,object,offset+offsetof(struct windows_vxd_ddb_win31,DDB_Control_Proc),&le_parser) > 0) {
                        if ((label=dec_label_malloc()) != NULL) {
                            dec_label_set_name(label,"VXD DDB_Control_Proc");

                            label->seg_v =
                                object;
                            label->ofs_v =
                                ddb_31->DDB_Control_Proc;

                            dec_label_xlate_32flat(label,&le_parser);
                        }
                    }

                    if (ddb_31->DDB_V86_API_Proc != 0 ||
                        le_parser_apply_fixup((unsigned char*)tmp,4,object,offset+offsetof(struct windows_vxd_ddb_win31,DDB_V86_API_Proc),&le_parser) > 0) {
                        if ((label=dec_label_malloc()) != NULL) {
                            dec_label_set_name(label,"VXD DDB_V86_API_Proc");

                            label->seg_v =
                                object;
                            label->ofs_v =
                                ddb_31->DDB_V86_API_Proc;

                            dec_label_xlate_32flat(label,&le_parser);
                        }
                    }

                    if (ddb_31->DDB_PM_API_Proc != 0 ||
                        le_parser_apply_fixup((unsigned char*)tmp,4,object,offset+offsetof(struct windows_vxd_ddb_win31,DDB_PM_API_Proc),&le_parser) > 0) {
                        if ((label=dec_label_malloc()) != NULL) {
                            dec_label_set_name(label,"VXD DDB_PM_API_Proc");

                            label->seg_v =
                                object;
                            label->ofs_v =
                                ddb_31->DDB_PM_API_Proc;

                            dec_label_xlate_32flat(label,&le_parser);
                        }
                    }

                    // go dump the service table
                    if (ddb_31->DDB_Service_Table_Size != 0 && le_segofs_to_trackio(&io,0/*flat 32-bit*/,ddb_31->DDB_Service_Table_Ptr,&le_parser)) {
                        uint32_t ptr;

                        printf("            DDB service table:\n");
                        for (i=0;i < (unsigned int)ddb_31->DDB_Service_Table_Size;i++) {
                            uint32_t ent_offset = io.offset;

                            if (le_trackio_read((unsigned char*)(&ptr),sizeof(uint32_t),src_fd,&io,&le_parser) != sizeof(uint32_t))
                                break;

                            /* service table entries can also be affected by fixups. */
                            le_parser_apply_fixup((unsigned char*)(&ptr),sizeof(ptr),object,ent_offset,&le_parser);

                            if (ddb_31->DDB_Service_Table_Ptr != 0) {
                                label = dec_find_label(object,ptr);
                                if (label == NULL) {
                                    if ((label=dec_label_malloc()) != NULL) {
                                        char tmp[256];

                                        sprintf(tmp,"VXD service call #0x%04x",i);
                                        dec_label_set_name(label,tmp);

                                        label->seg_v =
                                            object;
                                        label->ofs_v =
                                            ptr;

                                        dec_label_xlate_32flat(label,&le_parser);
                                    }
                                }
                            }

                            printf("                0x%08lX\n",(unsigned long)ptr);
                        }
                    }
                }
            }
        }
    }
 
    /* first pass: decompilation */
    {
        struct exe_le_header_object_table_entry *ent;
        unsigned int inscount;
        unsigned int los = 0;

        while (los < dec_label_count) {
            label = dec_label + los;
            if (label->seg_v == 0 || label->seg_v > le_parser.le_header.object_table_entries) {
                los++;
                continue;
            }

            ent = le_parser.le_object_table + label->seg_v - 1;
            if (!(ent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_EXECUTABLE)) {
                los++;
                continue;
            }

            if (label->seg_v == le_parser.le_object_flat_32bit) {
                if (!le_segofs_to_trackio(&io,0/*flat*/,label->ofs_v,&le_parser)) {
                    los++;
                    continue;
                }
            }
            else {
                if (!le_segofs_to_trackio(&io,label->seg_v,label->ofs_v,&le_parser)) {
                    los++;
                    continue;
                }
            }

            inscount = 0;
            reset_buffer();
            dec_ofs = 0;
            entry_ip = 0;
            dec_cs = label->seg_v;
            start_decom = 0;
            entry_cs = dec_cs;
            current_offset = label->ofs_v;
            dec_read = dec_end = dec_buffer;
            end_decom = ent->virtual_segment_size;
            if (ent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_386_BIG_DEFAULT)
                end_decom += le_parser.le_object_table_loaded_linear[label->seg_v - 1];
            refill(&io,&le_parser);
            minx86dec_init_state(&dec_st);
            dec_st.data32 = dec_st.addr32 =
                (ent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_386_BIG_DEFAULT) ? 1 : 0;

            printf("* NE segment #%d : 0x%04lx 1st pass from '%s'\n",
                (unsigned int)dec_cs,(unsigned long)label->ofs_v,label->name);

            do {
                uint32_t ofs = (uint32_t)(dec_read - dec_buffer) + current_offset_minus_buffer();
                uint32_t ip = ofs + entry_ip - dec_ofs;
                unsigned int c;

                if (!refill(&io,&le_parser)) break;

                minx86dec_set_buffer(&dec_st,dec_read,(int)(dec_end - dec_read));
                minx86dec_init_instruction(&dec_i);
                dec_st.ip_value = ip;
                minx86dec_decodeall(&dec_st,&dec_i);
                assert(dec_i.end >= dec_read);
                assert(dec_i.end <= (dec_buffer+sizeof(dec_buffer)));

                if (ent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_386_BIG_DEFAULT)
                    printf("%04lX:%08lX  ",(unsigned long)dec_cs,(unsigned long)dec_st.ip_value);
                else
                    printf("%04lX:%04lX      ",(unsigned long)dec_cs,(unsigned long)dec_st.ip_value);

                {
                    unsigned char *e = dec_i.end;

                    // INT 20h VXDCALL
                    if (is_vxd && dec_i.opcode == MXOP_INT && dec_i.argc == 1 &&
                            dec_i.argv[0].regtype == MX86_RT_IMM && dec_i.argv[0].value == 0x20)
                        e += 2+2;

                    for (c=0,iptr=dec_i.start;iptr != e;c++)
                        printf("%02X ",*iptr++);
                }

                if (dec_i.rep != MX86_REP_NONE) {
                    for (;c < 6;c++)
                        printf("   ");

                    switch (dec_i.rep) {
                        case MX86_REPE:
                            printf("REP   ");
                            break;
                        case MX86_REPNE:
                            printf("REPNE ");
                            break;
                        default:
                            break;
                    };
                }
                else {
                    for (;c < 8;c++)
                        printf("   ");
                }

                // Special instruction:
                //   Windows VXDs use INT 20h followed by two WORDs to call other VXDs.
                if (is_vxd && dec_i.opcode == MXOP_INT && dec_i.argc == 1 &&
                    dec_i.argv[0].regtype == MX86_RT_IMM && dec_i.argv[0].value == 0x20) {
                    // INT 20h WORD, WORD
                    // the decompiler should have set the instruction pointer at the first WORD now.
                    uint16_t vxd_device,vxd_service;

                    vxd_service = *((uint16_t*)dec_i.end); dec_i.end += 2;
                    vxd_device = *((uint16_t*)dec_i.end); dec_i.end += 2;

                    // bit 15 of the service indicates a jmp, not call
                    if (vxd_service & 0x8000) {
                        printf("VxDJmp   Device=0x%04X '%s' Service=0x%04X '%s'",
                            vxd_device,
                            vxd_device_to_name(vxd_device),
                            vxd_service & 0x7FFF,
                            vxd_service_to_name(vxd_device,vxd_service & 0x7FFF));
                    }
                    else {
                        printf("VxDCall  Device=0x%04X '%s' Service=0x%04X '%s'",
                            vxd_device,
                            vxd_device_to_name(vxd_device),
                            vxd_service,
                            vxd_service_to_name(vxd_device,vxd_service));
                    }
                }
                else {
                    printf("%-8s ",opcode_string[dec_i.opcode]);

                    for (c=0;c < (unsigned int)dec_i.argc;) {
                        minx86dec_regprint(&dec_i.argv[c],arg_c);
                        printf("%s",arg_c);
                        if (++c < (unsigned int)dec_i.argc) printf(",");
                    }
                }
                if (dec_i.lock) printf("  ; LOCK#");
                printf("\n");

                dec_read = dec_i.end;

                if ((dec_i.opcode == MXOP_JMP || dec_i.opcode == MXOP_CALL || dec_i.opcode == MXOP_JCXZ ||
                    (dec_i.opcode >= MXOP_JO && dec_i.opcode <= MXOP_JG)) && dec_i.argc == 1 &&
                    dec_i.argv[0].regtype == MX86_RT_IMM) {
                    /* make a label of the target */
                    /* 1st arg is target offset */
                    uint32_t toffset = dec_i.argv[0].value;

                    printf("Target: 0x%04lx\n",(unsigned long)toffset);

                    label = dec_find_label(dec_cs,toffset);
                    if (label == NULL) {
                        if ((label=dec_label_malloc()) != NULL) {
                            if (dec_i.opcode == MXOP_JMP || dec_i.opcode == MXOP_JCXZ ||
                                    (dec_i.opcode >= MXOP_JO && dec_i.opcode <= MXOP_JG))
                                dec_label_set_name(label,"JMP target");
                            else if (dec_i.opcode == MXOP_CALL)
                                dec_label_set_name(label,"CALL target");

                            label->seg_v =
                                dec_cs;
                            label->ofs_v =
                                toffset;

                            dec_label_xlate_32flat(label,&le_parser);
                        }
                    }

                    if (dec_i.opcode == MXOP_JMP)
                        break;
                }
                else if (dec_i.opcode == MXOP_JMP)
                    break;
                else if (dec_i.opcode == MXOP_RET || dec_i.opcode == MXOP_RETF)
                    break;
                else if (dec_i.opcode == MXOP_CALL_FAR || dec_i.opcode == MXOP_JMP_FAR) {
                    if (dec_i.opcode == MXOP_JMP_FAR)
                        break;
                }

                if (++inscount >= 1024)
                    break;
            } while(1);

            los++;
        }
    }

    /* sort labels */
    dec_label_sort();

    {
        struct dec_label *label;
        unsigned int i;

        printf("* Labels (%u labels):\n",(unsigned int)dec_label_count);
        for (i=0;i < dec_label_count;i++) {
            label = dec_label + i;

            printf("    %04X:%08lX '%s'\n",
                (unsigned int)label->seg_v,
                (unsigned long)label->ofs_v,
                label->name);
        }
    }

    /* second pass decompiler */
    if (le_parser.le_object_table != NULL) {
        struct exe_le_header_object_table_entry *ent;
        struct fixup_tracking_window_ent *fixent;
        uint32_t page_base;
        unsigned int i;
        uint32_t page;
        size_t inslen;

        for (i=0;i < le_parser.le_header.object_table_entries;i++) {
            ent = le_parser.le_object_table + i;

            fixup_tracking_window_free(&fixup_window);

            printf("* LE object #%u (%u-bit)\n",
                i + 1,
                (ent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_386_BIG_DEFAULT) ? 32 : 16);
            if (!(ent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_EXECUTABLE)) {
                printf("    Ignoring data object\n");
                continue;
            }

            if (ent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_386_BIG_DEFAULT) {
                if (!le_segofs_to_trackio(&io,0/*flat*/,le_parser.le_object_table_loaded_linear[i],&le_parser))
                    continue;
            }
            else {
                if (!le_segofs_to_trackio(&io,i + 1,0,&le_parser))
                    continue;
            }

            reset_buffer();
            labeli = 0;
            dec_ofs = 0;
            entry_ip = 0;
            start_decom = 0;
            page = ent->page_map_index;
            end_decom = ent->virtual_segment_size;

            if (ent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_386_BIG_DEFAULT) {
                current_offset = le_parser.le_object_table_loaded_linear[i];
                end_decom += le_parser.le_object_table_loaded_linear[i];
                page_base = le_parser.le_object_table_loaded_linear[i];
                dec_cs = le_parser.le_object_flat_32bit;
            }
            else {
                current_offset = 0;
                dec_cs = i + 1;
                page_base = 0;
            }

            entry_cs = dec_cs;
            dec_read = dec_end = dec_buffer;
            refill(&io,&le_parser);
            minx86dec_init_state(&dec_st);
            dec_st.data32 = dec_st.addr32 =
                (ent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_386_BIG_DEFAULT) ? 1 : 0;

            do {
                uint32_t ofs = (uint32_t)(dec_read - dec_buffer) + current_offset_minus_buffer();
                uint32_t ip = ofs + entry_ip - dec_ofs;
                unsigned char dosek = 0;
                uint32_t label_ip = 0;
                unsigned int c;

                while (labeli < dec_label_count) {
                    label = dec_label + labeli;
                    if (label->seg_v != dec_cs) {
                        labeli++;
                        continue;
                    }

                    if (ent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_386_BIG_DEFAULT) {
                        if (label->ofs_v < le_parser.le_object_table_loaded_linear[i]) {
                            labeli++;
                            continue;
                        }
                    }

                    if (ip < label->ofs_v)
                        break;

                    labeli++;
                    label_ip = label->ofs_v;

                    printf("Label '%s' at %04lx:%04lx\n",
                            label->name ? label->name : "",
                            (unsigned long)label->seg_v,
                            (unsigned long)label->ofs_v);

                    label = dec_label + labeli;
                    dosek = 1;
                }

                if (dosek) {
                    ip = label_ip;
                    reset_buffer();
                    current_offset = ofs;
                    if (ent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_386_BIG_DEFAULT) {
                        if (!le_segofs_to_trackio(&io,0/*flat*/,ip,&le_parser))
                            break;
                    }
                    else {
                        if (!le_segofs_to_trackio(&io,i + 1,ip,&le_parser))
                            break;
                    }
                }

                if (!refill(&io,&le_parser)) break;

                /* track page.
                 * load new entries slightly ahead (16 bytes) because
                 * relocations that span pages reach backwards into the prior page.
                 * since we only read forward, we can free older relocations from memory
                 * when we load new ones. */
                if (le_parser.le_fixup_records.table != NULL) {
                    uint32_t pob = ip + (uint32_t)16 - page_base;
                    uint32_t po = (pob / (uint32_t)le_parser.le_header.memory_page_size) + ent->page_map_index;
                    struct le_header_fixup_record_table *frtable;
                    unsigned int srcoff_count,srcoff_i;
                    unsigned char flags,src;
                    uint32_t srclinoff;
                    unsigned char *raw;
                    uint16_t srcoff;
                    uint8_t chk=0;
                    size_t ti;

                    /* setup */

                    /* free old entries */
                    fixup_tracking_window_lazy_flush(&fixup_window);

                    /* load new entries */
                    while (page <= po) {
                        uint32_t pagelinoff =
                            ((uint32_t)page - (uint32_t)ent->page_map_index) * (uint32_t)le_parser.le_header.memory_page_size;

                        if (page != 0 && page <= le_parser.le_header.number_of_memory_pages) {
                            frtable = le_parser.le_fixup_records.table + page - 1;
                            if (frtable->table != NULL && frtable->length != 0) {
                                printf("* Loading relocations for page #%u\n",page);

                                chk = 1;
                                for (ti=0;ti < frtable->length;ti++) {
                                    raw = le_header_fixup_record_table_get_raw_entry(frtable,ti);

                                    // caller ensures the record is long enough
                                    src = *raw++;
                                    flags = *raw++;

                                    if (src & 0xC0)
                                        continue;

                                    if (src & 0x20) {
                                        srcoff_count = *raw++; //number of source offsets. object follows, then array of srcoff
                                    }
                                    else {
                                        srcoff_count = 1;
                                        srcoff = *((int16_t*)raw); raw += 2;
                                    }

                                    if ((flags&3) == 0) { // internal reference
                                        if (flags&0x40) {
                                            raw += 2; /* tobject = *((uint16_t*)raw); */
                                        }
                                        else {
                                            raw++; /* tobject = *raw++; */
                                        }

                                        if ((src&0xF) != 0x2) { /* not 16-bit selector fixup */
                                            if (flags&0x10) { // 32-bit target offset
                                                raw += 4; /* trgoff = *((uint32_t*)raw); */
                                            }
                                            else { // 16-bit target offset
                                                raw += 2; /* trgoff = *((uint16_t*)raw); */
                                            }
                                        }

                                        if (src & 0x20) {
                                            for (srcoff_i=0;srcoff_i < srcoff_count;srcoff_i++) {
                                                srcoff = *((int16_t*)raw); raw += 2;

                                                if ((src&0xF) == 0x7) { // must be 32-bit offset fixup
                                                    // what is the relocation relative to the struct we just read?
                                                    srclinoff =
                                                        le_parser.le_object_table_loaded_linear[i] + pagelinoff + (uint32_t)srcoff;
                                                    fixent =
                                                        fixup_tracking_window_alloc_entry(&fixup_window);
                                                    if (fixent) {
                                                        fixent->fixup_rec_page = page;
                                                        fixent->fixup_rec_index = ti;
                                                        fixent->linear_address = srclinoff;
                                                    }
                                                    else {
                                                        printf("! unable to alloc reloc tracking\n");
                                                    }
                                                }
                                            }
                                        }
                                        else {
                                            if ((src&0xF) == 0x7) { // must be 32-bit offset fixup
                                                // what is the relocation relative to the struct we just read?
                                                srclinoff =
                                                    le_parser.le_object_table_loaded_linear[i] + pagelinoff + (uint32_t)srcoff;
                                                fixent =
                                                    fixup_tracking_window_alloc_entry(&fixup_window);
                                                if (fixent) {
                                                    fixent->fixup_rec_page = page;
                                                    fixent->fixup_rec_index = ti;
                                                    fixent->linear_address = srclinoff;
                                                }
                                                else {
                                                    printf("! unable to alloc reloc tracking\n");
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        page++;
                    }

                    if (chk) {
                        /* sort entries (past read pointer) so code below can read entry-by-entry.
                         * ONLY sort the entries yet to be read, not the ones already read. */
                        assert(fixup_window.table != NULL);
                        assert(fixup_window.read <= fixup_window.length);
                        if (fixup_window.read < fixup_window.length) {
                            qsort(fixup_window.table+fixup_window.read,
                                  fixup_window.length-fixup_window.read,
                                  sizeof(*(fixup_window.table)),fixup_window_sort);
                        }
                    }
                }

                minx86dec_set_buffer(&dec_st,dec_read,(int)(dec_end - dec_read));
                minx86dec_init_instruction(&dec_i);
                dec_st.ip_value = ip;
                minx86dec_decodeall(&dec_st,&dec_i);
                assert(dec_i.end >= dec_read);
                assert(dec_i.end <= (dec_buffer+sizeof(dec_buffer)));
                inslen = (size_t)(dec_i.end - dec_i.start);

                /* fixup tracking */
                while (fixup_window.table != NULL && fixup_window.read < fixup_window.length) {
                    fixent = fixup_window.table + fixup_window.read;
                    if (dec_st.ip_value < fixent->linear_address) break;
                    fixup_window.read++;
                }

                if (ent->object_flags & LE_HEADER_OBJECT_TABLE_ENTRY_FLAGS_386_BIG_DEFAULT)
                    printf("%04lX:%08lX  ",(unsigned long)dec_cs,(unsigned long)dec_st.ip_value);
                else
                    printf("%04lX:%04lX      ",(unsigned long)dec_cs,(unsigned long)dec_st.ip_value);

                {
                    unsigned char *e = dec_i.end;

                    // INT 20h VXDCALL
                    if (is_vxd && dec_i.opcode == MXOP_INT && dec_i.argc == 1 &&
                        dec_i.argv[0].regtype == MX86_RT_IMM && dec_i.argv[0].value == 0x20)
                        e += 2+2;

                    for (c=0,iptr=dec_i.start;iptr != e;c++)
                        printf("%02X ",*iptr++);
                }

                if (dec_i.rep != MX86_REP_NONE) {
                    for (;c < 6;c++)
                        printf("   ");

                    switch (dec_i.rep) {
                        case MX86_REPE:
                            printf("REP   ");
                            break;
                        case MX86_REPNE:
                            printf("REPNE ");
                            break;
                        default:
                            break;
                    };
                }
                else {
                    for (;c < 8;c++)
                        printf("   ");
                }

                // Special instruction:
                //   Windows VXDs use INT 20h followed by two WORDs to call other VXDs.
                if (is_vxd && dec_i.opcode == MXOP_INT && dec_i.argc == 1 &&
                    dec_i.argv[0].regtype == MX86_RT_IMM && dec_i.argv[0].value == 0x20) {
                    // INT 20h WORD, WORD
                    // the decompiler should have set the instruction pointer at the first WORD now.
                    uint16_t vxd_device,vxd_service;

                    vxd_service = *((uint16_t*)dec_i.end); dec_i.end += 2;
                    vxd_device = *((uint16_t*)dec_i.end); dec_i.end += 2;

                    // bit 15 of the service indicates a jmp, not call
                    if (vxd_service & 0x8000) {
                        printf("VxDJmp   Device=0x%04X '%s' Service=0x%04X '%s'",
                            vxd_device,
                            vxd_device_to_name(vxd_device),
                            vxd_service & 0x7FFF,
                            vxd_service_to_name(vxd_device,vxd_service & 0x7FFF));
                    }
                    else {
                        printf("VxDCall  Device=0x%04X '%s' Service=0x%04X '%s'",
                            vxd_device,
                            vxd_device_to_name(vxd_device),
                            vxd_service,
                            vxd_service_to_name(vxd_device,vxd_service));
                    }
                }
                else {
                    printf("%-8s ",opcode_string[dec_i.opcode]);

                    for (c=0;c < (unsigned int)dec_i.argc;) {
                        minx86dec_regprint(&dec_i.argv[c],arg_c);
                        printf("%s",arg_c);
                        if (++c < (unsigned int)dec_i.argc) printf(",");
                    }
                }
                if (dec_i.lock) printf("  ; LOCK#");
                printf("\n");

                dec_read = dec_i.end;

                if (fixup_window.table != NULL && fixup_window.read < fixup_window.length) {
                    fixent = fixup_window.table + fixup_window.read;
                    if (fixent->linear_address >= dec_st.ip_value &&
                        fixent->linear_address < (dec_st.ip_value + inslen)) {
                        struct le_header_fixup_record_table *frtable;
                        unsigned char flags,src;
                        unsigned char *raw;

                        assert(fixent->fixup_rec_page > 0);
                        assert(fixent->fixup_rec_page <= le_parser.le_header.number_of_memory_pages);
                        frtable = le_parser.le_fixup_records.table + fixent->fixup_rec_page - 1;
                        raw = le_header_fixup_record_table_get_raw_entry(frtable,fixent->fixup_rec_index);

                        printf("             ^ Relocation at 0x%08lx (+%u bytes from start of instruction)\n",
                                (unsigned long)fixent->linear_address,
                                (unsigned int)(fixent->linear_address - dec_st.ip_value));

                        if (raw != NULL) {
                            src = *raw++;
                            flags = *raw++;

                            printf("                Source type:            0x%02X ",src);
                            switch (src&0xF) {
                                case 0x2:
                                    printf("16-bit selector fixup (16 bits)");
                                    break;
                                case 0x7:
                                    printf("32-bit offset fixup (32 bits)");
                                    break;
                                case 0x8:
                                    printf("32-bit self-relative offset fixup (32 bits)");
                                    break;
                                default:
                                    printf("Unknown");
                                    continue;
                            };
                            if (src & 0x10)
                                printf(" Fix-up to alias");
                            printf("\n");

                            printf("                Source flags:           0x%02X ",flags);
                            switch (flags&3) {
                                case 0x0:
                                    printf("Internal reference");
                                    break;
                                case 0x1:
                                    printf("Imported reference by ordinal");
                                    break;
                                case 0x2:
                                    printf("Imported reference by name");
                                    break;
                                case 0x3:
                                    printf("Internal reference via entry table");
                                    break;
                            };
                            if (flags&4) printf(" ADDITIVE");
                            if (flags&8) printf(" \"Internal chaining fixup\"");
                            if (flags&0x10) printf(" \"32-bit target offset\"");
                            if (flags&0x20) printf(" \"32-bit additive fixup value\"");
                            if (flags&0x40) printf(" \"16-bit object number/module ordinal\"");
                            if (flags&0x80) printf(" \"8-bit ordinal\"");
                            printf("\n");

                            if (src & 0x20)
                                raw++; //number of source offsets. object follows, then array of srcoff
                            else
                                raw += 2; //srcoff

                            if ((flags&3) == 0) { // internal reference
                                uint32_t trglinoff;
                                uint16_t tobject;
                                uint32_t trgoff;

                                if (flags&0x40) {
                                    tobject = *((uint16_t*)raw); raw += 2;
                                }
                                else {
                                    tobject = *raw++;
                                }

                                printf("                Target object:          #%u\n",(unsigned int)tobject);
                                if ((src&0xF) != 0x2) { /* not 16-bit selector fixup */
                                    if (flags&0x10) { // 32-bit target offset
                                        trgoff = *((uint32_t*)raw); raw += 4;
                                    }
                                    else { // 16-bit target offset
                                        trgoff = *((uint16_t*)raw); raw += 2;
                                    }

                                    // for this computation, we need to convert target object:offset to linear address
                                    if (tobject != 0 && tobject <= le_parser.le_header.object_table_entries)
                                        trglinoff = le_parser.le_object_table_loaded_linear[tobject - 1] + trgoff;
                                    else
                                        trglinoff = 0;

                                    printf("                Target offset:          linear=0x%08lX offset=0x%08lX\n",
                                        (unsigned long)trglinoff,(unsigned long)trgoff);
                                }
                            }
                        }
                    }
                }
            } while(1);

            fixup_tracking_window_free(&fixup_window);
        }
    }

    fixup_tracking_window_free(&fixup_window);
    le_header_parseinfo_free(&le_parser);
    dec_free_labels();
    close(src_fd);
    return 0;
}

