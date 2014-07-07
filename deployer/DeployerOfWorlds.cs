using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.IO.Ports;
using System.Management;
using System.Runtime.InteropServices;
using System.Text;

namespace PiOSDeployer
{
    internal class DeployerOfWorlds
    {
        private SerialPort mSerial;
        private byte[] mKernelBuffer;
        private int mKernelSize = 0;
        private byte[] mReceiveBuffer = new byte[2048];
        private int mReceiveIndex = 0;
        private string mReceivedString = String.Empty;
        private bool mSendingKernel = false;

        public void Run(string pathToKernelImage, int baudRate)
        {
            if (!File.Exists(pathToKernelImage))
            {
                Console.WriteLine("Who're you trying to fool!? There's no kernel there!");
                return;
            }

            NativeMethods.SetConsoleCtrlHandler(new HandlerRoutine(ConsoleCtrlCheck), true);

            if (!InitializeSerialPort(baudRate))
                return;

            try
            {
                // Send kernel size
                mKernelSize = (int)new FileInfo(pathToKernelImage).Length;

                // Read kernel into buffer so we can send it
                mKernelBuffer = new byte[mKernelSize];
                using (var fs = File.OpenRead(pathToKernelImage))
                {
                    fs.Read(mKernelBuffer, 0, (int)mKernelSize);
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("Could not read kernel from disk, {0}", e.Message);
            }

            Console.WriteLine("Sending file {0} ({1} bytes) at {2} baud.", pathToKernelImage, mKernelSize, baudRate);

            // Write Size, then the data
            this.Send(BitConverter.GetBytes(mKernelSize));

            Console.Write("Kernel size sent, waiting for confirmation...");

            InifiniteCommandLoop();
        }

        private bool InitializeSerialPort(int baudRate)
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

            try
            {
                mSerial = new SerialPort("COM" + port.ToString(), baudRate, Parity.None, 8, StopBits.One);
            }
            catch(Exception)
            {
                Console.WriteLine("Failed to create serial port. Pi connected to the correct USB port?");

                Console.WriteLine("Press any key to exit...");

                Console.ReadLine();
            }
            
            mSerial.DataReceived += Serial_DataReceived;

            try
            {
                mSerial.Open();
            }
            catch(UnauthorizedAccessException)
            {
                Console.WriteLine("The COM port is already in use, is another instance of the deployer running?");
                return false;
            }

            return true;
        }

        private void Serial_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            var bytesToRead = mSerial.BytesToRead; // Decrements when reading, so freeze it
            Debug.WriteLine("Bytes to read: {0}", bytesToRead);

            // Note that we can receive several events with partial data,
            // Keep receiving data until we get a null terminator
            for (var i = 0; i < bytesToRead; i++)
            {
                // We might receive more data than the buffer can hold
                if(mReceiveIndex >= mReceiveBuffer.Length - 1)
                {
                    mReceivedString += Encoding.UTF8.GetString(mReceiveBuffer, 0, mReceiveIndex - 1);
                    Array.Clear(mReceiveBuffer, 0, mReceiveBuffer.Length);
                    mReceiveIndex = 0;
                }

                mReceiveBuffer[mReceiveIndex++] = (byte)mSerial.ReadByte();
            }
            // Was the last one a 0 indicating EOF
            // NOTE: This means the kernel is currently not allowed to send data!
            if (mReceiveBuffer[mReceiveIndex - 1] != 0)
                return; // We got more to receive yet

            mReceivedString += Encoding.UTF8.GetString(mReceiveBuffer, 0, mReceiveIndex - 1); // (Don't read \0)
            mReceivedString = mReceivedString.Replace("\0", "");

            switch (mReceivedString.ToUpper())
            {
                case "SIZEOK":
                    Console.Write(" Done!{0}Sending kernel...", Environment.NewLine);

                    mSendingKernel = true;

                    this.Send(mKernelBuffer); // Do we need to pass in kernel size?

                    mSendingKernel = false;

                    Console.Write(" Done!" + Environment.NewLine);
                    break;
                case "BADSIZE": Console.WriteLine("Kernel is too big. :("); break;
                case "KRNOK": Console.WriteLine("Kernel accepted, booting..."); break;
                case "STRT":
                    // Occurs when the Pi is powercycled - Redeploy kernel
                    Console.WriteLine("Bootloader restarted: Sending kernelsize");

                    // Resend the kernel size (kernel is sent in receive handler)
                    this.Send(BitConverter.GetBytes(mKernelSize));
                    break;
                default:
                    Console.Write(mReceivedString);
                    break;
            }

            mReceivedString = String.Empty;

            // Clear receive buffer in preparation of more data
            mReceiveIndex = 0;
            Array.Clear(mReceiveBuffer, 0, 40);
        }

        private void InifiniteCommandLoop()
        {
            while (true)
            {
                var key = Console.ReadKey(true);

                if (key.Key == ConsoleKey.Escape)
                    break;

                if (!this.ShouldSendKey(key))
                    continue;

                this.Send(new byte[] { (byte)key.KeyChar });
            }
        }

        private int GetComPort()
        {
            foreach (ManagementObject obj in new ManagementObjectSearcher("root\\CIMV2", "SELECT * FROM Win32_PnPEntity").Get())
            {
                var caption = obj["Caption"] as string;

                if (caption != null && caption.IndexOf("Prolific") > -1)
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

        private bool ConsoleCtrlCheck(CtrlTypes ctrlType)
        {
            if (ctrlType == CtrlTypes.CTRL_CLOSE_EVENT)
            {
                // Tell PiOS to restart so that it's ready to receive a new kernel
                this.Send(new byte[] { (byte)'x' });
            }

            return true;
        }

        private void Send(byte[] data)
        {
            if (!mSerial.IsOpen)
            {
                Console.WriteLine("Could not write data as serial port is closed.");
                return;
            }
            try
            {
                mSerial.Write(data, 0, data.Length);
            }
            catch (IOException ex)
            {
                Console.WriteLine("Failed to write to serial, {0}.", ex.Message);
            }
        }

        private bool ShouldSendKey(ConsoleKeyInfo keyInfo)
        {
            // Only send printable keys
            return !mSendingKernel && keyInfo.KeyChar != 0;
        }
    }
}
