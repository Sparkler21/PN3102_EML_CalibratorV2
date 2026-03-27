using System;
using System.Globalization;

namespace EML_Calibrator_GUI
{
    public enum GaugeType
    {
        Disabled = 0,
        SingleA = 1,
        Dual = 2
    }

    public enum ReedsFitted
    {
        None = 0,
        AOnly = 1,
        AB = 2
    }

    public enum MachineState
    {
        Idle = 0,
        Fill = 1,
        BalanceTest = 2,
        Test = 3,
        Shutdown = 4,
        Fault = 5
    }

    public static class CommandProtocol
    {
        public const string SetGauge = "SET_GAUGE";
        public const string GetGaugeStatus = "GET_GAUGE_STATUS";
        public const string GetAllGaugeStatus = "GET_ALL_GAUGE_STATUS";
        public const string GetReedStatus = "GET_REED_STATUS";
        public const string ResetGaugeStats = "RESET_GAUGE_STATS";
        public const string ResetAllGaugeStats = "RESET_ALL_GAUGE_STATS";
        public const string ResetReedStats = "RESET_REED_STATS";
        public const string ResetAllReedStats = "RESET_ALL_REED_STATS";
        public const string SetMachineState = "SET_MACHINE_STATE";
        public const string Ping = "PING";

        public static string BuildSetGaugeCommand(
            int gaugeNumber,
            GaugeType gaugeType,
            ReedsFitted reedsFitted,
            float targetCf)
        {
            ValidateGaugeNumber(gaugeNumber);

            return string.Format(
                CultureInfo.InvariantCulture,
                "{0},{1},{2},{3},{4:0.###}",
                SetGauge,
                gaugeNumber,
                (int)gaugeType,
                (int)reedsFitted,
                targetCf);
        }

        public static string BuildGetGaugeStatusCommand(int gaugeNumber)
        {
            ValidateGaugeNumber(gaugeNumber);
            return $"{GetGaugeStatus},{gaugeNumber}";
        }

        public static string BuildGetAllGaugeStatusCommand()
        {
            return GetAllGaugeStatus;
        }

        public static string BuildGetReedStatusCommand(int reedNumber)
        {
            ValidateReedNumber(reedNumber);
            return $"{GetReedStatus},{reedNumber}";
        }

        public static string BuildResetGaugeStatsCommand(int gaugeNumber)
        {
            ValidateGaugeNumber(gaugeNumber);
            return $"{ResetGaugeStats},{gaugeNumber}";
        }

        public static string BuildResetAllGaugeStatsCommand()
        {
            return ResetAllGaugeStats;
        }

        public static string BuildResetReedStatsCommand(int reedNumber)
        {
            ValidateReedNumber(reedNumber);
            return $"{ResetReedStats},{reedNumber}";
        }

        public static string BuildResetAllReedStatsCommand()
        {
            return ResetAllReedStats;
        }

        public static string BuildSetMachineStateCommand(int gaugeNumber, MachineState state)
        {
            ValidateGaugeNumber(gaugeNumber);
            return $"{SetMachineState},{gaugeNumber},{(int)state}";
        }

        public static string BuildPingCommand()
        {
            return Ping;
        }

        private static void ValidateGaugeNumber(int gaugeNumber)
        {
            if (gaugeNumber < 1 || gaugeNumber > 4)
            {
                throw new ArgumentOutOfRangeException(
                    nameof(gaugeNumber),
                    "Gauge number must be between 1 and 4.");
            }
        }

        private static void ValidateReedNumber(int reedNumber)
        {
            if (reedNumber < 1 || reedNumber > 8)
            {
                throw new ArgumentOutOfRangeException(
                    nameof(reedNumber),
                    "Reed number must be between 1 and 8.");
            }
        }
    }
}