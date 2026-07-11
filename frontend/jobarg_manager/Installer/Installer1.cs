using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Configuration.Install;
using System.Linq;
using System.IO;
using Microsoft.Win32;
using System.Diagnostics;
using System.Management;


namespace Installer
{
    // インストーラクラス
    [RunInstaller(true)]
    public partial class Installer1 : System.Configuration.Install.Installer
    {
        public Installer1()
        {
            InitializeComponent();
        }
      
        // インストール時に実行される処理
        public override void Install(IDictionary stateSaver)
        {
            string w_InstDir = this.Context.Parameters["instdir"];
            w_InstDir = w_InstDir.Substring(0, w_InstDir.Length - 2);

            string confFilePath = Path.Combine(w_InstDir, "conf", "jobarg_manager.conf");

            if (File.Exists(confFilePath))
            {
                string tempFolder = Path.GetTempPath();
                string tempFilePath = Path.Combine(tempFolder, "jobarg_manager.conf");

                try
                {
                    File.Copy(confFilePath, tempFilePath, true); 
                    System.Diagnostics.EventLog.WriteEntry("Job Arranger Manager", "Config file successfully backed up to : " + tempFilePath, EventLogEntryType.Information);
                }
                catch (Exception ex)
                {
                    System.Diagnostics.EventLog.WriteEntry("Job Arranger Manager", "Backup process failed. Error copying file: " + ex.Message, EventLogEntryType.Error);
                }
            }

            base.Install(stateSaver);

        }

        // インストール確定時に実行される処理
        public override void Commit(System.Collections.IDictionary savedState)
        {
            base.Commit(savedState);

            // インストール先のパス取得(末尾に\\が付いているので削除)
            string w_InstDir = this.Context.Parameters["instdir"];
            w_InstDir = w_InstDir.Substring(0, w_InstDir.Length - 2);

            string originalConfFilePath = Path.Combine(w_InstDir, "conf", "jobarg_manager.conf");
            string tempFolder = Path.GetTempPath();
            string tempFilePath = Path.Combine(tempFolder, "jobarg_manager.conf");

            try
            {
                if (File.Exists(tempFilePath))
                {
                    File.Copy(tempFilePath, originalConfFilePath, true);
                    System.Diagnostics.EventLog.WriteEntry("Job Arranger Manager", "Config file successfully restored to: " + originalConfFilePath, EventLogEntryType.Information);

                    File.Delete(tempFilePath);
                    System.Diagnostics.EventLog.WriteEntry("Job Arranger Manager", "Config file successfully deleted from: " + tempFilePath, EventLogEntryType.Information);
                }
                else
                {
                    System.Diagnostics.EventLog.WriteEntry("Job Arranger Manager", "Config backup file not found in the temp folder. Skipping the restore process.", EventLogEntryType.Information);
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.EventLog.WriteEntry("Job Arranger Manager", "Error restoring config backup file: " + ex.Message, EventLogEntryType.Error);
            }

            // コマンドプロンプト起動パラメータ設定
            System.Diagnostics.Process w_Process = new System.Diagnostics.Process();
            w_Process.StartInfo.FileName = "cmd.exe";
            w_Process.StartInfo.CreateNoWindow = true;
            w_Process.StartInfo.UseShellExecute = false;

            // usersにフルコントロール権限を当たるコマンド設定
            w_Process.StartInfo.Arguments = "/c cacls \"" + w_InstDir + "\" /t /e /g users:f";

            // コマンドプロンプト起動
            w_Process.Start();
            w_Process.WaitForExit();
            w_Process.Close();
        }

        // インストール失敗時のロールバック時に実行される処理
        public override void Rollback(IDictionary savedState)
        {
            base.Rollback(savedState);
        }

        // アンインストール時に実行される処理
        public override void Uninstall(IDictionary savedState)
        {
            string w_InstDir = this.Context.Parameters["instdir"];
            w_InstDir = w_InstDir.Substring(0, w_InstDir.Length - 2);

            string logsDirectory = Path.Combine(w_InstDir, "logs");
            string confFilePath = Path.Combine(w_InstDir, "conf", "jobarg_manager.conf");

            try
            {
                if (Directory.Exists(logsDirectory))
                {
                    foreach (string file in Directory.GetFiles(logsDirectory))
                    {
                        File.Delete(file);
                    }
                    System.Diagnostics.EventLog.WriteEntry("Job Arranger Manager", "Logs files successfully deleted.", EventLogEntryType.Information);
                }

                if (File.Exists(confFilePath))
                {
                    File.Delete(confFilePath);
                    System.Diagnostics.EventLog.WriteEntry("Job Arranger Manager", "Run-config file successfully deleted.", EventLogEntryType.Information);
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.EventLog.WriteEntry("Job Arranger Manager", "Error deleing additional folders and files: " + ex.Message, EventLogEntryType.Error);
            }

            base.Uninstall(savedState);
        }
    }
}
