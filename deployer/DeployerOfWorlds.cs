using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Management;
using System.Text;
using System.Threading.Tasks;

namespace PiOSDeployer
{
    class DeployerOfWorlds
    {
        private SerialPort mSerial;
        private int mKernelByteIndex = 0;
        private byte[] mKernelBuffer;
        private int mKernelSize = 0;
        private byte[] mReceiveBuffer = new byte[40];
        private int mReceiveIndex = 0;
        private bool mKernelLoaded;

        public void Run(string pathToKernelImage)
        {
            if (!File.Exists(pathToKernelImage))
            {
                Console.WriteLine("Who're you trying to fool!? There's no kernel there!");
                return;
            }

            InitializeSerialPort();
            
            try
            {
                // Send kernel size
                mKernelSize = (int)new FileInfo(pathToKernelImage).Length;

                // Send kernel
                mKernelBuffer = new byte[mKernelSize];
                using (var fs = File.OpenRead(pathToKernelImage))
                {
                    fs.Read(mKernelBuffer, 0, (int)mKernelSize);
                }
            }
            catch(Exception e)
            {
                Console.WriteLine("Could not read kernel from disk, {0}", e.Message);
            }

            // Write Size, then the data
            mSerial.Write(BitConverter.GetBytes(mKernelSize), 0, 4);

            Console.Write("Kernel size sent, waiting for confirmation...");

            InifiniteCommandLoop();
        }

        private void InitializeSerialPort()
        {
            int port = -1;
            while (true)
            {
                port = GetComPort();

                if (port != -1)
                    break;

                Console.WriteLine("Pi not connected, dummy! :) Connect the pi and hit any key.");
                Console.ReadLine();
            }

            mSerial = new SerialPort("COM" + port.ToString(), 115200, Parity.None, 8, StopBits.One);
            mSerial.DataReceived += (s, e) =>
            {
                var bytesToRead = mSerial.BytesToRead; // Decrements when reading, so freeze it
                if (mKernelLoaded)
                {
                    for (var i = 0; i < bytesToRead; i++)
                        Console.Write((char)mSerial.ReadByte());

                    return;
                }

                for (var i = 0; i < bytesToRead; i++)
                    mReceiveBuffer[mReceiveIndex++] = (byte)mSerial.ReadByte();
                
                // Was the last one a 0 indicating EOF
                if (mReceiveBuffer[mReceiveIndex - 1] == 0)
                {
                    var receivedString = Encoding.UTF8.GetString(mReceiveBuffer, 0, mReceiveIndex - 1); // (Don't read \0)
                    switch (receivedString.ToUpper())
                    {
                        case "SIZEOK":
                            Console.Write(" Done!{0}Sending kernel...", Environment.NewLine);

                            mSerial.Write(mKernelBuffer, 0, mKernelSize);

                            Console.Write(" Done!" + Environment.NewLine);
                            break;
                        case "BADSIZE":
                            Console.WriteLine("Kernel is too big. :(");
                            break;
                        case "KRNOK":
                            mKernelLoaded = true;
                            Console.WriteLine("Kernel loaded, press any key to continue.");
                            break;
                    }
                    mReceiveIndex = 0;
                    Array.Clear(mReceiveBuffer, 0, 40);
                }
            };

            mSerial.Open();
        }

        private void InifiniteCommandLoop()
        {
            while (true)
            {
                var command = Console.ReadLine();
                if (!String.IsNullOrEmpty(command))
                {
                    var bytes = System.Text.Encoding.UTF8.GetBytes(command);

                    mSerial.Write(bytes, 0, bytes.Length);

                    if (command.Equals("exit", StringComparison.InvariantCultureIgnoreCase))
                    {
                        break;
                    }
                }
            }
        }

        private int GetComPort()
        {
            foreach (ManagementObject obj in new ManagementObjectSearcher("root\\CIMV2", "SELECT * FROM Win32_PnPEntity").Get())
            {
                var caption = obj["Caption"] as string;

                if (caption!= null && caption.IndexOf("Prolific") > -1)
                {
                    var comPort = caption.Substring(caption.IndexOf("COM") + 3, 1);
                    int com;
                    if (int.TryParse(comPort, out com))
                    {
                        return com;
                    }
                }
            }

            return -1;
        }
    }
}
