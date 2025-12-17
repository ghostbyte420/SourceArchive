using System;
using System.Runtime.InteropServices;

namespace MasterControlProgram
{
    enum OutgoingMessageType : byte
    {
        OutgoingMessage = 1,
        IncomingPacket,
        OutgoingPacket,
        IncomingFilteredPacket,
        OutgoingFilteredPacket
    };

    enum IncomingMessageType : byte
    {
        IncomingMessage = 1,
        SendClientPacket,
        SendServerPacket,
        CallGumpFunction,
        Pathfind,
        Type1Macro, // 2 parameter
        Type2Macro, // 3 parameter
        AddIncomingPacketFilter,
        RemoveIncomingPacketFilter,
        AddOutgoingPacketFilter,
        RemoveOutgoingPacketFilter,
        PatchEncryption,
        SetLoginServer
    };


    [StructLayout(LayoutKind.Sequential)]
    struct Type1MacroStruct
    {
        public uint arg1;
        public ushort arg2;
    };

    [StructLayout(LayoutKind.Sequential)]
    unsafe struct Type2MacroStruct
    {
        public uint arg1;
        public ushort arg2;
        public uint textLen;
        //public byte[] text; //unicode
    };

    [StructLayout(LayoutKind.Sequential)]
    struct CallGumpFunctionStruct
    {
        public uint gumpHandle;    // Handle of gump to interact with. 0 = top gump
        public uint functionIndex; // vTable function index to call.  Incorrect values -will- cause nasty client crashes.
    };

    [StructLayout(LayoutKind.Sequential, Pack = 2)]
    struct PathfindStruct
    {
        public ushort x;
        public ushort y;
        public ushort z;
    };

    [StructLayout(LayoutKind.Sequential)]
    struct IpcInfoStruct
    {
        public IntPtr inFileMap;
        public IntPtr outFileMap;
        public IntPtr inSemaphore;
        public IntPtr outSemaphore;
    };

    [StructLayout(LayoutKind.Sequential)]
    struct LoginServerStruct 
    {
		public uint server;
		public ushort port;
	};

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    unsafe struct IpcMessage
    {
        public int bufferLock;
        public byte messageType;
        public int bufferLen;
        public byte* buffer;

        public static implicit operator IpcMessage(byte* source)
        {
            return new IpcMessage(source);
        }

        public IpcMessage(byte* source)
        {
            bufferLock = *(int*)&source[0];
            messageType = source[4];
            bufferLen = *(int*)&source[5];
            buffer = source + 9;
        }
    };
}