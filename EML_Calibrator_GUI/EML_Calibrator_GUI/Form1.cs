using System;
using System.IO;
using System.Net.Sockets;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace EML_Calibrator_GUI
{
    public partial class Form1 : Form
    {
        private TcpClient? _client;
        private StreamReader? _reader;
        private StreamWriter? _writer;

        public Form1()
        {
            InitializeComponent();
            InitialiseGaugeModeCombos();
            SetConnectedState(false);
        }

        private void InitialiseGaugeModeCombos()
        {
            string[] modes = { "Disabled", "Single", "Dual" };

            cmbGauge1Mode.Items.AddRange(modes);
            cmbGauge2Mode.Items.AddRange(modes);
            cmbGauge3Mode.Items.AddRange(modes);
            cmbGauge4Mode.Items.AddRange(modes);

            cmbGauge1Mode.SelectedIndex = 0;
            cmbGauge2Mode.SelectedIndex = 0;
            cmbGauge3Mode.SelectedIndex = 0;
            cmbGauge4Mode.SelectedIndex = 0;
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

        private async void btnSendGaugeSetup_Click(object sender, EventArgs e)
        {
            await SendGaugeSetupAsync();
        }

        private async void txtCommand_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                e.SuppressKeyPress = true;
                await SendCommandAsync(txtCommand.Text.Trim());
            }
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
                }
            }
            catch (Exception ex)
            {
                Disconnect();
                MessageBox.Show(ex.Message, "Connection failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void Disconnect()
        {
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

            cmbGauge1Mode.Enabled = connected;
            cmbGauge2Mode.Enabled = connected;
            cmbGauge3Mode.Enabled = connected;
            cmbGauge4Mode.Enabled = connected;
        }

        private int GetGaugeModeValue(ComboBox comboBox)
        {
            return comboBox.SelectedItem?.ToString() switch
            {
                "Single" => 1,
                "Dual" => 2,
                _ => 0,
            };
        }

        private async Task SendGaugeSetupAsync()
        {
            if (_writer == null || _reader == null)
                return;

            int[] modes =
            {
                GetGaugeModeValue(cmbGauge1Mode),
                GetGaugeModeValue(cmbGauge2Mode),
                GetGaugeModeValue(cmbGauge3Mode),
                GetGaugeModeValue(cmbGauge4Mode)
            };

            Log($"Applying gauge setup: G1={cmbGauge1Mode.Text}, G2={cmbGauge2Mode.Text}, G3={cmbGauge3Mode.Text}, G4={cmbGauge4Mode.Text}");

            for (int gauge = 1; gauge <= 4; gauge++)
            {
                await SendCommandAsync($"SET_GAUGE_MODE,{gauge},{modes[gauge - 1]}");
            }

            await SendCommandAsync("GET_GAUGE_CONFIG");
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
                Log($"RX: {reply ?? "<no reply>"}");
            }
            catch (Exception ex)
            {
                Log($"ERROR: {ex.Message}");
                Disconnect();
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
