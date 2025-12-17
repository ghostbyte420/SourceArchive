using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Win32;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Reflection;
using System.Reflection.Emit;
using System.Net;

namespace MasterControlProgram
{
    unsafe static class Program
    {
        
        // Fix for messed up registry values.  Windows ignores the null unicode termination
        // and returns junk bytes if they exist after the end of the string.  I think the AOS
        // installer is the one that can place a messed up value into registry.
        private static string FixString(string s)
        {
            if (!string.IsNullOrEmpty(s))
                if (s.Contains("\0"))
                    return s.Split('\0')[0];
            return s;
        }

        public static string GetClientPath()
        {
            string path = "";
            RegistryKey rk = Registry.LocalMachine.OpenSubKey(@"Software\Origin Worlds Online\Ultima Online\1.0", false);
            if (rk != null)
            {
                path = (string)rk.GetValue("ExePath");
                rk.Close();
            }
            return FixString(path);
        }

        public static void PressEnterToContinue()
        {
            Console.WriteLine("Press <Enter> to continue...");
            Console.ReadLine();
        }

        static IPAddress Resolve(string hostname)
        {
            IPAddress ip = IPAddress.None;
            if (!string.IsNullOrEmpty(hostname))
            {
                if (!IPAddress.TryParse(hostname, out ip))
                {
                    try
                    {
                        IPHostEntry entry = Dns.GetHostEntry(hostname);
                        if (entry.AddressList.Length > 0)
                        {
                            ip = entry.AddressList[entry.AddressList.Length - 1];
                        }
                    }
                    catch { }
                }
            }
            return ip;
        }

        static void ClientPacketTest()
        {
            string message = "Sit right there; make yourself comfortable. Remember the time we used to spend playing chess together?\0";
            byte[] baseSystemMessagePacket = new byte[] {   
            0xAE, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
            0xFF, 0x00, 0x03, 0xB2, 0x00, 0x03, 0x45, 0x4E, 
            0x55, 0x00, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6D, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            byte[] textBytes = UnicodeEncoding.BigEndianUnicode.GetBytes(message);
            byte[] packet = new byte[baseSystemMessagePacket.Length + textBytes.Length];
            Buffer.BlockCopy(baseSystemMessagePacket, 0, packet, 0, baseSystemMessagePacket.Length);
            Buffer.BlockCopy(textBytes, 0, packet, baseSystemMessagePacket.Length, textBytes.Length);
            packet[1] = (byte)(packet.Length);
            packet[2] = (byte)(packet.Length >> 8);
            IPC.SendIpcMessage(IncomingMessageType.SendClientPacket, packet);
        }

        static void ServerPacketTest()
        {
            string message = "You're in trouble, program. Why don't you make it easy on yourself. Who's your user?\0";
            byte[] textBytes = UnicodeEncoding.BigEndianUnicode.GetBytes(message);
            byte[] packet = new byte[textBytes.Length + 12];
            packet[0] = 0xAD;
            packet[1] = (byte)(packet.Length);
            packet[2] = (byte)(packet.Length >> 8);
            packet[3] = 0; //say
            packet[4] = 0x00; //color
            packet[5] = 0x58;
            packet[6] = 0x00; //font
            packet[7] = 0x03;
            packet[8] = 0x45; //ENU\0
            packet[9] = 0x4E;
            packet[10] = 0x55;
            packet[11] = 0x00;
            Buffer.BlockCopy(textBytes, 0, packet, 12, textBytes.Length);
            IPC.SendIpcMessage(IncomingMessageType.SendServerPacket, packet);
        }

        static void HandleInput(string option)
        {
            switch (option)
            {
                case "1": 
                    ClientPacketTest();
                    DisplayMenu();
                    break;
                case "2":
                    ServerPacketTest();
                    DisplayMenu();
                    break;
                case "3":
                    IPC.CallGumpFunction(0, 0);
                    DisplayMenu();
                    break;
                case "4":
                    // Hardcoded location for sample app
                    ushort x = 3652;
                    ushort y = 2628;
                    Random r = new Random(DateTime.Now.Millisecond);
                    x += (ushort)r.Next(10);
                    x -= (ushort)r.Next(10);
                    y += (ushort)r.Next(10);
                    y -= (ushort)r.Next(10);
                    IPC.CallPathfindFunction(x, y, 0);
                    DisplayMenu();
                    break;
                case "5":
                    Random r2 = new Random(DateTime.Now.Millisecond);
                    ushort direction = (ushort)r2.Next(8);
                    IPC.ExecuteMacroType1(5, direction);
                    IPC.ExecuteMacroType1(5, direction);
                    IPC.ExecuteMacroType1(5, direction);
                    IPC.ExecuteMacroType1(5, direction);
                    DisplayMenu();
                    break;
                case "6":
                    IPC.ExecuteMacroType2(3, 0, "I'm going to have to put you on the game grid.");
                    DisplayMenu();
                    break;
                case "7":
                    IPC.IncomingFilterDemo = true;
                    IPC.AddIncomingPacketFilter(0x1C); // packet 1C = ascii text
                    IPC.AddIncomingPacketFilter(0xAE);  // unicode text
                    DisplayMenu();
                    break;
                case "8":
                    IPC.IncomingFilterDemo = false;
                    IPC.RemoveIncomingPacketFilter(0x1C); // packet 1C = ascii text
                    IPC.RemoveIncomingPacketFilter(0xAE);  // unicode text
                    DisplayMenu();
                    break;
                case "9":
                    IPC.OutgoingFilterDemo = true;
                    IPC.AddOutgoingPacketFilter(0xAD); // packet AD = unicode speech
                    DisplayMenu();
                    break;
                case "10":
                    IPC.OutgoingFilterDemo = false;
                    IPC.RemoveOutgoingPacketFilter(0xAD); // packet AD = unicode speech
                    DisplayMenu();
                    break;
                case "11":
                    IPC.LoggingEnabled = true;
                    DisplayMenu();
                    break;
                case "12":
                    IPC.LoggingEnabled = false;
                    DisplayMenu();
                    break;
                case "13":
                    IPC.Stop();
                    break;
                default:
                    DisplayMenu();
                    break;
            }
        }

        static void DisplayMenu()
        {
            Console.Clear();
            Console.WriteLine("Welcome to the MasterControl.dll injector demo!");
            Console.WriteLine("Please select from the following options AFTER YOU'VE LOGGED IN: ");
            Console.WriteLine();
            Console.WriteLine("1.)  Send system message to client (send raw packet to client example)");
            Console.WriteLine("2.)  Send unicode speech packet to server (send raw packet to server example)");
            Console.WriteLine("3.)  Close top gump (gump function execution example)");
            Console.WriteLine("4.)  Pathfind to hardcoded x,y,z. (use client pathfind function example)");
            Console.WriteLine("5.)  Move player a few steps in random direction (EUO macro style 1 example)");
            Console.WriteLine("6.)  Whisper some text (EUO macro style 2 example)");
            Console.WriteLine("7.)  Modify incoming text (incoming packet filter + modification example)");
            Console.WriteLine("8.)  Turn off incoming packet filter demo above");
            Console.WriteLine("9.)  l33tsp34k your ass off (outgoing packet filter + modification example)");
            Console.WriteLine("10.) Turn off outgoing packet filter demo above");
            Console.WriteLine("11.) Enable packet logger (SENSITIVE INFORMATION MAY BE LOGGED!)");
            Console.WriteLine("12.) Disable packet logger (disabled by default)");
            Console.WriteLine("13.) Exit");
            Console.WriteLine();
            Console.Write("Option (1-13): ");
            string input = Console.ReadLine().Trim();
            HandleInput(input);
        }

        static void Main(string[] args)
        {
            SafeProcessHandle hProcess = null;
            SafeThreadHandle hThread = null;
            int pid = 0;
            int tid = 0;


            // Attach to running client if available
            /*
            Process[] clients = Process.GetProcessesByName("client");
            if (clients != null && clients.Length > 0)
            {
                clients[0].EnableRaisingEvents = true;
                clients[0].Exited += new EventHandler(p_Exited);
                hProcess = new SafeProcessHandle(WINAPI.OpenProcess(ProcessAccessFlags.All, false, clients[0].Id));
                if (!hProcess.IsInvalid)
                {
                    if (0 != WINAPI.NtSuspendProcess(hProcess.DangerousGetHandle()))
                    {
                        hProcess = null;
                        Console.WriteLine(string.Format("NtSuspendProcess error: {0}\r\n", Marshal.GetLastWin32Error()));
                        Console.WriteLine("Error suspending existing client process, launching new client instead...");
                        PressEnterToContinue();
                    }
                }
            }*/

            // Start new client if attaching failed
            if (hProcess == null)
            {
                string clientPath = GetClientPath();
                if (string.IsNullOrEmpty(clientPath))
                {
                    Console.WriteLine("Path for client.exe not found!");
                    PressEnterToContinue();
                    return;
                }

                ProcessStartInfo startInfo = new ProcessStartInfo();
                startInfo.WorkingDirectory = Path.GetDirectoryName(clientPath);
                startInfo.FileName = clientPath;

                if (!WINAPI.CreateProcess(startInfo, true, out hProcess, out hThread, out pid, out tid))
                {
                    Console.WriteLine(string.Format("CreateProcess error: {0}\r\n", Marshal.GetLastWin32Error()));
                    PressEnterToContinue();
                    return;
                }
            }

            Thread.Sleep(1000);

            // This project is -not- officially endorsed by UOSA -and- it's against the rules to use this
            // application on their server, but I had to put some kind of example here
            int serverLong = 0;
            IPAddress server = Resolve("uosecondage.com");
            if (server != IPAddress.None)
                serverLong = IPAddress.HostToNetworkOrder(BitConverter.ToInt32(server.GetAddressBytes(), 0));

            // Set up simple event handler to close us down with the client
            Process p = null;
            if (pid != 0)
            {
                p = Process.GetProcessById((int)pid);
                p.EnableRaisingEvents = true;
                if (p != null)
                    p.Exited += new EventHandler(p_Exited);
            }

            if (hProcess != null && IPC.Initialize(hProcess.DangerousGetHandle()))
            {
                // Patch client encryption (doesn't matter if it's already patched)
                // It's recommended to do patches while the client is still suspended
                IPC.SendIpcMessage(IncomingMessageType.PatchEncryption);
                // Patch in our own login server
                if (serverLong != 0)
                    IPC.SetLoginServer((uint)serverLong, 2593);

                Thread.Sleep(500);

                /*if (pid == 0)
                {
                    //if we're here it means we attached to the process
                    if (0 != WINAPI.NtResumeProcess(hProcess.DangerousGetHandle()))
                    {
                        WINAPI.TerminateProcess(hProcess.DangerousGetHandle(), 310);
                        hProcess.Dispose();
                        hThread.Dispose();
                        Console.WriteLine(string.Format("NtResumeProcess error: {0}\r\n", Marshal.GetLastWin32Error()));
                        PressEnterToContinue();
                        return;
                    }

                }*/

                //start up suspended client after we're done with injection
                if (pid != 0 && WINAPI.ResumeThread(hThread.DangerousGetHandle()) == WINAPI.INVALID_HANDLE_VALUE)
                {
                    WINAPI.TerminateProcess(hProcess.DangerousGetHandle(), 310);
                    hProcess.Dispose();
                    hThread.Dispose();
                    Console.WriteLine(string.Format("ResumeThread error: {0}\r\n", Marshal.GetLastWin32Error()));
                    PressEnterToContinue();
                    return;
                }
            }
            else if (hProcess != null)
            {
                Console.WriteLine("Error attaching to client; it may be an incompatible version. ");
                PressEnterToContinue();
                WINAPI.TerminateProcess(hProcess.DangerousGetHandle(), 310); //3!0
            }

            DisplayMenu();
            IPC.Stop();
            Log.Dispose();
            if (hProcess != null)
                WINAPI.TerminateProcess(hProcess.DangerousGetHandle(), 0);
            hProcess.Close();
            if (hThread != null)
                hThread.Close();
        }

        static void p_Exited(object sender, EventArgs e)
        {
            IPC.Stop();
            Log.Dispose();
            Environment.Exit(0);
        }
    }
}
