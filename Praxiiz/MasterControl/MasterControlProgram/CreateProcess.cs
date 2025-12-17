using System;
using System.Runtime.InteropServices;
using System.ComponentModel;
using System.Security;
using Microsoft.Win32.SafeHandles;
using System.Runtime.ConstrainedExecution;
using System.Security.Permissions;
using System.Text;
using System.Collections;
using System.Collections.Specialized;
using System.Diagnostics;

namespace MasterControlProgram
{
    internal sealed class OrdinalCaseInsensitiveComparer : IComparer
    {
        public static readonly OrdinalCaseInsensitiveComparer Default = new OrdinalCaseInsensitiveComparer();
        public int Compare(object a, object b)
        {
            string str = a as string;
            string str2 = b as string;
            if ((str != null) && (str2 != null))
            {
                return string.CompareOrdinal(str.ToUpperInvariant(), str2.ToUpperInvariant());
            }
            return Comparer.Default.Compare(a, b);
        }
    }

    [SuppressUnmanagedCodeSecurity]
    static partial class WINAPI
    {
        private static bool IsNt
        {
            get { return (Environment.OSVersion.Platform == PlatformID.Win32NT); }
        }


        public static byte[] EnvironmentToByteArray(StringDictionary sd, bool unicode)
        {
            string[] array = new string[sd.Count];
            byte[] bytes = null;
            sd.Keys.CopyTo(array, 0);
            string[] strArray2 = new string[sd.Count];
            sd.Values.CopyTo(strArray2, 0);
            Array.Sort(array, strArray2, OrdinalCaseInsensitiveComparer.Default);
            StringBuilder builder = new StringBuilder();
            for (int i = 0; i < sd.Count; i++)
            {
                builder.Append(array[i]);
                builder.Append('=');
                builder.Append(strArray2[i]);
                builder.Append('\0');
            }
            builder.Append('\0');
            if (unicode)
            {
                bytes = Encoding.Unicode.GetBytes(builder.ToString());
            }
            else
            {
                bytes = Encoding.Default.GetBytes(builder.ToString());
            }
            if (bytes.Length > 0xFFFF)
            {
                throw new InvalidOperationException("EnvironmentBlockTooLong");
            }
            return bytes;
        }

        private static StringBuilder BuildCommandLine(string executableFileName, string arguments)
        {
            StringBuilder sb = new StringBuilder();
            executableFileName = executableFileName.Trim();
            bool ready = executableFileName.StartsWith("\"", StringComparison.Ordinal) && executableFileName.EndsWith("\"", StringComparison.Ordinal);
            if (!ready)
            {
                sb.Append("\"");
            }
            sb.Append(executableFileName);
            if (!ready)
            {
                sb.Append("\"");
            }
            if (!string.IsNullOrEmpty(arguments))
            {
                sb.Append(" ");
                sb.Append(arguments);
            }
            return sb;
        }

        // I think I jacked a lot of this code from mscorlib, I don't remember....
        public static bool CreateProcess(ProcessStartInfo startInfo, bool createSuspended, out SafeProcessHandle process, out SafeThreadHandle thread, out int processID, out int threadID)
        {
            StringBuilder cmdLine = BuildCommandLine(startInfo.FileName, startInfo.Arguments);
            STARTUPINFO lpStartupInfo = new STARTUPINFO();
            PROCESS_INFORMATION lpProcessInformation = new PROCESS_INFORMATION();
            SafeProcessHandle processHandle = new SafeProcessHandle();
            SafeThreadHandle threadHandle = new SafeThreadHandle();
            GCHandle environment = new GCHandle();
            uint creationFlags = 0;
            bool success;

            creationFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_PRESERVE_CODE_AUTHZ_LEVEL;


            if (createSuspended)
                creationFlags |= CREATE_SUSPENDED;

            if (startInfo.CreateNoWindow)
                creationFlags |= CREATE_NO_WINDOW;

            IntPtr pinnedEnvironment = IntPtr.Zero;

            if (startInfo.EnvironmentVariables != null)
            {
                bool unicode = false;
                if (IsNt)
                {
                    creationFlags |= CREATE_UNICODE_ENVIRONMENT;
                    unicode = true;
                }
                environment = GCHandle.Alloc(EnvironmentToByteArray(startInfo.EnvironmentVariables, unicode), GCHandleType.Pinned);
                pinnedEnvironment = environment.AddrOfPinnedObject();
            }

            string workingDirectory = startInfo.WorkingDirectory;
            if (workingDirectory == "")
                workingDirectory = Environment.CurrentDirectory;

            success = CreateProcess(null, cmdLine, null, null, false, creationFlags, pinnedEnvironment, workingDirectory, lpStartupInfo, lpProcessInformation);

            if ((lpProcessInformation.hProcess != IntPtr.Zero) && (lpProcessInformation.hProcess.ToInt32() != INVALID_HANDLE_VALUE))
                processHandle.InitialSetHandle(lpProcessInformation.hProcess);

            if ((lpProcessInformation.hThread != IntPtr.Zero) && (lpProcessInformation.hThread.ToInt32() != INVALID_HANDLE_VALUE))
                threadHandle.InitialSetHandle(lpProcessInformation.hThread);

            if (environment.IsAllocated)
                environment.Free();

            lpStartupInfo.Dispose();

            if (success && !processHandle.IsInvalid && !threadHandle.IsInvalid)
            {
                process = processHandle;
                thread = threadHandle;
                processID = (int)lpProcessInformation.dwProcessId;
                threadID = (int)lpProcessInformation.dwThreadId;
                return true;
            }

            process = null;
            thread = null;
            processID = 0;
            threadID = 0;
            return false;
        }
    }
}
