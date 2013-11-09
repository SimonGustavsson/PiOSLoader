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
        private const string WINDOW_TITLE = "PiOS Deployer";
        private SerialPort mSerial;
        private bool mSizeConfirmationRecevied = false;
        byte[] mSizeConfirmation = new byte[4];
        private int mSizeConfirmationIndex = 0;
        private byte[] mKernelBytes = new byte[20];
        private int mKernelByteIndex = 0;
        private byte[] mKernelBuffer;
        private int mKernelSize = 0;

        public void Run(string pathToKernelImage)
        {
            if (!File.Exists(pathToKernelImage))
            {
                Console.WriteLine("Who're you trying to fool!? There's no kernel there!");
                return;
            }

            Console.Title = WINDOW_TITLE;

            InitializeSerialPort();

            // Tell the Pi we've connected by sending a byte
            mSerial.Write(new byte[] { 1 }, 0, 1);
            
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

            Console.WriteLine("Kernel size sent!");

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
                if (mSizeConfirmationIndex != 3)
                {
                    // We're receiving size
                    for (int i = 0; i < mSerial.BytesToRead && mSizeConfirmationIndex < 4; i++)
                    {
                        mSizeConfirmation[mSizeConfirmationIndex++] = (byte)mSerial.ReadByte();
                    }

                    if(mSizeConfirmationIndex == 3)
                    {
                        // We've received confirmation of the size of the kernel, send the kernel
                        mSerial.Write(mKernelBuffer, 0, mKernelSize);

                        Console.WriteLine("Kernel sent to the Pi!");
                    }
                }
                else
                {
                    for (var i = 0; i < mSerial.BytesToRead; i++)
                        mKernelBytes[mKernelByteIndex++] = (byte)mSerial.ReadByte();
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

                    if (command.Equals("x", StringComparison.InvariantCultureIgnoreCase))
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

                if (caption.IndexOf("Prolific") > -1)
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
