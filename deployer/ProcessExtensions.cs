using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PiOSDeployer
{
    public static class ProcessExtensions
    {
        /// <summary>
        /// Gets the parent process.
        /// </summary>
        /// Parent() Extension courtesy of http://stackoverflow.com/questions/394816/how-to-get-parent-process-in-net-in-managed-way
        public static Process Parent(this Process process)
        {
            return FindPidFromIndexedProcessName(FindIndexedProcessName(process.Id));
        }

        /// <summary>
        /// Exits the current process when the parent process dies.
        /// </summary>
        public static bool DieWithParent(this Process process)
        {
            try
            {
                var parent = process.Parent();

                parent.EnableRaisingEvents = true;
                parent.Exited += (s, e) => 
                { 
                    if(!process.CloseMainWindow())
                    {
                        process.Kill();
                    }
                };
            }
            catch(Exception ex)
            {
                // Pokemon because it doesn't really matter (we might not have been launched through VS)
                Debug.WriteLine("Failed to die with parent: {0}", ex.Message);

                return false;
            }

            return true;
        }

        private static string FindIndexedProcessName(int pid)
        {
            var processName = Process.GetProcessById(pid).ProcessName;
            var processesByName = Process.GetProcessesByName(processName);
            string processIndexdName = null;

            for (var index = 0; index < processesByName.Length; index++)
            {
                processIndexdName = index == 0 ? processName : processName + "#" + index;
                var processId = new PerformanceCounter("Process", "ID Process", processIndexdName);
                if ((int)processId.NextValue() == pid)
                {
                    return processIndexdName;
                }
            }

            return processIndexdName;
        }

        private static Process FindPidFromIndexedProcessName(string indexedProcessName)
        {
            var parentId = new PerformanceCounter("Process", "Creating Process ID", indexedProcessName);
            return Process.GetProcessById((int)parentId.NextValue());
        }
    }
}
