﻿using System;

namespace PiOSDeployer
{
    class Program
    {
        private const string WINDOW_TITLE = "PiOS Deployer";

        static void Main(string[] args)
        {
            Console.Title = WINDOW_TITLE;

            if(args.Length < 1)
            {
                Console.WriteLine("Too few arguments, at least tell me which kernel to deploy, man!");
                return;
            }

            new DeployerOfWorlds().Run(args[0]);
        }
    }
}