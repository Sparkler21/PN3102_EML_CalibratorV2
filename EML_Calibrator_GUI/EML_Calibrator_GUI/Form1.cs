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
            SetConnectedState(false);
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