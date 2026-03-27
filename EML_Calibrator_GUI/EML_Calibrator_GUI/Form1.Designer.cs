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

        private System.Windows.Forms.Panel pnlGaugeSetup;
        private System.Windows.Forms.TableLayoutPanel tableGaugeSetup;
        private System.Windows.Forms.Button btnSendGaugeSetup;

        private System.Windows.Forms.Label lblHeaderGauge;
        private System.Windows.Forms.Label lblHeaderType;
        private System.Windows.Forms.Label lblHeaderReeds;
        private System.Windows.Forms.Label lblHeaderTargetCf;

        private System.Windows.Forms.Label lblGauge1;
        private System.Windows.Forms.Label lblGauge2;
        private System.Windows.Forms.Label lblGauge3;
        private System.Windows.Forms.Label lblGauge4;

        private System.Windows.Forms.ComboBox cbGaugeType1;
        private System.Windows.Forms.ComboBox cbGaugeType2;
        private System.Windows.Forms.ComboBox cbGaugeType3;
        private System.Windows.Forms.ComboBox cbGaugeType4;

        private System.Windows.Forms.ComboBox cbReeds1;
        private System.Windows.Forms.ComboBox cbReeds2;
        private System.Windows.Forms.ComboBox cbReeds3;
        private System.Windows.Forms.ComboBox cbReeds4;

        private System.Windows.Forms.ComboBox cbTargetCf1;
        private System.Windows.Forms.ComboBox cbTargetCf2;
        private System.Windows.Forms.ComboBox cbTargetCf3;
        private System.Windows.Forms.ComboBox cbTargetCf4;

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
            tableLayoutPanel1 = new TableLayoutPanel();
            flowConnection = new FlowLayoutPanel();
            lblIpAddress = new Label();
            txtIpAddress = new TextBox();
            lblPort = new Label();
            txtPort = new TextBox();
            btnConnect = new Button();
            btnDisconnect = new Button();
            flowCommands = new FlowLayoutPanel();
            btnPing = new Button();
            btnStatus = new Button();
            btnWeights = new Button();
            txtCommand = new TextBox();
            btnSend = new Button();
            pnlGaugeSetup = new Panel();
            btnSendGaugeSetup = new Button();
            tableGaugeSetup = new TableLayoutPanel();
            lblHeaderGauge = new Label();
            lblHeaderType = new Label();
            lblHeaderReeds = new Label();
            lblHeaderTargetCf = new Label();
            lblGauge1 = new Label();
            cbGaugeType1 = new ComboBox();
            cbReeds1 = new ComboBox();
            cbTargetCf1 = new ComboBox();
            lblGauge2 = new Label();
            cbGaugeType2 = new ComboBox();
            cbReeds2 = new ComboBox();
            cbTargetCf2 = new ComboBox();
            lblGauge3 = new Label();
            cbGaugeType3 = new ComboBox();
            cbReeds3 = new ComboBox();
            cbTargetCf3 = new ComboBox();
            lblGauge4 = new Label();
            cbGaugeType4 = new ComboBox();
            cbReeds4 = new ComboBox();
            cbTargetCf4 = new ComboBox();
            txtLog = new TextBox();
            tableLayoutPanel1.SuspendLayout();
            flowConnection.SuspendLayout();
            flowCommands.SuspendLayout();
            pnlGaugeSetup.SuspendLayout();
            tableGaugeSetup.SuspendLayout();
            SuspendLayout();
            // 
            // tableLayoutPanel1
            // 
            tableLayoutPanel1.ColumnCount = 1;
            tableLayoutPanel1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            tableLayoutPanel1.Controls.Add(flowConnection, 0, 0);
            tableLayoutPanel1.Controls.Add(flowCommands, 0, 1);
            tableLayoutPanel1.Controls.Add(pnlGaugeSetup, 0, 2);
            tableLayoutPanel1.Controls.Add(txtLog, 0, 3);
            tableLayoutPanel1.Dock = DockStyle.Fill;
            tableLayoutPanel1.Location = new Point(8, 8);
            tableLayoutPanel1.Name = "tableLayoutPanel1";
            tableLayoutPanel1.RowCount = 4;
            tableLayoutPanel1.RowStyles.Add(new RowStyle());
            tableLayoutPanel1.RowStyles.Add(new RowStyle());
            tableLayoutPanel1.RowStyles.Add(new RowStyle());
            tableLayoutPanel1.RowStyles.Add(new RowStyle(SizeType.Percent, 100F));
            tableLayoutPanel1.Size = new Size(1084, 645);
            tableLayoutPanel1.TabIndex = 0;
            // 
            // flowConnection
            // 
            flowConnection.AutoSize = true;
            flowConnection.Controls.Add(lblIpAddress);
            flowConnection.Controls.Add(txtIpAddress);
            flowConnection.Controls.Add(lblPort);
            flowConnection.Controls.Add(txtPort);
            flowConnection.Controls.Add(btnConnect);
            flowConnection.Controls.Add(btnDisconnect);
            flowConnection.Dock = DockStyle.Fill;
            flowConnection.Location = new Point(3, 3);
            flowConnection.Name = "flowConnection";
            flowConnection.Size = new Size(1078, 33);
            flowConnection.TabIndex = 0;
            // 
            // lblIpAddress
            // 
            lblIpAddress.AutoSize = true;
            lblIpAddress.Location = new Point(3, 8);
            lblIpAddress.Margin = new Padding(3, 8, 3, 0);
            lblIpAddress.Name = "lblIpAddress";
            lblIpAddress.Size = new Size(20, 15);
            lblIpAddress.TabIndex = 0;
            lblIpAddress.Text = "IP:";
            // 
            // txtIpAddress
            // 
            txtIpAddress.Location = new Point(29, 3);
            txtIpAddress.Name = "txtIpAddress";
            txtIpAddress.Size = new Size(140, 23);
            txtIpAddress.TabIndex = 1;
            txtIpAddress.Text = "172.16.168.99";
            // 
            // lblPort
            // 
            lblPort.AutoSize = true;
            lblPort.Location = new Point(175, 8);
            lblPort.Margin = new Padding(3, 8, 3, 0);
            lblPort.Name = "lblPort";
            lblPort.Size = new Size(32, 15);
            lblPort.TabIndex = 2;
            lblPort.Text = "Port:";
            // 
            // txtPort
            // 
            txtPort.Location = new Point(213, 3);
            txtPort.Name = "txtPort";
            txtPort.Size = new Size(70, 23);
            txtPort.TabIndex = 3;
            txtPort.Text = "5000";
            // 
            // btnConnect
            // 
            btnConnect.Location = new Point(289, 3);
            btnConnect.Name = "btnConnect";
            btnConnect.Size = new Size(90, 27);
            btnConnect.TabIndex = 4;
            btnConnect.Text = "Connect";
            btnConnect.UseVisualStyleBackColor = true;
            btnConnect.Click += btnConnect_Click;
            // 
            // btnDisconnect
            // 
            btnDisconnect.Location = new Point(385, 3);
            btnDisconnect.Name = "btnDisconnect";
            btnDisconnect.Size = new Size(90, 27);
            btnDisconnect.TabIndex = 5;
            btnDisconnect.Text = "Disconnect";
            btnDisconnect.UseVisualStyleBackColor = true;
            btnDisconnect.Click += btnDisconnect_Click;
            // 
            // flowCommands
            // 
            flowCommands.AutoSize = true;
            flowCommands.Controls.Add(btnPing);
            flowCommands.Controls.Add(btnStatus);
            flowCommands.Controls.Add(btnWeights);
            flowCommands.Controls.Add(txtCommand);
            flowCommands.Controls.Add(btnSend);
            flowCommands.Dock = DockStyle.Fill;
            flowCommands.Location = new Point(3, 42);
            flowCommands.Name = "flowCommands";
            flowCommands.Size = new Size(1078, 33);
            flowCommands.TabIndex = 1;
            // 
            // btnPing
            // 
            btnPing.Location = new Point(3, 3);
            btnPing.Name = "btnPing";
            btnPing.Size = new Size(90, 27);
            btnPing.TabIndex = 0;
            btnPing.Text = "PING";
            btnPing.UseVisualStyleBackColor = true;
            btnPing.Click += btnPing_Click;
            // 
            // btnStatus
            // 
            btnStatus.Location = new Point(99, 3);
            btnStatus.Name = "btnStatus";
            btnStatus.Size = new Size(100, 27);
            btnStatus.TabIndex = 1;
            btnStatus.Text = "GET_STATUS";
            btnStatus.UseVisualStyleBackColor = true;
            btnStatus.Click += btnStatus_Click;
            // 
            // btnWeights
            // 
            btnWeights.Location = new Point(205, 3);
            btnWeights.Name = "btnWeights";
            btnWeights.Size = new Size(110, 27);
            btnWeights.TabIndex = 2;
            btnWeights.Text = "GET_WEIGHTS";
            btnWeights.UseVisualStyleBackColor = true;
            btnWeights.Click += btnWeights_Click;
            // 
            // txtCommand
            // 
            txtCommand.Location = new Point(321, 3);
            txtCommand.Name = "txtCommand";
            txtCommand.Size = new Size(300, 23);
            txtCommand.TabIndex = 3;
            txtCommand.Text = "PING";
            txtCommand.KeyDown += txtCommand_KeyDown;
            // 
            // btnSend
            // 
            btnSend.Location = new Point(627, 3);
            btnSend.Name = "btnSend";
            btnSend.Size = new Size(90, 27);
            btnSend.TabIndex = 4;
            btnSend.Text = "Send";
            btnSend.UseVisualStyleBackColor = true;
            btnSend.Click += btnSend_Click;
            // 
            // pnlGaugeSetup
            // 
            pnlGaugeSetup.AutoSize = true;
            pnlGaugeSetup.Controls.Add(btnSendGaugeSetup);
            pnlGaugeSetup.Controls.Add(tableGaugeSetup);
            pnlGaugeSetup.Dock = DockStyle.Fill;
            pnlGaugeSetup.Location = new Point(3, 81);
            pnlGaugeSetup.Name = "pnlGaugeSetup";
            pnlGaugeSetup.Size = new Size(1078, 168);
            pnlGaugeSetup.TabIndex = 2;
            // 
            // btnSendGaugeSetup
            // 
            btnSendGaugeSetup.Location = new Point(925, 138);
            btnSendGaugeSetup.Name = "btnSendGaugeSetup";
            btnSendGaugeSetup.Size = new Size(150, 27);
            btnSendGaugeSetup.TabIndex = 1;
            btnSendGaugeSetup.Text = "Send Gauge Setup";
            btnSendGaugeSetup.UseVisualStyleBackColor = true;
            btnSendGaugeSetup.Click += btnSendGaugeSetup_Click;
            // 
            // tableGaugeSetup
            // 
            tableGaugeSetup.ColumnCount = 4;
            tableGaugeSetup.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 100F));
            tableGaugeSetup.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 180F));
            tableGaugeSetup.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 140F));
            tableGaugeSetup.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 140F));
            tableGaugeSetup.Controls.Add(lblHeaderGauge, 0, 0);
            tableGaugeSetup.Controls.Add(lblHeaderType, 1, 0);
            tableGaugeSetup.Controls.Add(lblHeaderReeds, 2, 0);
            tableGaugeSetup.Controls.Add(lblHeaderTargetCf, 3, 0);
            tableGaugeSetup.Controls.Add(lblGauge1, 0, 1);
            tableGaugeSetup.Controls.Add(cbGaugeType1, 1, 1);
            tableGaugeSetup.Controls.Add(cbReeds1, 2, 1);
            tableGaugeSetup.Controls.Add(cbTargetCf1, 3, 1);
            tableGaugeSetup.Controls.Add(lblGauge2, 0, 2);
            tableGaugeSetup.Controls.Add(cbGaugeType2, 1, 2);
            tableGaugeSetup.Controls.Add(cbReeds2, 2, 2);
            tableGaugeSetup.Controls.Add(cbTargetCf2, 3, 2);
            tableGaugeSetup.Controls.Add(lblGauge3, 0, 3);
            tableGaugeSetup.Controls.Add(cbGaugeType3, 1, 3);
            tableGaugeSetup.Controls.Add(cbReeds3, 2, 3);
            tableGaugeSetup.Controls.Add(cbTargetCf3, 3, 3);
            tableGaugeSetup.Controls.Add(lblGauge4, 0, 4);
            tableGaugeSetup.Controls.Add(cbGaugeType4, 1, 4);
            tableGaugeSetup.Controls.Add(cbReeds4, 2, 4);
            tableGaugeSetup.Controls.Add(cbTargetCf4, 3, 4);
            tableGaugeSetup.Location = new Point(0, 0);
            tableGaugeSetup.Name = "tableGaugeSetup";
            tableGaugeSetup.RowCount = 5;
            tableGaugeSetup.RowStyles.Add(new RowStyle(SizeType.Absolute, 28F));
            tableGaugeSetup.RowStyles.Add(new RowStyle(SizeType.Absolute, 28F));
            tableGaugeSetup.RowStyles.Add(new RowStyle(SizeType.Absolute, 28F));
            tableGaugeSetup.RowStyles.Add(new RowStyle(SizeType.Absolute, 28F));
            tableGaugeSetup.RowStyles.Add(new RowStyle(SizeType.Absolute, 28F));
            tableGaugeSetup.Size = new Size(580, 146);
            tableGaugeSetup.TabIndex = 0;
            // 
            // lblHeaderGauge
            // 
            lblHeaderGauge.Anchor = AnchorStyles.Left;
            lblHeaderGauge.AutoSize = true;
            lblHeaderGauge.Font = new Font("Segoe UI", 9F, FontStyle.Bold, GraphicsUnit.Point);
            lblHeaderGauge.Location = new Point(3, 6);
            lblHeaderGauge.Name = "lblHeaderGauge";
            lblHeaderGauge.Size = new Size(43, 15);
            lblHeaderGauge.TabIndex = 0;
            lblHeaderGauge.Text = "Gauge";
            // 
            // lblHeaderType
            // 
            lblHeaderType.Anchor = AnchorStyles.Left;
            lblHeaderType.AutoSize = true;
            lblHeaderType.Font = new Font("Segoe UI", 9F, FontStyle.Bold, GraphicsUnit.Point);
            lblHeaderType.Location = new Point(103, 6);
            lblHeaderType.Name = "lblHeaderType";
            lblHeaderType.Size = new Size(33, 15);
            lblHeaderType.TabIndex = 1;
            lblHeaderType.Text = "Type";
            // 
            // lblHeaderReeds
            // 
            lblHeaderReeds.Anchor = AnchorStyles.Left;
            lblHeaderReeds.AutoSize = true;
            lblHeaderReeds.Font = new Font("Segoe UI", 9F, FontStyle.Bold, GraphicsUnit.Point);
            lblHeaderReeds.Location = new Point(283, 6);
            lblHeaderReeds.Name = "lblHeaderReeds";
            lblHeaderReeds.Size = new Size(41, 15);
            lblHeaderReeds.TabIndex = 2;
            lblHeaderReeds.Text = "Reeds";
            // 
            // lblHeaderTargetCf
            // 
            lblHeaderTargetCf.Anchor = AnchorStyles.Left;
            lblHeaderTargetCf.AutoSize = true;
            lblHeaderTargetCf.Font = new Font("Segoe UI", 9F, FontStyle.Bold, GraphicsUnit.Point);
            lblHeaderTargetCf.Location = new Point(423, 6);
            lblHeaderTargetCf.Name = "lblHeaderTargetCf";
            lblHeaderTargetCf.Size = new Size(59, 15);
            lblHeaderTargetCf.TabIndex = 3;
            lblHeaderTargetCf.Text = "Target CF";
            // 
            // lblGauge1
            // 
            lblGauge1.Anchor = AnchorStyles.Left;
            lblGauge1.AutoSize = true;
            lblGauge1.Location = new Point(3, 34);
            lblGauge1.Name = "lblGauge1";
            lblGauge1.Size = new Size(50, 15);
            lblGauge1.TabIndex = 4;
            lblGauge1.Text = "Gauge 1";
            // 
            // cbGaugeType1
            // 
            cbGaugeType1.Dock = DockStyle.Fill;
            cbGaugeType1.DropDownStyle = ComboBoxStyle.DropDownList;
            cbGaugeType1.FormattingEnabled = true;
            cbGaugeType1.Items.AddRange(new object[] { "Disabled", "SBS500", "SBS314", "ARG314", "Kalyx", "UPG1000" });
            cbGaugeType1.Location = new Point(103, 31);
            cbGaugeType1.Name = "cbGaugeType1";
            cbGaugeType1.Size = new Size(174, 23);
            cbGaugeType1.TabIndex = 8;
            // 
            // cbReeds1
            // 
            cbReeds1.Dock = DockStyle.Fill;
            cbReeds1.DropDownStyle = ComboBoxStyle.DropDownList;
            cbReeds1.FormattingEnabled = true;
            cbReeds1.Items.AddRange(new object[] { "None", "Single", "Dual" });
            cbReeds1.Location = new Point(283, 31);
            cbReeds1.Name = "cbReeds1";
            cbReeds1.Size = new Size(134, 23);
            cbReeds1.TabIndex = 12;
            // 
            // cbTargetCf1
            // 
            cbTargetCf1.Dock = DockStyle.Fill;
            cbTargetCf1.DropDownStyle = ComboBoxStyle.DropDownList;
            cbTargetCf1.FormattingEnabled = true;
            cbTargetCf1.Items.AddRange(new object[] { "0.1", "0.2", "0.5" });
            cbTargetCf1.Location = new Point(423, 31);
            cbTargetCf1.Name = "cbTargetCf1";
            cbTargetCf1.Size = new Size(154, 23);
            cbTargetCf1.TabIndex = 16;
            // 
            // lblGauge2
            // 
            lblGauge2.Anchor = AnchorStyles.Left;
            lblGauge2.AutoSize = true;
            lblGauge2.Location = new Point(3, 62);
            lblGauge2.Name = "lblGauge2";
            lblGauge2.Size = new Size(50, 15);
            lblGauge2.TabIndex = 5;
            lblGauge2.Text = "Gauge 2";
            // 
            // cbGaugeType2
            // 
            cbGaugeType2.Dock = DockStyle.Fill;
            cbGaugeType2.DropDownStyle = ComboBoxStyle.DropDownList;
            cbGaugeType2.FormattingEnabled = true;
            cbGaugeType2.Items.AddRange(new object[] { "Disabled", "SBS500", "SBS314", "ARG314", "Kalyx", "UPG1000" });
            cbGaugeType2.Location = new Point(103, 59);
            cbGaugeType2.Name = "cbGaugeType2";
            cbGaugeType2.Size = new Size(174, 23);
            cbGaugeType2.TabIndex = 9;
            // 
            // cbReeds2
            // 
            cbReeds2.Dock = DockStyle.Fill;
            cbReeds2.DropDownStyle = ComboBoxStyle.DropDownList;
            cbReeds2.FormattingEnabled = true;
            cbReeds2.Items.AddRange(new object[] { "None", "Single", "Dual" });
            cbReeds2.Location = new Point(283, 59);
            cbReeds2.Name = "cbReeds2";
            cbReeds2.Size = new Size(134, 23);
            cbReeds2.TabIndex = 13;
            // 
            // cbTargetCf2
            // 
            cbTargetCf2.Dock = DockStyle.Fill;
            cbTargetCf2.DropDownStyle = ComboBoxStyle.DropDownList;
            cbTargetCf2.FormattingEnabled = true;
            cbTargetCf2.Items.AddRange(new object[] { "0.1", "0.2", "0.5" });
            cbTargetCf2.Location = new Point(423, 59);
            cbTargetCf2.Name = "cbTargetCf2";
            cbTargetCf2.Size = new Size(154, 23);
            cbTargetCf2.TabIndex = 17;
            // 
            // lblGauge3
            // 
            lblGauge3.Anchor = AnchorStyles.Left;
            lblGauge3.AutoSize = true;
            lblGauge3.Location = new Point(3, 90);
            lblGauge3.Name = "lblGauge3";
            lblGauge3.Size = new Size(50, 15);
            lblGauge3.TabIndex = 6;
            lblGauge3.Text = "Gauge 3";
            // 
            // cbGaugeType3
            // 
            cbGaugeType3.Dock = DockStyle.Fill;
            cbGaugeType3.DropDownStyle = ComboBoxStyle.DropDownList;
            cbGaugeType3.FormattingEnabled = true;
            cbGaugeType3.Items.AddRange(new object[] { "Disabled", "SBS500", "SBS314", "ARG314", "Kalyx", "UPG1000" });
            cbGaugeType3.Location = new Point(103, 87);
            cbGaugeType3.Name = "cbGaugeType3";
            cbGaugeType3.Size = new Size(174, 23);
            cbGaugeType3.TabIndex = 10;
            // 
            // cbReeds3
            // 
            cbReeds3.Dock = DockStyle.Fill;
            cbReeds3.DropDownStyle = ComboBoxStyle.DropDownList;
            cbReeds3.FormattingEnabled = true;
            cbReeds3.Items.AddRange(new object[] { "None", "Single", "Dual" });
            cbReeds3.Location = new Point(283, 87);
            cbReeds3.Name = "cbReeds3";
            cbReeds3.Size = new Size(134, 23);
            cbReeds3.TabIndex = 14;
            // 
            // cbTargetCf3
            // 
            cbTargetCf3.Dock = DockStyle.Fill;
            cbTargetCf3.DropDownStyle = ComboBoxStyle.DropDownList;
            cbTargetCf3.FormattingEnabled = true;
            cbTargetCf3.Items.AddRange(new object[] { "0.1", "0.2", "0.5" });
            cbTargetCf3.Location = new Point(423, 87);
            cbTargetCf3.Name = "cbTargetCf3";
            cbTargetCf3.Size = new Size(154, 23);
            cbTargetCf3.TabIndex = 18;
            // 
            // lblGauge4
            // 
            lblGauge4.Anchor = AnchorStyles.Left;
            lblGauge4.AutoSize = true;
            lblGauge4.Location = new Point(3, 121);
            lblGauge4.Name = "lblGauge4";
            lblGauge4.Size = new Size(50, 15);
            lblGauge4.TabIndex = 7;
            lblGauge4.Text = "Gauge 4";
            // 
            // cbGaugeType4
            // 
            cbGaugeType4.Dock = DockStyle.Fill;
            cbGaugeType4.DropDownStyle = ComboBoxStyle.DropDownList;
            cbGaugeType4.FormattingEnabled = true;
            cbGaugeType4.Items.AddRange(new object[] { "Disabled", "SBS500", "SBS314", "ARG314", "Kalyx", "UPG1000" });
            cbGaugeType4.Location = new Point(103, 115);
            cbGaugeType4.Name = "cbGaugeType4";
            cbGaugeType4.Size = new Size(174, 23);
            cbGaugeType4.TabIndex = 11;
            // 
            // cbReeds4
            // 
            cbReeds4.Dock = DockStyle.Fill;
            cbReeds4.DropDownStyle = ComboBoxStyle.DropDownList;
            cbReeds4.FormattingEnabled = true;
            cbReeds4.Items.AddRange(new object[] { "None", "Single", "Dual" });
            cbReeds4.Location = new Point(283, 115);
            cbReeds4.Name = "cbReeds4";
            cbReeds4.Size = new Size(134, 23);
            cbReeds4.TabIndex = 15;
            // 
            // cbTargetCf4
            // 
            cbTargetCf4.Dock = DockStyle.Fill;
            cbTargetCf4.DropDownStyle = ComboBoxStyle.DropDownList;
            cbTargetCf4.FormattingEnabled = true;
            cbTargetCf4.Items.AddRange(new object[] { "0.1", "0.2", "0.5" });
            cbTargetCf4.Location = new Point(423, 115);
            cbTargetCf4.Name = "cbTargetCf4";
            cbTargetCf4.Size = new Size(154, 23);
            cbTargetCf4.TabIndex = 19;
            // 
            // txtLog
            // 
            txtLog.Dock = DockStyle.Fill;
            txtLog.Font = new Font("Consolas", 10F, FontStyle.Regular, GraphicsUnit.Point);
            txtLog.Location = new Point(3, 255);
            txtLog.Multiline = true;
            txtLog.Name = "txtLog";
            txtLog.ReadOnly = true;
            txtLog.ScrollBars = ScrollBars.Vertical;
            txtLog.Size = new Size(1078, 387);
            txtLog.TabIndex = 3;
            txtLog.WordWrap = false;
            // 
            // Form1
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(1100, 661);
            Controls.Add(tableLayoutPanel1);
            Name = "Form1";
            Padding = new Padding(8);
            StartPosition = FormStartPosition.CenterScreen;
            Text = "EML Calibrator GUI";
            tableLayoutPanel1.ResumeLayout(false);
            tableLayoutPanel1.PerformLayout();
            flowConnection.ResumeLayout(false);
            flowConnection.PerformLayout();
            flowCommands.ResumeLayout(false);
            flowCommands.PerformLayout();
            pnlGaugeSetup.ResumeLayout(false);
            tableGaugeSetup.ResumeLayout(false);
            tableGaugeSetup.PerformLayout();
            ResumeLayout(false);
        }
    }
}
