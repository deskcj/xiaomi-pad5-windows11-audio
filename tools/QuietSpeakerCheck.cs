using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;

namespace NabuAudio
{
    public static class QuietSpeakerCheck
    {
        private const double Amplitude = 0.006;

        public sealed class Result
        {
            public string Endpoint { get; set; }
            public int SampleRateHz { get; set; }
            public double TonePeakDbfs { get; set; }
            public float[] BeforePlayback { get; set; }
            public float[] LeftToneEndpointPeaks { get; set; }
            public float[] RightToneEndpointPeaks { get; set; }
            public string Microphone { get; set; }
            public double? MicrophoneIdleToneDbfs { get; set; }
            public double? MicrophoneLeftToneDbfs { get; set; }
            public double? MicrophoneRightToneDbfs { get; set; }
        }

        public static Result Run(bool measureBuiltInMic)
        {
            var enumerator = (IMMDeviceEnumerator)Activator.CreateInstance(
                Type.GetTypeFromCLSID(new Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")));
            IMMDevice endpoint = null;
            IAudioClient audio = null;
            IAudioMeterInformation meter = null;
            IAudioRenderClient renderer = null;
            IntPtr mixFormat = IntPtr.Zero;
            MicrophoneToneMeter microphone = null;

            try
            {
                Check(enumerator.GetDefaultAudioEndpoint(0, 1, out endpoint));
                string name = GetEndpointName(endpoint);
                bool builtIn = name.StartsWith("Speakers (", StringComparison.OrdinalIgnoreCase)
                    || name.StartsWith("Динамики (", StringComparison.OrdinalIgnoreCase);
                if (!builtIn || name.IndexOf("Mi Pad 5 Qualcomm", StringComparison.OrdinalIgnoreCase) < 0)
                    throw new InvalidOperationException("Select the Xiaomi Pad 5 built-in speakers first. Default output: " + name);

                audio = Activate<IAudioClient>(endpoint);
                meter = Activate<IAudioMeterInformation>(endpoint);
                Check(meter.GetMeteringChannelCount(out uint meterChannels));
                if (meterChannels != 2)
                    throw new InvalidOperationException("Expected two channels on the endpoint peak meter.");

                var idle = new float[2];
                Check(meter.GetChannelsPeakValues(2, idle));
                if (Math.Max(idle[0], idle[1]) > 0.003)
                    throw new InvalidOperationException("Other audio is playing. Pause it and retry the quiet test.");

                Check(audio.GetMixFormat(out mixFormat));
                int sampleRate = Marshal.ReadInt32(mixFormat, 4);
                if (Marshal.ReadInt16(mixFormat, 2) != 2 || sampleRate < 8000 || sampleRate > 192000)
                    throw new InvalidOperationException("Expected an ordinary stereo mix format.");

                var format = new WaveFormatEx
                {
                    Tag = 1, Channels = 2, SampleRate = sampleRate,
                    BytesPerSecond = sampleRate * 4, BlockAlign = 4, BitsPerSample = 16
                };
                // Shared mode, with PCM conversion to the endpoint's mix format.
                Check(audio.Initialize(0, 0x88000000, 1000000, 0, ref format, IntPtr.Zero));
                Guid rendererId = typeof(IAudioRenderClient).GUID;
                Check(audio.GetService(ref rendererId, out object service));
                renderer = (IAudioRenderClient)service;

                if (measureBuiltInMic)
                    microphone = new MicrophoneToneMeter(enumerator);

                var result = new Result
                {
                    Endpoint = name, SampleRateHz = sampleRate,
                    TonePeakDbfs = Math.Round(20 * Math.Log10(Amplitude), 1),
                    BeforePlayback = idle,
                    Microphone = microphone?.Name,
                    MicrophoneIdleToneDbfs = microphone?.MeasureBackground(),
                    LeftToneEndpointPeaks = Play(audio, renderer, meter, sampleRate, 0, microphone)
                };
                result.MicrophoneLeftToneDbfs = microphone?.GetToneDbfs();
                Thread.Sleep(500);
                result.RightToneEndpointPeaks = Play(audio, renderer, meter, sampleRate, 1, microphone);
                result.MicrophoneRightToneDbfs = microphone?.GetToneDbfs();
                return result;
            }
            finally
            {
                microphone?.Dispose();
                if (mixFormat != IntPtr.Zero) Marshal.FreeCoTaskMem(mixFormat);
                if (renderer != null) Marshal.ReleaseComObject(renderer);
                if (meter != null) Marshal.ReleaseComObject(meter);
                if (audio != null) Marshal.ReleaseComObject(audio);
                if (endpoint != null) Marshal.ReleaseComObject(endpoint);
                Marshal.ReleaseComObject(enumerator);
            }
        }

        private static float[] Play(IAudioClient audio, IAudioRenderClient renderer,
            IAudioMeterInformation meter, int rate, int activeChannel, MicrophoneToneMeter microphone)
        {
            var samples = new short[rate * 2];
            int fade = rate / 20;
            for (int frame = 0; frame < rate; frame++)
            {
                double envelope = Math.Min(1.0, (double)Math.Min(frame, rate - frame - 1) / fade);
                samples[2 * frame + activeChannel] = (short)Math.Round(
                    short.MaxValue * Amplitude * envelope * Math.Sin(2 * Math.PI * 440 * frame / rate));
            }

            Check(audio.GetBufferSize(out uint capacity));
            int queued = 0;
            bool started = false;
            var peaks = new float[2];
            var reading = new float[2];
            microphone?.BeginMeasurement();
            var clock = Stopwatch.StartNew();

            try
            {
                while (clock.ElapsedMilliseconds < 5000)
                {
                    microphone?.Drain(started && clock.ElapsedMilliseconds >= 200);
                    Check(audio.GetCurrentPadding(out uint pending));
                    if (queued == rate && pending == 0)
                        return peaks;

                    int count = Math.Min((int)(capacity - pending), rate - queued);
                    if (count > 0)
                    {
                        Check(renderer.GetBuffer((uint)count, out IntPtr destination));
                        try { Marshal.Copy(samples, 2 * queued, destination, 2 * count); }
                        catch { renderer.ReleaseBuffer((uint)count, 2); throw; }
                        Check(renderer.ReleaseBuffer((uint)count, 0));
                        queued += count;
                    }

                    if (!started)
                    {
                        Check(audio.Start());
                        started = true;
                    }

                    Check(meter.GetChannelsPeakValues(2, reading));
                    peaks[0] = Math.Max(peaks[0], reading[0]);
                    peaks[1] = Math.Max(peaks[1], reading[1]);
                    Thread.Sleep(5);
                }
                throw new TimeoutException("Timed out while playing the quiet sample.");
            }
            finally
            {
                if (started) audio.Stop();
                audio.Reset();
            }
        }

        private sealed class MicrophoneToneMeter : IDisposable
        {
            private const int Rate = 48000;
            private const int Window = Rate / 20;
            private readonly IAudioClient audio;
            private readonly IAudioCaptureClient reader;
            private readonly List<double> levels = new List<double>();
            private int position;
            private double inPhase;
            private double quadrature;
            public string Name { get; }

            public MicrophoneToneMeter(IMMDeviceEnumerator enumerator)
            {
                Check(enumerator.GetDefaultAudioEndpoint(1, 0, out IMMDevice endpoint));
                try
                {
                    Name = GetEndpointName(endpoint);
                    bool internalMic = Name.StartsWith("Internal Microphone", StringComparison.OrdinalIgnoreCase)
                        || Name.StartsWith("Встроенный микрофон", StringComparison.OrdinalIgnoreCase)
                        || Name.StartsWith("Массив микрофонов", StringComparison.OrdinalIgnoreCase);
                    if (!internalMic || Name.IndexOf("Mi Pad 5 Qualcomm", StringComparison.OrdinalIgnoreCase) < 0)
                        throw new InvalidOperationException("Select an internal Xiaomi Pad 5 microphone. Default input: " + Name);

                    audio = Activate<IAudioClient>(endpoint);
                    var format = new WaveFormatEx
                    {
                        Tag = 1, Channels = 1, SampleRate = Rate,
                        BytesPerSecond = Rate * 2, BlockAlign = 2, BitsPerSample = 16
                    };
                    Check(audio.Initialize(0, 0x88000000, 2000000, 0, ref format, IntPtr.Zero));
                    Guid id = typeof(IAudioCaptureClient).GUID;
                    Check(audio.GetService(ref id, out object service));
                    reader = (IAudioCaptureClient)service;
                    Check(audio.Start());
                }
                catch
                {
                    Dispose();
                    throw;
                }
                finally { Marshal.ReleaseComObject(endpoint); }
            }

            public void BeginMeasurement()
            {
                Drain(false);
                levels.Clear();
                position = 0;
                inPhase = quadrature = 0;
            }

            public double? MeasureBackground()
            {
                BeginMeasurement();
                var time = Stopwatch.StartNew();
                while (time.ElapsedMilliseconds < 650)
                {
                    Drain(true);
                    Thread.Sleep(5);
                }
                return GetToneDbfs();
            }

            public void Drain(bool measure)
            {
                Check(reader.GetNextPacketSize(out uint frames));
                while (frames != 0)
                {
                    Check(reader.GetBuffer(out IntPtr data, out frames, out uint flags, out _, out _));
                    try
                    {
                        if (measure)
                        {
                            bool silence = (flags & 2) != 0;
                            for (int sample = 0; sample < frames; sample++)
                            {
                                double value = silence ? 0 : Marshal.ReadInt16(data, sample * 2) / 32768.0;
                                double angle = 2 * Math.PI * 440 * position / Rate;
                                inPhase += value * Math.Cos(angle);
                                quadrature += value * Math.Sin(angle);
                                if (++position < Window) continue;

                                levels.Add(2 * Math.Sqrt(inPhase * inPhase + quadrature * quadrature) / Window);
                                position = 0;
                                inPhase = quadrature = 0;
                            }
                        }
                    }
                    finally { Check(reader.ReleaseBuffer(frames)); }
                    Check(reader.GetNextPacketSize(out frames));
                }
            }

            public double? GetToneDbfs()
            {
                if (levels.Count == 0) return null;
                levels.Sort();
                double amplitude = levels[levels.Count / 2];
                return Math.Round(20 * Math.Log10(Math.Max(amplitude, 0.000001)), 1);
            }

            public void Dispose()
            {
                if (audio != null) audio.Stop();
                if (reader != null) Marshal.ReleaseComObject(reader);
                if (audio != null) Marshal.ReleaseComObject(audio);
            }
        }

        [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("C8ADBD64-E71E-48A0-A4DE-185C395CD317")]
        private interface IAudioCaptureClient
        {
            [PreserveSig] int GetBuffer(out IntPtr samples, out uint frames, out uint flags,
                out ulong devicePosition, out ulong qpcPosition);
            [PreserveSig] int ReleaseBuffer(uint frames);
            [PreserveSig] int GetNextPacketSize(out uint frames);
        }

        private static T Activate<T>(IMMDevice endpoint) where T : class
        {
            Guid id = typeof(T).GUID;
            Check(endpoint.Activate(ref id, 23, IntPtr.Zero, out object instance));
            return (T)instance;
        }

        private static string GetEndpointName(IMMDevice endpoint)
        {
            Check(endpoint.OpenPropertyStore(0, out IPropertyStore properties));
            var key = new PropertyKey
            {
                Format = new Guid("A45C254E-DF1C-4EFD-8020-67D146A850E0"), Id = 14
            };
            try
            {
                Check(properties.GetValue(ref key, out PropVariant value));
                try { return value.Type == 31 ? Marshal.PtrToStringUni(value.Pointer) ?? "" : ""; }
                finally { PropVariantClear(ref value); }
            }
            finally { Marshal.ReleaseComObject(properties); }
        }

        private static void Check(int hr)
        {
            if (hr < 0) Marshal.ThrowExceptionForHR(hr);
        }

        [DllImport("ole32.dll")]
        private static extern int PropVariantClear(ref PropVariant value);

        [StructLayout(LayoutKind.Sequential, Pack = 2)]
        private struct WaveFormatEx
        {
            public ushort Tag, Channels;
            public int SampleRate, BytesPerSecond;
            public ushort BlockAlign, BitsPerSample, ExtraSize;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct PropertyKey { public Guid Format; public uint Id; }

        [StructLayout(LayoutKind.Explicit, Size = 24)]
        private struct PropVariant
        {
            [FieldOffset(0)] public ushort Type;
            [FieldOffset(8)] public IntPtr Pointer;
        }

        [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("A95664D2-9614-4F35-A746-DE8DB63617E6")]
        private interface IMMDeviceEnumerator
        {
            [PreserveSig] int EnumAudioEndpoints(int flow, uint state, out IntPtr devices);
            [PreserveSig] int GetDefaultAudioEndpoint(int flow, int role, out IMMDevice device);
        }

        [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("D666063F-1587-4E43-81F1-B948E807363F")]
        private interface IMMDevice
        {
            [PreserveSig] int Activate(ref Guid id, uint context, IntPtr parameters,
                [MarshalAs(UnmanagedType.IUnknown)] out object instance);
            [PreserveSig] int OpenPropertyStore(uint access, out IPropertyStore properties);
        }

        [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("886D8EEB-8CF2-4446-8D02-CDBA1DBDCF99")]
        private interface IPropertyStore
        {
            [PreserveSig] int GetCount(out uint count);
            [PreserveSig] int GetAt(uint index, out PropertyKey key);
            [PreserveSig] int GetValue(ref PropertyKey key, out PropVariant value);
        }

        [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("1CB9AD4C-DBFA-4C32-B178-C2F568A703B2")]
        private interface IAudioClient
        {
            [PreserveSig] int Initialize(int mode, uint flags, long duration, long period,
                ref WaveFormatEx format, IntPtr session);
            [PreserveSig] int GetBufferSize(out uint frames);
            [PreserveSig] int GetStreamLatency(out long latency);
            [PreserveSig] int GetCurrentPadding(out uint frames);
            [PreserveSig] int IsFormatSupported(int mode, IntPtr format, out IntPtr closest);
            [PreserveSig] int GetMixFormat(out IntPtr format);
            [PreserveSig] int GetDevicePeriod(out long regular, out long minimum);
            [PreserveSig] int Start();
            [PreserveSig] int Stop();
            [PreserveSig] int Reset();
            [PreserveSig] int SetEventHandle(IntPtr handle);
            [PreserveSig] int GetService(ref Guid id, [MarshalAs(UnmanagedType.IUnknown)] out object service);
        }

        [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("F294ACFC-3146-4483-A7BF-ADDCA7C260E2")]
        private interface IAudioRenderClient
        {
            [PreserveSig] int GetBuffer(uint frames, out IntPtr samples);
            [PreserveSig] int ReleaseBuffer(uint frames, uint flags);
        }

        [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("C02216F6-8C67-4B5B-9D00-D008E73E0064")]
        private interface IAudioMeterInformation
        {
            [PreserveSig] int GetPeakValue(out float peak);
            [PreserveSig] int GetMeteringChannelCount(out uint count);
            [PreserveSig] int GetChannelsPeakValues(uint count,
                [Out, MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 0)] float[] peaks);
        }
    }
}
