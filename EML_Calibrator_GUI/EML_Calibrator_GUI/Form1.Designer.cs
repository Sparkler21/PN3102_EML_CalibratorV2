namespace EML_Calibrator_GUI
{
    partial class Form1
    {
        private System.ComponentModel.IContainer components = null;

        private System.Windows.Forms.Label lblIpAddress;
        private System.Windows.Forms.Label lblPort;
        private System.Windows.Forms.TextBox txtIpAddress;
        private System.Windows.Forms.TextBox txtPort;
        private System.Windows.Forms.Button btnConnect;
        private System.Windows.Forms.Button btnDisconnect;
        private System.Windows.Forms.Button btnPing;
        private System.Windows.Forms.Button btnStatus;
        private System.Windows.Forms.Button btnWeights;
        private System.Windows.Forms.TextBox txtCommand;
        private System.Windows.Forms.Button btnSend;
        private System.Windows.Forms.TextBox txtLog;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.FlowLayoutPanel flowConnection;
        private System.Windows.Forms.FlowLayoutPanel flowCommands;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }

            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.flowConnection = new System.Windows.Forms.FlowLayoutPanel();
            this.lblIpAddress = new System.Windows.Forms.Label();
            this.txtIpAddress = new System.Windows.Forms.TextBox();
            this.lblPort = new System.Windows.Forms.Label();
            this.txtPort = new System.Windows.Forms.TextBox();
            this.btnConnect = new System.Windows.Forms.Button();
            this.btnDisconnect = new System.Windows.Forms.Button();
            this.flowCommands = new System.Windows.Forms.FlowLayoutPanel();
            this.btnPing = new System.Windows.Forms.Button();
            this.btnStatus = new System.Windows.Forms.Button();
            this.btnWeights = new System.Windows.Forms.Button();
            this.txtCommand = new System.Windows.Forms.TextBox();
            this.btnSend = new System.Windows.Forms.Button();
            this.txtLog = new System.Windows.Forms.TextBox();
            this.tableLayoutPanel1.SuspendLayout();
            this.flowConnection.SuspendLayout();
            this.flowCommands.SuspendLayout();
            this.SuspendLayout();

            // tableLayoutPanel1
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Controls.Add(this.flowConnection, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.flowCommands, 0, 1);
            this.tableLayoutPanel1.Controls.Add(this.txtLog, 0, 2);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(8, 8);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 3;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(884, 545);
            this.tableLayoutPanel1.TabIndex = 0;

            // flowConnection
            this.flowConnection.AutoSize = true;
            this.flowConnection.Controls.Add(this.lblIpAddress);
            this.flowConnection.Controls.Add(this.txtIpAddress);
            this.flowConnection.Controls.Add(this.lblPort);
            this.flowConnection.Controls.Add(this.txtPort);
            this.flowConnection.Controls.Add(this.btnConnect);
            this.flowConnection.Controls.Add(this.btnDisconnect);
            this.flowConnection.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowConnection.Location = new System.Drawing.Point(3, 3);
            this.flowConnection.Name = "flowConnection";
            this.flowConnection.Size = new System.Drawing.Size(878, 35);
            this.flowConnection.TabIndex = 0;

            // lblIpAddress
            this.lblIpAddress.AutoSize = true;
            this.lblIpAddress.Location = new System.Drawing.Point(3, 8);
            this.lblIpAddress.Margin = new System.Windows.Forms.Padding(3, 8, 3, 0);
            this.lblIpAddress.Name = "lblIpAddress";
            this.lblIpAddress.Size = new System.Drawing.Size(20, 15);
            this.lblIpAddress.TabIndex = 0;
            this.lblIpAddress.Text = "IP:";

            // txtIpAddress
            this.txtIpAddress.Location = new System.Drawing.Point(29, 3);
            this.txtIpAddress.Name = "txtIpAddress";
            this.txtIpAddress.Size = new System.Drawing.Size(140, 23);
            this.txtIpAddress.TabIndex = 1;
            this.txtIpAddress.Text = "172.16.168.99";

            // lblPort
            this.lblPort.AutoSize = true;
            this.lblPort.Location = new System.Drawing.Point(175, 8);
            this.lblPort.Margin = new System.Windows.Forms.Padding(3, 8, 3, 0);
            this.lblPort.Name = "lblPort";
            this.lblPort.Size = new System.Drawing.Size(32, 15);
            this.lblPort.TabIndex = 2;
            this.lblPort.Text = "Port:";

            // txtPort
            this.txtPort.Location = new System.Drawing.Point(213, 3);
            this.txtPort.Name = "txtPort";
            this.txtPort.Size = new System.Drawing.Size(70, 23);
            this.txtPort.TabIndex = 3;
            this.txtPort.Text = "5000";

            // btnConnect
            this.btnConnect.Location = new System.Drawing.Point(289, 3);
            this.btnConnect.Name = "btnConnect";
            this.btnConnect.Size = new System.Drawing.Size(90, 27);
            this.btnConnect.TabIndex = 4;
            this.btnConnect.Text = "Connect";
            this.btnConnect.UseVisualStyleBackColor = true;
            this.btnConnect.Click += new System.EventHandler(this.btnConnect_Click);

            // btnDisconnect
            this.btnDisconnect.Location = new System.Drawing.Point(385, 3);
            this.btnDisconnect.Name = "btnDisconnect";
            this.btnDisconnect.Size = new System.Drawing.Size(90, 27);
            this.btnDisconnect.TabIndex = 5;
            this.btnDisconnect.Text = "Disconnect";
            this.btnDisconnect.UseVisualStyleBackColor = true;
            this.btnDisconnect.Click += new System.EventHandler(this.btnDisconnect_Click);

            // flowCommands
            this.flowCommands.AutoSize = true;
            this.flowCommands.Controls.Add(this.btnPing);
            this.flowCommands.Controls.Add(this.btnStatus);
            this.flowCommands.Controls.Add(this.btnWeights);
            this.flowCommands.Controls.Add(this.txtCommand);
            this.flowCommands.Controls.Add(this.btnSend);
            this.flowCommands.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowCommands.Location = new System.Drawing.Point(3, 44);
            this.flowCommands.Name = "flowCommands";
            this.flowCommands.Size = new System.Drawing.Size(878, 35);
            this.flowCommands.TabIndex = 1;

            // btnPing
            this.btnPing.Location = new System.Drawing.Point(3, 3);
            this.btnPing.Name = "btnPing";
            this.btnPing.Size = new System.Drawing.Size(90, 27);
            this.btnPing.TabIndex = 0;
            this.btnPing.Text = "PING";
            this.btnPing.UseVisualStyleBackColor = true;
            this.btnPing.Click += new System.EventHandler(this.btnPing_Click);

            // btnStatus
            this.btnStatus.Location = new System.Drawing.Point(99, 3);
            this.btnStatus.Name = "btnStatus";
            this.btnStatus.Size = new System.Drawing.Size(100, 27);
            this.btnStatus.TabIndex = 1;
            this.btnStatus.Text = "GET_STATUS";
            this.btnStatus.UseVisualStyleBackColor = true;
            this.btnStatus.Click += new System.EventHandler(this.btnStatus_Click);

            // btnWeights
            this.btnWeights.Location = new System.Drawing.Point(205, 3);
            this.btnWeights.Name = "btnWeights";
            this.btnWeights.Size = new System.Drawing.Size(110, 27);
            this.btnWeights.TabIndex = 2;
            this.btnWeights.Text = "GET_WEIGHTS";
            this.btnWeights.UseVisualStyleBackColor = true;
            this.btnWeights.Click += new System.EventHandler(this.btnWeights_Click);

            // txtCommand
            this.txtCommand.Location = new System.Drawing.Point(321, 3);
            this.txtCommand.Name = "txtCommand";
            this.txtCommand.Size = new System.Drawing.Size(300, 23);
            this.txtCommand.TabIndex = 3;
            this.txtCommand.Text = "PING";
            this.txtCommand.KeyDown += new System.Windows.Forms.KeyEventHandler(this.txtCommand_KeyDown);

            // btnSend
            this.btnSend.Location = new System.Drawing.Point(627, 3);
            this.btnSend.Name = "btnSend";
            this.btnSend.Size = new System.Drawing.Size(90, 27);
            this.btnSend.TabIndex = 4;
            this.btnSend.Text = "Send";
            this.btnSend.UseVisualStyleBackColor = true;
            this.btnSend.Click += new System.EventHandler(this.btnSend_Click);

            // txtLog
            this.txtLog.Dock = System.Windows.Forms.DockStyle.Fill;
            this.txtLog.Font = new System.Drawing.Font("Consolas", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point);
            this.txtLog.Location = new System.Drawing.Point(3, 85);
            this.txtLog.Multiline = true;
            this.txtLog.Name = "txtLog";
            this.txtLog.ReadOnly = true;
            this.txtLog.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.txtLog.Size = new System.Drawing.Size(878, 457);
            this.txtLog.TabIndex = 2;
            this.txtLog.WordWrap = false;

            // Form1
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(900, 561);
            this.Controls.Add(this.tableLayoutPanel1);
            this.Name = "Form1";
            this.Padding = new System.Windows.Forms.Padding(8);
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "EML Calibrator GUI";
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.flowConnection.ResumeLayout(false);
            this.flowConnection.PerformLayout();
            this.flowCommands.ResumeLayout(false);
            this.flowCommands.PerformLayout();
            this.ResumeLayout(false);
        }
    }
}