using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Threading;
using System.Reflection;
using System.Reflection.Emit;
using System.IO;

namespace MasterControlProgram
{
    unsafe class IPC
    {
        public delegate void MemCpyFunction(void* des, void* src, uint len);
        public static readonly MemCpyFunction MemCpy;

        static IntPtr inFileMap = IntPtr.Zero;
        static IntPtr outFileMap = IntPtr.Zero;
        static IntPtr inSemaphore = IntPtr.Zero;
        static IntPtr outSemaphore = IntPtr.Zero;
        static IntPtr inputView = IntPtr.Zero;
        static IntPtr outputView = IntPtr.Zero;
        static volatile bool outputContinue = true;
        static volatile bool loggingEnabled = false;
        static byte* input;
        static byte* output;
        static Thread outputThread;
        public static volatile bool IncomingFilterDemo = false;
        public static volatile bool OutgoingFilterDemo = false;
        static uint *inputLock;
        static uint *outputLock;

        public static bool LoggingEnabled
        {
            get { return loggingEnabled; }
            set { loggingEnabled = value; }
        }

        static IPC()
        {
            // Fast managed memcpy using IL opcodes
            DynamicMethod dm = new DynamicMethod("MemCpy", typeof(void), new[] { typeof(void*), typeof(void*), typeof(uint) }, typeof(Program));
            ILGenerator i = dm.GetILGenerator();
            i.Emit(OpCodes.Ldarg_0);
            i.Emit(OpCodes.Ldarg_1);
            i.Emit(OpCodes.Ldarg_2);
            i.Emit(OpCodes.Cpblk);
            i.Emit(OpCodes.Ret);
            MemCpy = (MemCpyFunction)dm.CreateDelegate(typeof(MemCpyFunction));
        }

        static void ObtainInputBufferAccess()
        {
            // 0 = free for write access, 1 = in use, 2 = free for read access
            int bufferLock = *(int*)input;
            while (0 != WINAPI.InterlockedCompareExchange(inputLock, 1, 0))
            {
                Thread.Sleep(1);
            }
        }

        static void ReleaseInputBuffer()
        {
            // 0 = free for write access, 1 = in use, 2 = free for read access
            WINAPI.InterlockedExchange(inputLock, 2);
            // tell MasterControl.dll that it has data to grab
            WINAPI.ReleaseSemaphore(inSemaphore, 1, IntPtr.Zero);
        }

        public static void ExecuteMacroType1(uint arg1, ushort arg2)
        {
            /* layout of IPC buffer:
             * DWORD lock (used for thread safe access to the buffer)
             * BYTE MessageType (either IncomingMessageType or OutgoingMessageType
             * DWORD len (the length of the following raw byte buffer
             * BYTE[] ^len bytes of raw data */

            /* Each MessageType will have a struct associated with it which
             * describes the data layout for that specific message.  The only
             * exception is for messages which only have one field (i.e. raw data)
             * like the incoming and outgoing packet messages for instance. */

            /* The layout for the Type2Macro message type is:
             * DWORD arg1 (first argument for macro, anyone who remembers old version of EUO will remember...)
             * WORD arg2 */

            /* We're sending commands on the input buffer because the buffers are named
             * from the point of view of MasterControl.dll.  MasterControl receives commands on the input buffer */

            Type1MacroStruct m = new Type1MacroStruct();
            m.arg1 = arg1;
            m.arg2 = arg2;
            SendIpcMessage(IncomingMessageType.Type1Macro, &m, (uint)sizeof(Type1MacroStruct));
        }

        public static void CallPathfindFunction(ushort x, ushort y, ushort z)
        {
            PathfindStruct p;
            p.x = x;
            p.y = y;
            p.z = z;
            SendIpcMessage(IncomingMessageType.Pathfind, &p, (uint)sizeof(PathfindStruct));
        }

        public static void CallGumpFunction(uint gumpHandle, uint function)
        {
            CallGumpFunctionStruct cg;
            cg.functionIndex = function;
            cg.gumpHandle = gumpHandle;
            SendIpcMessage(IncomingMessageType.CallGumpFunction, &cg, (uint)sizeof(CallGumpFunctionStruct));
        }

        public static void SetLoginServer(uint server, ushort port)
        {
            LoginServerStruct l;
            l.server = server;
            l.port = port;
            SendIpcMessage(IncomingMessageType.SetLoginServer, &l, (uint)sizeof(LoginServerStruct));
        }

        public static void ExecuteMacroType2(uint arg1, ushort arg2, string arg3)
        {
            /* layout of IPC buffer:
             * DWORD lock (used for thread safe access to the buffer)
             * BYTE MessageType (either IncomingMessageType or OutgoingMessageType
             * DWORD len (the length of the following raw byte buffer
             * BYTE[] ^len bytes of raw data */

            /* Each MessageType will have a struct associated with it which
             * describes the data layout for that specific message.  The only
             * exception is for messages which only have one field (i.e. raw data)
             * like the incoming and outgoing packet messages for instance. */

            /* The layout for the Type2Macro message type is:
             * DWORD arg1 (first argument for macro, anyone who remembers old version of EUO will remember...)
             * WORD arg2 second macro argument
             * DWORD text buffer len (length of the following text buffer)
             * BYTE[] text buffer (little endian unicode string) */

            /* There isn't really any 'clean' way to do this one in managed code,
             * but you only have to code it once: */

            ObtainInputBufferAccess();
            // Please NULL terminate your strings
            byte[] arg3Bytes = UnicodeEncoding.Unicode.GetBytes(arg3 + "\0");
            Type2MacroStruct m = new Type2MacroStruct();
            m.arg1 = arg1;
            m.arg2 = arg2;
            m.textLen = (uint)arg3Bytes.Length;
            input[4] = (byte)IncomingMessageType.Type2Macro;
            int structLen = 10 + arg3Bytes.Length;
            *(int*)&input[5] = structLen;
            MemCpy(&input[9], &m, 10);
            fixed (byte* text = &arg3Bytes[0])
                MemCpy(&input[19], text, (uint)arg3Bytes.Length);
            ReleaseInputBuffer();
        }

        public static void AddIncomingPacketFilter(byte packet)
        {
            /* layout of IPC buffer:
             * DWORD lock (used for thread safe access to the buffer)
             * BYTE MessageType (either IncomingMessageType or OutgoingMessageType
             * DWORD len (the length of the following raw byte buffer
             * BYTE[] ^len bytes of raw data */

            ObtainInputBufferAccess();
            try
            {
                input[4] = (byte)IncomingMessageType.AddIncomingPacketFilter;
                *(uint*)&input[5] = 1;
                input[9] = packet;
            }
            finally
            {
                ReleaseInputBuffer();
            }
        }

        public static void RemoveIncomingPacketFilter(byte packet)
        {
            /* layout of IPC buffer:
             * DWORD lock (used for thread safe access to the buffer)
             * BYTE MessageType (either IncomingMessageType or OutgoingMessageType
             * DWORD len (the length of the following raw byte buffer
             * BYTE[] ^len bytes of raw data */

            ObtainInputBufferAccess();
            try
            {
                input[4] = (byte)IncomingMessageType.RemoveIncomingPacketFilter;
                *(uint*)&input[5] = 1;
                input[9] = packet;
            }
            finally
            {
                ReleaseInputBuffer();
            }
        }

        public static void AddOutgoingPacketFilter(byte packet)
        {
            /* layout of IPC buffer:
             * DWORD lock (used for thread safe access to the buffer)
             * BYTE MessageType (either IncomingMessageType or OutgoingMessageType
             * DWORD len (the length of the following raw byte buffer
             * BYTE[] ^len bytes of raw data */

            ObtainInputBufferAccess();
            try
            {
                input[4] = (byte)IncomingMessageType.AddOutgoingPacketFilter;
                *(uint*)&input[5] = 1;
                input[9] = packet;
            }
            finally
            {
                ReleaseInputBuffer();
            }
        }

        public static void RemoveOutgoingPacketFilter(byte packet)
        {
            /* layout of IPC buffer:
             * DWORD lock (used for thread safe access to the buffer)
             * BYTE MessageType (either IncomingMessageType or OutgoingMessageType
             * DWORD len (the length of the following raw byte buffer
             * BYTE[] ^len bytes of raw data */

            ObtainInputBufferAccess();
            try
            {
                input[4] = (byte)IncomingMessageType.RemoveOutgoingPacketFilter;
                *(uint*)&input[5] = 1;
                input[9] = packet;
            }
            finally
            {
                ReleaseInputBuffer();
            }
        }

        static void PressEnterToContinue()
        {
            Console.WriteLine("Press <Enter> to continue...");
            Console.ReadLine();
        }

        public static void SendIpcMessage(IncomingMessageType m)
        {
            ObtainInputBufferAccess();
            try
            {
                input[4] = (byte)m;
                *(uint*)&input[5] = 0;
            }
            finally
            {
                ReleaseInputBuffer();
            }
        }

        public static void SendIpcMessage(IncomingMessageType m, byte[] rawMessage)
        {
            fixed (byte* raw = &rawMessage[0])
            {
                SendIpcMessage(m, raw, (uint)rawMessage.Length);
            }
        }

        public static void SendIpcMessage(IncomingMessageType m, void* rawMessage, uint len)
        {
            ObtainInputBufferAccess();
            try
            {
                input[4] = (byte)m;
                *(uint*)&input[5] = len;
                MemCpy(&input[9], rawMessage, len);
            }
            finally
            {
                ReleaseInputBuffer();
            }
        }

        static void ProcessOutgoingPacket(byte[] buffer)
        {
            if (loggingEnabled)
            {
                Log.LogDataMessage(buffer, "Outgoing packet:\r\n");
            }
        }

        static void ProcessIncomingPacket(byte[] buffer)
        {
            if (loggingEnabled)
            {
                Log.LogDataMessage(buffer, "Incoming packet:\r\n");
            }
        }

        static void ProcessIncomingFilteredPacket(byte[] buffer)
        {
            if (loggingEnabled)
            {
                Log.LogDataMessage(buffer, "[!] Incoming packet DROPPED:\r\n");
            }
            //SendIpcMessage(IncomingMessageType.SendClientPacket, buffer);
            if (IncomingFilterDemo == true && buffer[0] == 0x1C)
            {
                /* Here we demonstrate the modification of a filtered packet.
                 * This packet was never received by the client because it's
                 * been added to the packet blacklist.  However, it's still
                 * sent to us for processing.  Packets sent by MasterControl.dll
                 * aren't subject to the packet filters.  They also aren't sent back
                 * over IPC. */

                string newText = "Hey, hey, hey, it's the big Master Control Program everybody's talking about.\0";
                byte[] newBytes = ASCIIEncoding.ASCII.GetBytes(newText);
                byte[] newPacket = new byte[44 + newBytes.Length];
                Buffer.BlockCopy(buffer, 0, newPacket, 0, 44);
                Buffer.BlockCopy(newBytes, 0, newPacket, 44, newBytes.Length);
                newPacket[1] = (byte)(newPacket.Length);
                newPacket[2] = (byte)(newPacket.Length >> 8);
                SendIpcMessage(IncomingMessageType.SendClientPacket, newPacket);
                return;
            }
            if (IncomingFilterDemo == true && buffer[0] == 0xAE)
            {
                /* Here we demonstrate the modification of a filtered packet.
                 * This packet was never received by the client because it's
                 * been added to the packet blacklist.  However, it's still
                 * sent to us for processing.  Packets sent by MasterControl.dll
                 * aren't subject to the packet filters.  They also aren't sent back
                 * over IPC. */

                string newText = "Hey, hey, hey, it's the big Master Control Program everybody's talking about.\0";
                byte[] newBytes = UnicodeEncoding.BigEndianUnicode.GetBytes(newText);
                byte[] newPacket = new byte[48 + newBytes.Length];
                Buffer.BlockCopy(buffer, 0, newPacket, 0, 48);
                Buffer.BlockCopy(newBytes, 0, newPacket, 48, newBytes.Length);
                if (buffer[9] == 0xC0)
                    newPacket[9] = 0; // not sure if this is actually working as intended...but it might be!
                newPacket[1] = (byte)(newPacket.Length);
                newPacket[2] = (byte)(newPacket.Length >> 8);
                SendIpcMessage(IncomingMessageType.SendClientPacket, newPacket);
            }
        }

        static void ProcessOutgoingFilteredPacket(byte[] buffer)
        {
            if (loggingEnabled)
            {
                Log.LogDataMessage(buffer, "[!] Outgoing packet DROPPED:\r\n");
            }

            if (OutgoingFilterDemo == true && buffer[0] == 0xAD)
            {
                /* Here we demonstrate the modification of a filtered packet.
                 * This packet was never sent by the client because it's
                 * been added to the packet blacklist.  However, it's still
                 * sent to us for processing.  Packets sent by MasterControl.dll
                 * aren't subject to the packet filters.  They also aren't sent back
                 * over IPC. */

                /* In this example we change the text in outgoing speech
                 * packets.  Everything is changed into l33t h4x0R sP34K.
                 * We also use a hackish technique to turn keyword speech
                 * packets into regular ascii speech packets. */

                bool ascii = false;
                if (buffer[3] == 0xC0) //keyword speech
                    ascii = true;
                string oldText;

                if (ascii)
                    oldText = ASCIIEncoding.ASCII.GetString(buffer, 15, buffer.Length - 15);
                else
                    oldText = UnicodeEncoding.BigEndianUnicode.GetString(buffer, 12, buffer.Length - 12);
                string newText = l33t.w00t(oldText);
                byte[] textBytes = UnicodeEncoding.BigEndianUnicode.GetBytes(newText);
                byte[] packet = new byte[textBytes.Length + 12];
                packet[0] = 0xAD;
                packet[1] = (byte)(packet.Length);
                packet[2] = (byte)(packet.Length >> 8);
                Buffer.BlockCopy(buffer, 3, packet, 3, 9);
                if (ascii)
                    packet[3] = 0;
                Buffer.BlockCopy(textBytes, 0, packet, 12, textBytes.Length);
                SendIpcMessage(IncomingMessageType.SendServerPacket, packet);
            }
        }

        static void ProcessOutput(OutgoingMessageType m, byte[] message)
        {
            switch (m)
            {
                case OutgoingMessageType.IncomingPacket:
                    ProcessIncomingPacket(message);
                    break;
                case OutgoingMessageType.OutgoingPacket:
                    ProcessOutgoingPacket(message);
                    break;
                case OutgoingMessageType.IncomingFilteredPacket:
                    ProcessIncomingFilteredPacket(message);
                    break;
                case OutgoingMessageType.OutgoingFilteredPacket:
                    ProcessOutgoingFilteredPacket(message);
                    break;
            }
        }

        static unsafe void OutputThreadProc()
        {
            while (outputContinue)
            {
                WINAPI.WaitForSingleObject(outSemaphore, WINAPI.INFINITE);
                // 0 = free for write access, 1 = in use, 2 = free for read access
                while (2 != WINAPI.InterlockedCompareExchange(outputLock, 1, 2))
                    Thread.Sleep(1); // we should rarely (if ever) be here
                IpcMessage m = output;
                byte[] message = new byte[m.bufferLen];
                try
                {
                    fixed (byte* buffer = &message[0])
                        MemCpy(buffer, m.buffer, (uint)m.bufferLen);
                }
                finally { WINAPI.InterlockedExchange(outputLock, 0); }
                // Avoid blocking this thread, as anything that makes this thread hang will also hang the client
                ManagedThreadPool.QueueUserWorkItem(delegate { ProcessOutput((OutgoingMessageType)m.messageType, message); });
            }
        }

        public static void Stop()
        {
            outputContinue = false;
            if (outputThread != null)
                outputThread.Abort();
            if (outSemaphore != IntPtr.Zero)
                WINAPI.ReleaseSemaphore(outSemaphore, 1, IntPtr.Zero);
        }

        public static bool Initialize(IntPtr hProcess)
        {
            Log.Initialize();
            WINAPI.Inject(hProcess);

            // This struct holds 4 handles we need to get everything up and running
            IpcInfoStruct ipc = new IpcInfoStruct();
            bool success = false;

            // Keep trying until injection is complete
            for (int x = 0; x < 50; x++)
            {
                if (WINAPI.GetIpcInfo(hProcess, ref ipc))
                {
                    success = true;
                    break;
                }
                Thread.Sleep(100);
            }

            if (!success)
                return false;

            /* Getting the IPC system up and running is fairly simple.  Just duplicate a few handles and start a worker
             * thread to process incoming IPC messages.  I use a lightweight, "lock free" system for safely sharing resources
             * amongst different threads.  I use this technique in inline assembly in MasterControl.dll, C++ intriniscs in
             * MasterControl.dll, and also here in C#.  It's based on atomic operations.  It's a fast simple system perfect
             * for the low contention we'll see here.  Any contentions are dealt with via a Sleep(1).  A Sleep(0) or an empty loop
             * could potentially give a faster response time (there shouldn't be any contention anyways), however it causes CPU usage
             * to spike if for some reason the injector application or client.exe should close in the middle of an IPC operation. */

            /* It's a 3 state locking system. 0 = free for write access, 1 = in use, 2 = free for read access.  A write must be followed by a read,
             * which must be followed by a write, etc.  This ensures messages are delivered in order and removes the need for messy queues.
             * In addition to that, signaling is used via semaphores.  This allows us to put the consumer threads to sleep until work is ready to 
             * be done instead of having them try to acquire the lock all the time. */

            IntPtr myProcess = WINAPI.OpenProcess(ProcessAccessFlags.All, false, WINAPI.GetCurrentProcessId());
            if (myProcess == IntPtr.Zero)
                return false;

            if (!WINAPI.DuplicateHandle(hProcess, ipc.inSemaphore, myProcess, out inSemaphore, 0, false, DuplicateOptions.DUPLICATE_SAME_ACCESS))
                return false;

            if (!WINAPI.DuplicateHandle(hProcess, ipc.outSemaphore, myProcess, out outSemaphore, 0, false, DuplicateOptions.DUPLICATE_SAME_ACCESS))
                return false;

            if (!WINAPI.DuplicateHandle(hProcess, ipc.inFileMap, myProcess, out inFileMap, 0, false, DuplicateOptions.DUPLICATE_SAME_ACCESS))
                return false;

            if (!WINAPI.DuplicateHandle(hProcess, ipc.outFileMap, myProcess, out outFileMap, 0, false, DuplicateOptions.DUPLICATE_SAME_ACCESS))
                return false;

            inputView = WINAPI.MapViewOfFile(inFileMap, WINAPI.FILE_MAP_ALL_ACCESS, 0, 0, 0);
            outputView = WINAPI.MapViewOfFile(outFileMap, WINAPI.FILE_MAP_ALL_ACCESS, 0, 0, 0);

            if (inputView == IntPtr.Zero || outputView == IntPtr.Zero)
                return false;

            input = (byte*)inputView.ToPointer();
            output = (byte*)outputView.ToPointer();

            inputLock = (uint*)input;
            outputLock = (uint*)output;

            outputThread = new Thread(new ThreadStart(OutputThreadProc));
            outputThread.Start();

            return true;
        }
        ~IPC()
        {
            WINAPI.CloseHandle(inSemaphore);
            WINAPI.CloseHandle(outSemaphore);
            WINAPI.CloseHandle(inFileMap);
            WINAPI.CloseHandle(outFileMap);
            WINAPI.CloseHandle(inputView);
            WINAPI.CloseHandle(outputView);
        }
    }
}
