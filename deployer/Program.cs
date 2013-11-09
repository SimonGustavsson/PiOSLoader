using System;

namespace PiOSDeployer
{
    class Program
    {
        static void Main(string[] args)
        {
            if(args.Length < 2 && !System.Diagnostics.Debugger.IsAttached)
            {
                Console.WriteLine("Too few arguments, at least tell me which kernel to deploy, man!");
                return;
            }

            new DeployerOfWorlds().Run(args[1]);
        }
    }
}