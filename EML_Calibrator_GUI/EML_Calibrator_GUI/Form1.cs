using System;
using System.Globalization;
using System.IO;
using System.Net.Sockets;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace EML_Calibrator_GUI
{
    public partial class Form1 : Form
    {
        private TcpClient? _client;
        private StreamReader? _reader;
        private StreamWriter? _writer;
        private bool _polling;

        public Form1()
        {
            InitializeComponent();
            SetConnectedState(false);
            SetDefaultGaugeSelections();
            ResetVisuals();
        }

        private void SetDefaultGaugeSelections()
        {
            cbGaugeType1.SelectedIndex = 0;
            cbGaugeType2.SelectedIndex = 0;
            cbGaugeType3.SelectedIndex = 0;
            cbGaugeType4.SelectedIndex = 0;

            cbReeds1.SelectedIndex = 0;
            cbReeds2.SelectedIndex = 0;
            cbReeds3.SelectedIndex = 0;
            cbReeds4.SelectedIndex = 0;

            cbTargetCf1.SelectedIndex = 0;
            cbTargetCf2.SelectedIndex = 0;
            cbTargetCf3.SelectedIndex = 0;
            cbTargetCf4.SelectedIndex = 0;
        }

        private void ResetVisuals()
        {
            UpdateTankDisplay(1, 0.0);
            UpdateTankDisplay(2, 0.0);
            UpdateTankDisplay(3, 0.0);
            UpdateTankDisplay(4, 0.0);

            lblCount1.Text = "0";
            lblCount2.Text = "0";
            lblCount3.Text = "0";
            lblCount4.Text = "0";
        }

        private async void btnConnect_Click(object sender, EventArgs e)
        {
            await ConnectAsync();
        }

        private void btnDisconnect_Click(object sender, EventArgs e)
        {
            Disconnect();
        }

        private async void btnPing_Click(object sender, EventArgs e)
        {
            await SendCommandAsync("PING");
        }

        private async void btnStatus_Click(object sender, EventArgs e)
        {
            await SendCommandAsync("GET_STATUS");
        }

        private async void btnWeights_Click(object sender, EventArgs e)
        {
            await SendCommandAsync("GET_WEIGHTS");
        }

        private async void btnSend_Click(object sender, EventArgs e)
        {
            await SendCommandAsync(txtCommand.Text.Trim());
        }

        private async void txtCommand_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                e.SuppressKeyPress = true;
                await SendCommandAsync(txtCommand.Text.Trim());
            }
        }

        private async void btnSendGaugeSetup_Click(object sender, EventArgs e)
        {
            await SendGaugeSetupAsync(1, cbGaugeType1.Text, cbReeds1.Text, cbTargetCf1.Text);
            await SendGaugeSetupAsync(2, cbGaugeType2.Text, cbReeds2.Text, cbTargetCf2.Text);
            await SendGaugeSetupAsync(3, cbGaugeType3.Text, cbReeds3.Text, cbTargetCf3.Text);
            await SendGaugeSetupAsync(4, cbGaugeType4.Text, cbReeds4.Text, cbTargetCf4.Text);
            await SendCommandAsync("GET_GAUGE_CONFIG");
        }

        private async Task SendGaugeSetupAsync(int gaugeNumber, string gaugeType, string reedsFitted, string targetCf)
        {
            string command = $"SET_GAUGE,{gaugeNumber},{gaugeType},{reedsFitted},{targetCf}";
            await SendCommandAsync(command);
        }

        private async Task ConnectAsync()
        {
            try
            {
                Disconnect();

                if (!int.TryParse(txtPort.Text.Trim(), out int port))
                {
                    MessageBox.Show("Invalid port.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }

                _client = new TcpClient();
                await _client.ConnectAsync(txtIpAddress.Text.Trim(), port);

                NetworkStream stream = _client.GetStream();
                _reader = new StreamReader(stream);
                _writer = new StreamWriter(stream)
                {
                    AutoFlush = true,
                    NewLine = "\n"
                };

                SetConnectedState(true);
                Log($"Connected to {txtIpAddress.Text.Trim()}:{port}");

                string? banner = await _reader.ReadLineAsync();
                if (!string.IsNullOrWhiteSpace(banner))
                {
                    Log($"RX: {banner}");
                    ProcessReply(banner);
                }

                pollTimer.Start();
            }
            catch (Exception ex)
            {
                Disconnect();
                MessageBox.Show(ex.Message, "Connection failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void Disconnect()
        {
            pollTimer.Stop();

            _reader?.Dispose();
            _writer?.Dispose();
            _client?.Close();

            _reader = null;
            _writer = null;
            _client = null;

            SetConnectedState(false);
            Log("Disconnected");
        }

        private void SetConnectedState(bool connected)
        {
            btnConnect.Enabled = !connected;
            btnDisconnect.Enabled = connected;
            btnPing.Enabled = connected;
            btnStatus.Enabled = connected;
            btnWeights.Enabled = connected;
            btnSend.Enabled = connected;
            btnSendGaugeSetup.Enabled = connected;
        }

        private async Task SendCommandAsync(string command)
        {
            if (string.IsNullOrWhiteSpace(command) || _writer == null || _reader == null)
                return;

            try
            {
                Log($"TX: {command}");
                await _writer.WriteLineAsync(command);

                string? reply = await _reader.ReadLineAsync();
                string shown = reply ?? "<no reply>";
                Log($"RX: {shown}");

                if (!string.IsNullOrWhiteSpace(reply))
                {
                    ProcessReply(reply);
                }
            }
            catch (Exception ex)
            {
                Log($"ERROR: {ex.Message}");
                Disconnect();
            }
        }

        private void ProcessReply(string reply)
        {
            if (reply.StartsWith("WEIGHTS,", StringComparison.OrdinalIgnoreCase))
            {
                ParseWeights(reply);
                return;
            }

            if (reply.StartsWith("GAUGE_STATUS,", StringComparison.OrdinalIgnoreCase))
            {
                ParseGaugeStatus(reply);
            }
        }

        private void ParseWeights(string reply)
        {
            string[] parts = reply.Split(',');
            if (parts.Length < 5)
                return;

            UpdateTankDisplay(1, ParseDouble(parts[1]));
            UpdateTankDisplay(2, ParseDouble(parts[2]));
            UpdateTankDisplay(3, ParseDouble(parts[3]));
            UpdateTankDisplay(4, ParseDouble(parts[4]));
        }

        private void ParseGaugeStatus(string reply)
        {
            Match m = Regex.Match(reply,
                @"^GAUGE_STATUS,(\d+),.*?,.*?,.*?,.*?,.*?,.*?,(\d+),",
                RegexOptions.IgnoreCase);

            if (!m.Success)
                return;

            int gauge = int.Parse(m.Groups[1].Value, CultureInfo.InvariantCulture);
            int count = int.Parse(m.Groups[2].Value, CultureInfo.InvariantCulture);
            SetGaugeCount(gauge, count);
        }

        private static double ParseDouble(string s)
        {
            _ = double.TryParse(s, NumberStyles.Float, CultureInfo.InvariantCulture, out double value);
            return value;
        }

        private void UpdateTankDisplay(int gaugeNumber, double weightKg)
        {
            weightKg = Math.Max(0.0, Math.Min(10.0, weightKg));
            int fillHeight = (int)Math.Round((weightKg / 10.0) * 180.0);
            fillHeight = Math.Max(2, fillHeight);

            Panel fillPanel;
            Label weightLabel;

            switch (gaugeNumber)
            {
                case 1:
                    fillPanel = pnlTankFill1;
                    weightLabel = lblWeight1;
                    break;
                case 2:
                    fillPanel = pnlTankFill2;
                    weightLabel = lblWeight2;
                    break;
                case 3:
                    fillPanel = pnlTankFill3;
                    weightLabel = lblWeight3;
                    break;
                case 4:
                    fillPanel = pnlTankFill4;
                    weightLabel = lblWeight4;
                    break;
                default:
                    return;
            }

            fillPanel.Height = fillHeight;
            fillPanel.Top = 188 - fillHeight;
            weightLabel.Text = $"{weightKg:0.000} kg";
        }

        private void SetGaugeCount(int gaugeNumber, int count)
        {
            switch (gaugeNumber)
            {
                case 1: lblCount1.Text = count.ToString(CultureInfo.InvariantCulture); break;
                case 2: lblCount2.Text = count.ToString(CultureInfo.InvariantCulture); break;
                case 3: lblCount3.Text = count.ToString(CultureInfo.InvariantCulture); break;
                case 4: lblCount4.Text = count.ToString(CultureInfo.InvariantCulture); break;
            }
        }

        private async void pollTimer_Tick(object? sender, EventArgs e)
        {
            if (_polling || _writer == null || _reader == null)
                return;

            _polling = true;
            try
            {
                await SendCommandAsync("GET_WEIGHTS");
                await SendCommandAsync("GET_GAUGE_STATUS,1");
                await SendCommandAsync("GET_GAUGE_STATUS,2");
                await SendCommandAsync("GET_GAUGE_STATUS,3");
                await SendCommandAsync("GET_GAUGE_STATUS,4");
            }
            finally
            {
                _polling = false;
            }
        }

        private void Log(string message)
        {
            txtLog.AppendText($"[{DateTime.Now:HH:mm:ss}] {message}{Environment.NewLine}");
        }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            Disconnect();
            base.OnFormClosing(e);
        }
    }
}
