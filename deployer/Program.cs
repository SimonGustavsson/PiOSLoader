using System;
using System.Diagnostics;

namespace PiOSDeployer
{
    class Program
    {
        private const string WINDOW_TITLE = "PiOS Deployer";
        private const int DEFAULT_BAUD_RATE = 460800;

        static void Main(string[] args)
        {
            Console.Title = WINDOW_TITLE;

            if(args.Length < 1)
            {
                Console.WriteLine("Too few arguments, at least tell me which kernel to deploy, man!");
                return;
            }

            int baud = DEFAULT_BAUD_RATE;
            if(args.Length > 1 && !int.TryParse(args[1], out baud))
            {
                // Second arg should be baud rate
                Console.WriteLine("Invalid baud rate passed to deployer {0}. Using default {1}", args[1], DEFAULT_BAUD_RATE);
            }

            Process.GetCurrentProcess().DieWithParent();

            new DeployerOfWorlds().Run(args[0], baud);
        }

        static void PrintHelp()
        {
            Console.WriteLine("");
            Console.WriteLine("PiOS Deployer");
            Console.WriteLine("Usage: PiOSDeployer.exe <kernel path> <baud rate>");
            Console.WriteLine("");
        }
    }
}