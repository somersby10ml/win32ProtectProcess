using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Security.Principal;
using System.IO;
using System.Runtime.InteropServices;

namespace MyDriverControl
{
    public partial class Form1 : Form
    {
        int ProcessID;

        // 이건 드라이버가 사용하는 링크 이름임
        String myName = "MyDevice";


        // 내가 사용할 서비스이름
        String myServiceName = "AAA_BBB";
        String myDisplayName = "AAA_BBB";

        // 드라이버 경로
        String driverPath = Directory.GetCurrentDirectory() + @"\MyDriver1.sys";

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {



            ProcessID = Process.GetCurrentProcess().Id;
            addLog("Process ID : " + ProcessID.ToString());

            bool isElevated;
            using (WindowsIdentity identity = WindowsIdentity.GetCurrent())
            {
                WindowsPrincipal principal = new WindowsPrincipal(identity);
                isElevated = principal.IsInRole(WindowsBuiltInRole.Administrator);

                if(isElevated)
                {
                    addLog("Admin : True");
                }
                else
                {
                    addLog("Admin : False");
                }
            }

        }

        private void addLog(string str, int level = 0)
        {
            
            for(int i=0; i<level; i++)
            {
                str = "  " + str;
            }
            
            if(textBox1.Text == "")
            {
                textBox1.Text = str;
            }
            else
            {
                textBox1.AppendText("\r\n" + str);
                //textBox1.Text =  textBox1.Text +;
            }
            textBox1.Select(0, 0);
        }

        private void button1_Click(object sender, EventArgs e)
        {
            addLog("[Driver Install]");

           



            IntPtr hServiceManager = api.OpenSCManager(null, null, api.SC_MANAGER_ALL_ACCESS);
            if (hServiceManager == IntPtr.Zero)
            {
                Int32 err = Marshal.GetLastWin32Error();
                addLog("OpenSCManager Error Number : " + err.ToString(), 1);
                addLog("Error Message : " + api.GetErrorMessage(err), 1);
                return;
            }


            IntPtr hService = api.CreateService(
                hServiceManager,
                myServiceName,
                myDisplayName,
                api.SERVICE_ACCESS.SERVICE_ALL_ACCESS,
                api.SERVICE_TYPE.SERVICE_KERNEL_DRIVER,
                api.SERVICE_START.SERVICE_DEMAND_START,
                api.SERVICE_ERROR.SERVICE_ERROR_NORMAL,
                driverPath,
                null,
                null,
                null,
                null,
                null
            );
            if(hService != IntPtr.Zero)
            {
                addLog("CreateService Successful", 1);
            }
            else
            {
                Int32 err = Marshal.GetLastWin32Error();
                addLog("CreateService Error Number : " + err.ToString(), 1);
                addLog("Error Message : " + api.GetErrorMessage(err), 1);
            }
            api.CloseServiceHandle(hService);
            api.CloseServiceHandle(hServiceManager);
        }

        private void button4_Click(object sender, EventArgs e)
        {
            addLog("[Driver UnInstall]");

            IntPtr hServiceManager = api.OpenSCManager(null, null, api.SC_MANAGER_ALL_ACCESS);
            if (hServiceManager == IntPtr.Zero)
            {
                Int32 err = Marshal.GetLastWin32Error();
                addLog("OpenSCManager Error Number : " + err.ToString(), 1);
                addLog("Error Message : " + api.GetErrorMessage(err), 1);
                return;
            }

            IntPtr hService = api.OpenService(hServiceManager, myServiceName, api.SERVICE_ALL_ACCESS);
            if (hService != IntPtr.Zero)
            {
                addLog("OpenService Successful", 1);
            }
            else
            {
                Int32 err = Marshal.GetLastWin32Error();
                addLog("OpenService Error Number : " + err.ToString(), 1);
                addLog("Error Message : " + api.GetErrorMessage(err), 1);
                api.CloseServiceHandle(hServiceManager);
                return;
            }

            api.SERVICE_STATUS sServiceStauts = new api.SERVICE_STATUS();
            if(api.ControlService(hService, api.SERVICE_CONTROL.STOP, ref sServiceStauts))
            {
                addLog("ControlService Successful", 1);

                if (api.DeleteService(hService))
                {
                    addLog("DeleteService Successful", 1);
                }
                else
                {
                    Int32 err = Marshal.GetLastWin32Error();
                    addLog("DeleteService Error Number : " + err.ToString(), 1);
                    addLog("Error Message : " + api.GetErrorMessage(err), 1);
                }

            }
            else
            {
                Int32 err = Marshal.GetLastWin32Error();
                addLog("ControlService Error Number : " + err.ToString(), 1);
                addLog("Error Message : " + api.GetErrorMessage(err), 1);
            }

           
           

            api.CloseServiceHandle(hService);
            api.CloseServiceHandle(hServiceManager);
        }


        private void button2_Click(object sender, EventArgs e)
        {
            addLog("[Driver Start]");

            IntPtr hServiceManager = api.OpenSCManager(null, null, api.SC_MANAGER_ALL_ACCESS);
            if (hServiceManager == IntPtr.Zero)
            {
                Int32 err = Marshal.GetLastWin32Error();
                addLog("OpenSCManager Error Number : " + err.ToString(), 1);
                addLog("Error Message : " + api.GetErrorMessage(err), 1);
                return;
            }

            IntPtr hService = api.OpenService(hServiceManager, myServiceName, api.SERVICE_ALL_ACCESS);
            if (hService != IntPtr.Zero)
            {
                addLog("OpenService Successful", 1);
            }
            else
            {
                Int32 err = Marshal.GetLastWin32Error();
                addLog("OpenService Error Number : " + err.ToString(), 1);
                addLog("Error Message : " + api.GetErrorMessage(err), 1);
                api.CloseServiceHandle(hServiceManager);
                return;
            }

            

            if (api.StartService(hService, 0, null))
            {
                addLog("StartService Successful", 1);
            }
            else
            {
                Int32 err = Marshal.GetLastWin32Error();
                addLog("StartService Error Number : " + err.ToString(), 1);
                addLog("Error Message : " + api.GetErrorMessage(err), 1);
            }

            api.CloseServiceHandle(hService);
            api.CloseServiceHandle(hServiceManager);
        }

        private void button3_Click(object sender, EventArgs e)
        {

        }

        private static uint CTL_CODE(uint DeviceType, uint Function, api.IOCTL_METHOD Method, api.IOCTL_ACCESS Access)
        {
            return ((DeviceType << 16) | (((uint)Access) << 14) | (Function << 2) | ((uint)Method));
        }

        private static byte[] ObjectToByteArray<T>(T obj)
        {
            if (null == obj) return (new byte[0]);
            else if (obj.GetType() == typeof(string)) return (Encoding.ASCII.GetBytes((obj as string) + '\0'));
            else
            {
                int size = Marshal.SizeOf(obj);
                byte[] arr = new byte[size];

                IntPtr ptr = Marshal.AllocHGlobal(size);
                Marshal.StructureToPtr(obj, ptr, true);
                Marshal.Copy(ptr, arr, 0, size);
                Marshal.FreeHGlobal(ptr);
                return (arr);
            }
        }

        private static T ByteArrayToObject<T>(byte[] arrBytes)
        {
            int size = Marshal.SizeOf(typeof(T));
            IntPtr ptr = Marshal.AllocHGlobal(size);

            Marshal.Copy(arrBytes, 0, ptr, size);

            T ret = (T)Marshal.PtrToStructure(ptr, typeof(T));
            Marshal.FreeHGlobal(ptr);
            return (ret);
        }

        private void button5_Click(object sender, EventArgs e)
        {
            addLog("[Protect]");

            uint code = CTL_CODE(40000, 0x800, api.IOCTL_METHOD.METHOD_BUFFERED, api.IOCTL_ACCESS.FILE_READ_ACCESS | api.IOCTL_ACCESS.FILE_WRITE_ACCESS);



            IntPtr hFile = api.CreateFile("\\\\.\\" + myName, api.EFileAccess.GenericRead | api.EFileAccess.GenericWrite,
            api.EFileShare.None, IntPtr.Zero, api.ECreationDisposition.OpenExisting,
            api.EFileAttributes.Normal, IntPtr.Zero);

            if(hFile == api.INVALID_HANDLE_VALUE)
            {
                Int32 err = Marshal.GetLastWin32Error();
                addLog("CreateFile Error Number : " + err.ToString(), 1);
                addLog("Error Message : " + api.GetErrorMessage(err), 1);
                return;
            }

            uint intValue;
            byte[] intBytes = BitConverter.GetBytes(uint.Parse(textBox2.Text));
            //Array.Reverse(intBytes);

            byte[] result = intBytes;

            int returnBytes = 0;


            if(api.DeviceIoControl(hFile, code, result, 4, null, 0, out returnBytes, IntPtr.Zero))
            {
                addLog("DeviceIoControl Successful", 1);
            }
            else
            {
                Int32 err = Marshal.GetLastWin32Error();
                addLog("CreateFile Error Number : " + err.ToString(), 1);
                addLog("Error Message : " + api.GetErrorMessage(err), 1);
            }
          

            api.CloseHandle(hFile);

        }

        private void button6_Click(object sender, EventArgs e)
        {
            addLog("[UnProtect]");

            uint code = CTL_CODE(40000, 0x801, api.IOCTL_METHOD.METHOD_BUFFERED, api.IOCTL_ACCESS.FILE_READ_ACCESS | api.IOCTL_ACCESS.FILE_WRITE_ACCESS);



            IntPtr hFile = api.CreateFile("\\\\.\\" + myName, api.EFileAccess.GenericRead | api.EFileAccess.GenericWrite,
            api.EFileShare.None, IntPtr.Zero, api.ECreationDisposition.OpenExisting,
            api.EFileAttributes.Normal, IntPtr.Zero);

            if (hFile == api.INVALID_HANDLE_VALUE)
            {
                Int32 err = Marshal.GetLastWin32Error();
                addLog("CreateFile Error Number : " + err.ToString(), 1);
                addLog("Error Message : " + api.GetErrorMessage(err), 1);
                return;
            }


            int returnBytes = 0;
            if (api.DeviceIoControl(hFile, code, null, 0, null, 0, out returnBytes, IntPtr.Zero))
            {
                addLog("DeviceIoControl Successful", 1);
            }
            else
            {
                Int32 err = Marshal.GetLastWin32Error();
                addLog("CreateFile Error Number : " + err.ToString(), 1);
                addLog("Error Message : " + api.GetErrorMessage(err), 1);
            }


            api.CloseHandle(hFile);
        }

        private void 지우기ToolStripMenuItem_Click(object sender, EventArgs e)
        {
            textBox1.Clear();
        }
    }
}
