using System;
using System.IO;
using System.Text;
using System.Collections.Generic;

namespace MasterControlProgram
{
    unsafe class Log
    {
        private static StreamWriter myFileWriter;
        private static string myLogFileName;

        internal static void Initialize()
        {
            StringBuilder sb = new StringBuilder(DateTime.Now.ToString());
            char[] invalid = Path.GetInvalidFileNameChars();
            for (int x = 0; x < sb.Length; x++)
                for (int y = 0; y < invalid.Length; y++)
                    if (sb[x] == invalid[y])
                        sb[x] = '.';
            sb.Append(".Log.txt");
            Log.Initialize(sb.ToString());
        }

        internal static void Initialize(string logFileName)
        {
            myLogFileName = logFileName;
            myFileWriter = File.CreateText(logFileName);
            myFileWriter.AutoFlush = true;
        }

        private static string GetStringFromBytes(byte[] data)
        {
            fixed (byte* buffer = &data[0])
            {
                return GetStringFromBytes(buffer, data.Length);
            }
        }

        private static string GetStringFromBytes(byte *data, int len)
        {
            StringBuilder sb = new StringBuilder(len * 5);
            int index = 0;
            bool prefix = true;

            for (int x = 0; x < len; x++)
            {
                if (prefix)
                {
                    sb.AppendFormat("{0:X4}: ", x);
                    prefix = false;
                }
                sb.AppendFormat("{0:X2} ", data[x]);
                index++;
                if (index == 16)
                {
                    sb.Append("| ");
                    for (int y = x - 15; y <= x; y++)
                    {
                        if (data[y] > 31 && data[y] < 127)
                            sb.Append((char)data[y]);
                        else
                            sb.Append('.');
                    }
                    if (x != len - 1) //don't leave extra newline
                        sb.Append("\r\n");
                    index = 0;
                    prefix = true;
                }
            }
            if (index != 0)
            {
                for (int x = index; x < 16; x++)
                    sb.Append("   ");
                sb.Append("| ");
                for (int x = len - index; x < len; x++)
                {
                    if (data[x] > 31 && data[x] < 127)
                        sb.Append((char)data[x]);
                    else
                        sb.Append('.');
                }
            }
            return sb.ToString();
        }

        private static string GetDateString()
        {
            return String.Format("[ ]========================[ {0} ]========================[ ]\r\n", DateTime.Now);
        }

        public static void LogDataMessage(byte *data, int len, string message)
        {
            string dataLen = String.Format("Length of data: {0} bytes (0x{1:X})\r\n", len, len);
            string logMessage = String.Format("{0}{1}{2}{3}\r\n\r\n", GetDateString(), message, dataLen, GetStringFromBytes(data, len));
            lock (myFileWriter) { myFileWriter.Write(logMessage); }
        }

        public static void LogDataMessage(byte[] data, string message)
        {
            fixed (byte* buffer = &data[0])
            {
                LogDataMessage(buffer, data.Length, message);
            }
        }

        public static void LogMessage(string message)
        {
            string logMessage = String.Format("{0}{1}\r\n\r\n", GetDateString(), message);
            lock (myFileWriter) { myFileWriter.Write(logMessage); }
        }

        public static void LogMessage(Exception exception)
        {
            string message = String.Format("{0}\r\n{1}\r\n{2}", exception.Message, exception.Source, exception.StackTrace);
            string logMessage = String.Format("{0}Exception:\r\n{1}\r\n\r\n", GetDateString(), message);
            lock (myFileWriter) { myFileWriter.Write(logMessage); }
        }

        public static void Dispose()
        {
            try
            {
                if (myFileWriter != null)
                {
                    myFileWriter.Flush();
                    myFileWriter.Close();
                    FileInfo fi = new FileInfo(myLogFileName);
                    if (fi.Exists && fi.Length == 0)
                        File.Delete(myLogFileName);
                }
            }
            catch (ObjectDisposedException) { }
        }

        ~Log()
        {
            try
            {
                if (myFileWriter != null)
                {
                    myFileWriter.Flush();
                    myFileWriter.Close();
                    FileInfo fi = new FileInfo(myLogFileName);
                    if (fi.Exists && fi.Length == 0)
                        File.Delete(myLogFileName);
                }
            }
            catch (ObjectDisposedException) { }
        }
    }
}
