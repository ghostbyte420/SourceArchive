using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using System.Security;
using System.Threading;

namespace MasterControlProgram
{
    [Flags]
    public enum DuplicateOptions : uint
    {
        DUPLICATE_CLOSE_SOURCE = (0x00000001),// Closes the source handle. This occurs regardless of any error status returned.
        DUPLICATE_SAME_ACCESS = (0x00000002), //Ignores the dwDesiredAccess parameter. The duplicate handle has the same access as the source handle.
    }

    [Flags]
    public enum ProcessAccessFlags : uint
    {
        All = 0x001F0FFF,
        Terminate = 0x00000001,
        CreateThread = 0x00000002,
        VMOperation = 0x00000008,
        VMRead = 0x00000010,
        VMWrite = 0x00000020,
        DupHandle = 0x00000040,
        SetInformation = 0x00000200,
        QueryInformation = 0x00000400,
        Synchronize = 0x00100000
    }

    [Flags]
    public enum AllocationType
    {
        Commit = 0x1000,
        Reserve = 0x2000,
        Decommit = 0x4000,
        Release = 0x8000,
        Reset = 0x80000,
        Physical = 0x400000,
        TopDown = 0x100000,
        WriteWatch = 0x200000,
        LargePages = 0x20000000
    }

    [Flags]
    public enum MemoryProtection
    {
        Execute = 0x10,
        ExecuteRead = 0x20,
        ExecuteReadWrite = 0x40,
        ExecuteWriteCopy = 0x80,
        NoAccess = 0x01,
        ReadOnly = 0x02,
        ReadWrite = 0x04,
        WriteCopy = 0x08,
        GuardModifierflag = 0x100,
        NoCacheModifierflag = 0x200,
        WriteCombineModifierflag = 0x400
    }

    [SuppressUnmanagedCodeSecurity]
    public sealed class SafeLocalMemHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        public SafeLocalMemHandle() : base(true) { }

        public SafeLocalMemHandle(IntPtr existingHandle, bool ownsHandle)
            : base(ownsHandle)
        {
            base.SetHandle(existingHandle);
        }

        [DllImport("advapi32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern bool ConvertStringSecurityDescriptorToSecurityDescriptor(string StringSecurityDescriptor, int StringSDRevision, out SafeLocalMemHandle pSecurityDescriptor, IntPtr SecurityDescriptorSize);

        [DllImport("kernel32.dll")]
        private static extern IntPtr LocalFree(IntPtr hMem);

        protected override bool ReleaseHandle()
        {
            return (LocalFree(base.handle) == IntPtr.Zero);
        }
    }

    [SuppressUnmanagedCodeSecurity]
    public sealed class SafeProcessHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        public SafeProcessHandle() : base(true) { }

        public SafeProcessHandle(IntPtr handle)
            : base(true)
        {
            base.SetHandle(handle);
        }

        internal void InitialSetHandle(IntPtr h)
        {
            base.handle = h;
        }

        protected override bool ReleaseHandle()
        {
            return WINAPI.CloseHandle(base.handle);
        }
    }

    [SuppressUnmanagedCodeSecurity]
    public sealed class SafeThreadHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        public SafeThreadHandle() : base(true) { }

        internal void InitialSetHandle(IntPtr h)
        {
            base.SetHandle(h);
        }

        protected override bool ReleaseHandle()
        {
            return WINAPI.CloseHandle(base.handle);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public sealed class PROCESS_INFORMATION
    {
        public IntPtr hProcess = IntPtr.Zero;
        public IntPtr hThread = IntPtr.Zero;
        public uint dwProcessId;
        public uint dwThreadId;
    }

    [StructLayout(LayoutKind.Sequential)]
    public sealed class SECURITY_ATTRIBUTES
    {
        public int nLength = 12;
        public SafeLocalMemHandle lpSecurityDescriptor = new SafeLocalMemHandle(IntPtr.Zero, false);
        public bool bInheritHandle;
    }

    [StructLayout(LayoutKind.Sequential)]
    public sealed class STARTUPINFO
    {
        public int cb;
        public IntPtr lpReserved = IntPtr.Zero;
        public IntPtr lpDesktop = IntPtr.Zero;
        public IntPtr lpTitle = IntPtr.Zero;
        public int dwX;
        public int dwY;
        public int dwXSize;
        public int dwYSize;
        public int dwXCountChars;
        public int dwYCountChars;
        public int dwFillAttribute;
        public int dwFlags;
        public short wShowWindow;
        public short cbReserved2;
        public IntPtr lpReserved2 = IntPtr.Zero;
        public SafeFileHandle hStdInput = new SafeFileHandle(IntPtr.Zero, false);
        public SafeFileHandle hStdOutput = new SafeFileHandle(IntPtr.Zero, false);
        public SafeFileHandle hStdError = new SafeFileHandle(IntPtr.Zero, false);
        public STARTUPINFO()
        {
            this.cb = Marshal.SizeOf(this);
        }

        public void Dispose()
        {
            if ((this.hStdInput != null) && !this.hStdInput.IsInvalid)
            {
                this.hStdInput.Close();
                this.hStdInput = null;
            }
            if ((this.hStdOutput != null) && !this.hStdOutput.IsInvalid)
            {
                this.hStdOutput.Close();
                this.hStdOutput = null;
            }
            if ((this.hStdError != null) && !this.hStdError.IsInvalid)
            {
                this.hStdError.Close();
                this.hStdError = null;
            }
        }
    }

    [SuppressUnmanagedCodeSecurity]
    static unsafe partial class WINAPI
    {
        public const uint CREATE_SUSPENDED = 0x00000004;
        public const uint CREATE_UNICODE_ENVIRONMENT = 0x00000400;
        public const uint CREATE_PRESERVE_CODE_AUTHZ_LEVEL = 0x02000000;
        public const uint CREATE_DEFAULT_ERROR_MODE = 0x04000000;
        public const uint CREATE_NO_WINDOW = 0x08000000;

        public const uint PROCESS_ALL_ACCESS = 0x1F0FFF;
        public const int INVALID_HANDLE_VALUE = -1;

        public const uint FILE_MAP_WRITE = 0x0002;
        public const uint FILE_MAP_ALL_ACCESS = 0x001F;
        public const uint SEMAPHORE_MODIFY_STATE = 0x0002;
        public const uint SEMAPHORE_ALL_ACCESS = 0x1F0003;
        public const uint SYNCHRONIZE = 0x00100000;
        public const uint MUTEX_ALL_ACCESS = 0x1F0001;
        public const uint INFINITE = 0xFFFFFFFF;
        public const uint WAIT_OBJECT_0 = 0x00000000;
        public const uint WAIT_ABANDONED = 0x00000080;
        public const uint WAIT_TIMEOUT = 0x00000102;
        public const uint WAIT_FAILED = 0xFFFFFFFF;
        public const uint PAGE_EXECUTE_READWRITE = 0x40;
        public const int ERROR_ALREADY_EXISTS = 183;

        [DllImport("MasterControl.dll")]
        public static extern void Inject(IntPtr hProcess);

        [DllImport("MasterControl.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool GetIpcInfo(IntPtr hProcess, ref IpcInfoStruct info);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool TerminateProcess(IntPtr hProcess, uint uExitCode);

        [DllImport("ntdll.dll", SetLastError = true)]
        public static extern uint NtResumeProcess(IntPtr hProcess);

        [DllImport("ntdll.dll", SetLastError = true)]
        public static extern uint NtSuspendProcess(IntPtr hProcess);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool DuplicateHandle(IntPtr hSourceProcessHandle,
           IntPtr hSourceHandle, IntPtr hTargetProcessHandle, out IntPtr lpTargetHandle,
           uint dwDesiredAccess, [MarshalAs(UnmanagedType.Bool)] bool bInheritHandle, DuplicateOptions dwOptions);

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.Cdecl, SetLastError = true)]
        public static extern uint InterlockedCompareExchange(uint* destination, uint exchange, uint comparand);

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.Cdecl, SetLastError = true)]
        public static extern uint InterlockedExchange(uint* target, uint val);

        [DllImport("kernel32.dll")]
        public static extern uint GetCurrentProcessId();

        [DllImport("kernel32.dll")]
        public static extern IntPtr OpenProcess(ProcessAccessFlags dwDesiredAccess, [MarshalAs(UnmanagedType.Bool)] bool bInheritHandle, uint dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr MapViewOfFile(IntPtr hFileMappingObject, uint dwDesiredAccess, uint dwFileOffsetHigh, uint dwFileOffsetLow, uint dwNumberOfBytesToMap);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool ReleaseSemaphore(IntPtr hSemaphore, int lReleaseCount, IntPtr lpPreviousCount);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool ReleaseMutex(IntPtr hMutex);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        [DllImport("msvcrt.dll")]
        public static extern unsafe void memcpy(void* to, void* from, int len);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr OpenProcess(ProcessAccessFlags dwDesiredAccess, [MarshalAs(UnmanagedType.Bool)] bool bInheritHandle, int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CreateProcess(
            [MarshalAs(UnmanagedType.LPTStr)] string lpApplicationName, StringBuilder lpCommandLine, SECURITY_ATTRIBUTES lpProcessAttributes,
            SECURITY_ATTRIBUTES lpThreadAttributes, bool bInheritHandles, uint dwCreationFlags, IntPtr lpEnvironment,
            [MarshalAs(UnmanagedType.LPTStr)] string lpCurrentDirectory, STARTUPINFO lpStartupInfo, PROCESS_INFORMATION lpProcessInformation);

        [DllImport("kernel32.dll")]
        public static extern int ResumeThread(IntPtr hThread);
    }
}
